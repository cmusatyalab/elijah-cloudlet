#!/usr/bin/env python 
#
# Cloudlet Infrastructure for Mobile Computing
#
#   Author: Kiryong Ha <krha@cmu.edu>
#
#   Copyright (C) 2011-2013 Carnegie Mellon University
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
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

    def __init__(self, bin_path, cache_root, url_root, redis_addr, \
            redis_req, redis_res, print_out=None):
        self._running = True
        self.cachefs_bin = bin_path
        self.cache_root = cache_root
        self.url_root = url_root
        self._args = "%s %s %s %s %s %s" % \
                (str(self.cache_root), str(self.url_root), \
                str(redis_addr[0]), str(redis_addr[1]), \
                str(redis_req), \
                str(redis_res))
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
            oneline = self.proc.stdout.readline()
            if len(oneline.strip()) <= 0:
                continue

            if oneline.startswith(CacheFS.PREFIX_REQUEST) == True:
                self.handle_request(oneline[len(CacheFS.PREFIX_REQUEST):])
            elif oneline.startswith(CacheFS.PREFIX_DEBUG) == True:
                self.print_out.write(oneline)
                pass
            elif oneline.startswith(CacheFS.PREFIX_ERROR) == True:
                self.print_out.write(oneline+"\n")

        if self.proc != None:
            return_code = self.proc.poll()
            if return_code == None:
                self.proc.wait()
                self.proc = None
        self.print_out.write("[INFO] close Fuse Exec thread\n")
        self._running = False

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
            self.fuse_write("terminate")
            self._pipe.close()
            self._pipe = None

