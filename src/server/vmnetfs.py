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
import glib
import io

# system.py is built at install time, so pylint may fail to import it.
# Also avoid warning on variable name.
# pylint: disable=F0401,C0103
vmnetfs_path = "/home/krha/cloudlet/src/vmnetx/vmnetfs/vmnetfs"
# pylint: enable=F0401,C0103

class VMNetFSError(Exception):
    pass


class VMNetFS(object):
    def __init__(self, args):
        self._args = '%d\n%s\n' % (len(args),
                '\n'.join(a.replace('\n', '') for a in args))
        self._pipe = None
        self.mountpoint = None

    # pylint is confused by the values returned from Popen.communicate()
    # pylint: disable=E1103
    def start(self):
        read, write = os.pipe()
        try:
            proc = subprocess.Popen([vmnetfs_path], stdin=read,
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


class StreamMonitor(object):
    def __init__(self, path):
        # We need to set O_NONBLOCK in open() because FUSE doesn't pass
        # through fcntl()
        self.name = os.path.basename(path)
        self._fh = io.FileIO(os.open(path, os.O_RDONLY | os.O_NONBLOCK))
        self._source = glib.io_add_watch(self._fh, glib.IO_IN | glib.IO_ERR,
                self._read)
        self._buf = ''
        self.chunk_list = list()
        # Defer initial update until requested by caller, to allow the
        # caller to connect to our signal

    def _read(self, _fh=None, _condition=None):
        try:
            buf = self._fh.read()
        except IOError:
            # e.g. vmnetfs crashed
            self.close()
            return False

        if buf == '':
            # EOF
            self.close()
            return False
        elif buf is not None:
            # We got some output
            lines = (self._buf + buf).split('\n')
            # Save partial last line, if any
            self._buf = lines.pop()
            # Emit chunks
            for line in lines:
                ctime, chunk = line.split("\t")
                ctime = float(ctime)
                chunk = int(chunk)
                #print "%s: %f, %d" % (self.name, ctime, chunk)
                self.chunk_list.append(int(chunk))
        return True

    def update(self):
        self._read()

    def close(self):
        if not self._fh.closed:
            self._fh.close()
