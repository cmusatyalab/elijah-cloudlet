#
# vmnetx.vmnetfs - Wrapper for vmnetfs FUSE driver
#
# Copyright (C) 2011-2012 Carnegie Mellon University
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.  A copy of the GNU General Public License
# should have been distributed along with this program in the file
# COPYING.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

import os
import subprocess
import select
import io
import threading

# system.py is built at install time, so pylint may fail to import it.
# Also avoid warning on variable name.
# pylint: disable=F0401,C0103
# pylint: enable=F0401,C0103

class VMNetFSError(Exception):
    pass


class VMNetFS(object):
    def __init__(self, bin_path, args):
        self.vmnetfs_path = bin_path
        self._args = '%d\n%s\n' % (len(args),
                '\n'.join(a.replace('\n', '') for a in args))
        self._pipe = None
        self.mountpoint = None

    # pylint is confused by the values returned from Popen.communicate()
    # pylint: disable=E1103
    def start(self):
        read, write = os.pipe()
        try:
            proc = subprocess.Popen([self.vmnetfs_path], stdin=read,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    close_fds=True)
            self._pipe = os.fdopen(write, 'w')
            self._pipe.write(self._args)
            self._pipe.flush()
            out, err = proc.communicate()
            if len(err) > 0:
                print "Error: " + str(err)
                raise VMNetFSError(err.strip())
            elif proc.returncode > 0:
                raise VMNetFSError('vmnetfs returned status %d' %
                        proc.returncode)
            self.mountpoint = out.strip()
        except:
            if self._pipe is not None:
                self._pipe.close()
            else:
                os.close(write)
            raise
        finally:
            pass
            #os.close(read)
    # pylint: enable=E1103

    def terminate(self):
        if self._pipe is not None:
            self._pipe.close()
            self._pipe = None


class StreamMonitor(threading.Thread):
    def __init__(self):
        self.epoll = select.epoll()
        self.stream_dict = dict()
        self._running = False
        self.stop = threading.Event()
        self.chunk_list = list()
        threading.Thread.__init__(self, target=self.io_watch)

    def add_path(self, path):
        # We need to set O_NONBLOCK in open() because FUSE doesn't pass
        # through fcntl()
        print "[INFO] start monitoring at %s" % path
        name = os.path.basename(path)
        fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
        self.stream_dict[fd] = {'name':name, 'buf':''}
        self.epoll.register(fd, select.EPOLLIN | select.EPOLLOUT | select.EPOLLPRI)

    def io_watch(self):
        while(not self.stop.wait(0.1)):
            self._running = True
            events = self.epoll.poll(0.1)
            for fileno, event in events:
                self._handle(fileno, event)
        
        for fileno in self.stream_dict.keys():
            self.epoll.unregister(fileno)
            os.close(fileno)
        self._running = False
        print "[INFO] close monitoring thread"

    def _handle(self, fd, event):
        if event & select.EPOLLIN:
            buf = os.read(fd, 1024)
            # We got some output
            lines = (self.stream_dict[fd]['buf']+ buf).split('\n')
            # Save partial last line, if any
            self.stream_dict[fd]['buf'] = lines.pop()
            for line in lines:
                stream_name = self.stream_dict[fd]['name']
                if stream_name == "chunks_modified":
                    self._handle_chunks_modification(line)
                elif stream_name == "chunks_accessed":
                    self._handle_chunks_access(line)
                else:
                    raise IOError("Error, invalid stream")
                    break
        elif event & select.EPOLLOUT:
            pass
        elif event & select.EPOLLPRI:
            print "error?"

    def _handle_chunks_modification(self, line):
        ctime, chunk = line.split("\t")
        ctime = float(ctime)
        chunk = int(chunk)
        self.chunk_list.append(int(chunk))
        #print "%s: %f, %d" % ("modification", ctime, chunk)

    def _handle_chunks_access(self, line):
        #print "access: " + line
        pass

    def terminate(self):
        self.stop.set()
