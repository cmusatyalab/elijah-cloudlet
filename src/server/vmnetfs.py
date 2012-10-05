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
import time
import sys

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
    DISK_MODIFY = "DISK_MODIFY"
    DISK_ACCESS = "DISK_ACCESS"
    MEMORY_ACCESS = "MEMORY_ACCESS"

    def __init__(self):
        self.epoll = select.epoll()
        self.stream_dict = dict()
        self._running = False
        self.stop = threading.Event()
        self.chunk_list = list()
        threading.Thread.__init__(self, target=self.io_watch)

    def add_path(self, path, name):
        # We need to set O_NONBLOCK in open() because FUSE doesn't pass
        # through fcntl()
        print "[INFO] start monitoring at %s" % path
        fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
        self.stream_dict[fd] = {'name':name, 'buf':''}
        self.epoll.register(fd, select.EPOLLIN | select.EPOLLOUT | select.EPOLLPRI)

    def io_watch(self):
        while(not self.stop.wait(0.0001)):
            self._running = True
            events = self.epoll.poll(0.0001)
            for fileno, event in events:
                self._handle(fileno, event)
        
        for fileno in self.stream_dict.keys():
            self.epoll.unregister(fileno)
            os.close(fileno)
        self._running = False
        print "[INFO] close monitoring thread"

    def _handle(self, fd, event):
        if event & select.EPOLLIN:
            #print "%d, %s" % (fd, self.stream_dict[fd]['name'])
            try:
                buf = os.read(fd, 1024)
            except OSError as e:
                # TODO: "Resource temporarily unavailable" Error
                return

            # We got some output
            lines = (self.stream_dict[fd]['buf']+ buf).split('\n')
            # Save partial last line, if any
            self.stream_dict[fd]['buf'] = lines.pop()
            for line in lines:
                stream_name = self.stream_dict[fd]['name']
                if stream_name == StreamMonitor.DISK_MODIFY:
                    self._handle_chunks_modification(line)
                elif stream_name == StreamMonitor.DISK_ACCESS:
                    self._handle_disk_access(line)
                elif stream_name == StreamMonitor.MEMORY_ACCESS:
                    self._handle_memory_access(line)
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
        self.chunk_list.append((ctime, chunk))
        #print "%s: %f, %d" % ("modification", ctime, chunk)

    def _handle_disk_access(self, line):
        #print "access: " + line
        pass

    def _handle_memory_access(self, line):
        #print "memory access: " + line
        pass

    def terminate(self):
        self.stop.set()


class FileMonitor(threading.Thread):
    QEMU_LOG    = "QEMU_LOG"

    def __init__(self, path, name):
        self.stream_dict = dict()
        self._running = False
        self.stop = threading.Event()
        self.fd = os.open(path, os.O_RDONLY | os.O_NONBLOCK)
        self.buf = ''
        threading.Thread.__init__(self, target=self.io_watch)
        print "[INFO] start monitoring at %s" % path

    def io_watch(self):
        while(not self.stop.wait(0.0001)):
            self._running = True
            line = os.read(self.fd, 1024)
            lines = (self.buf+line).split('\n')
            self.buf = lines.pop()
            for line in lines:
                self._handle_qemu_log(line)
            else:
                time.sleep(0.001)
        self._running = False
        print "[INFO] close monitoring thread"

    def _handle_qemu_log(self, line):
        splits = line.split(",", 2)
        event_time = splits[0].strip()
        header = splits[1].strip()
        data = splits[2].strip()
        if header == 'dma':
            #sys.stdout.write("(%s)\n" % line)
            pass
        elif header == 'bdrv_discard':
            sys.stdout.write("discard:(%s, %s)\n" % (event_time, data))
        else:
            sys.stdout.write("invalid log: (%s)(%s)(%s)\n" % (event_time, header, data))

    def terminate(self):
        self.stop.set()
