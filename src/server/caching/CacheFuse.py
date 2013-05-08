#!/usr/bin/env python
#
# Elijah: Cloudlet Infrastructure for Mobile Computing
# Copyright (C) 2011-2012 Carnegie Mellon University
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.  A copy of the GNU General Public License
# should have been distributed along with this program in the file
# LICENSE.GPL.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#

import os
import subprocess
import threading


class CacheFuseError(Exception):
    pass


class CacheFS(threading.Thread):
    PREFIX_REQUEST = "[request]"
    PREFIX_DEBUG = "[debug]"
    PREFIX_ERROR = "[error]"

    def __init__(self, bin_path, args, request_queue, print_out=None):
        self.cachefs_bin = bin_path
        self._args = args
        self.request_queue = request_queue
        self.print_out = print_out
        if self.print_out == None:
            self.print_out = open("/dev/null", "w+b")
        self._pipe = None
        self.mountpoint = None
        self.stop = threading.Event()

        # fuse can handle on-demand fetching
        threading.Thread.__init__(self, target=self.fuse_read)

    def launch(self):
        read, write = os.pipe()
        try:
            cmd = ["%s" % self.cachefs_bin]
            cmd.extend(self._args.split())
            self.proc = subprocess.Popen(cmd, stdin=read,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                    close_fds=True)
            self._pipe = os.fdopen(write, 'w')
            out = self.proc.stdout.readline()
            self.mountpoint = out.strip()
        except Exception, e:
            if self._pipe is not None:
                self._pipe.close()
            else:
                os.close(write)
            raise CacheFuseError("%s\nCannot start fuse %s" % (e, str(cmd)))
        finally:
            pass

    def fuse_read(self):
        while(not self.stop.wait(0.0001)):
            self._running = True
            oneline = self.proc.stdout.readline()
            if len(oneline.strip()) <= 0:
                continue

            if oneline.startswith(CacheFS.PREFIX_REQUEST) == True:
                self.handle_request(oneline[len(CacheFS.PREFIX_REQUEST):])
            elif oneline.startswith(CacheFS.PREFIX_DEBUG) == True:
                self.print_out.write(oneline)
                pass
            elif oneline.startswith(CacheFS.PREFIX_ERROR) == True:
                self.print_out.write(oneline)

        self._running = False
        self.print_out.write("[INFO] close Fuse Exec thread\n")

    def fuse_write(self, data):
        self._pipe.write(data + "\n")
        self._pipe.flush()

    def handle_request(self, request_str):
        self.request_queue.put(request_str.strip())

    def terminate(self):
        self.stop.set()
        if self._pipe is not None:
            self.print_out.write("[INFO] Fuse close pipe\n")
            # mal-formatted string will shutdown fuse
            #self.fuse_write("terminate")
            self._pipe.close()
            self._pipe = None

