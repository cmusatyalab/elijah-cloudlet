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

import subprocess
import threading
import os
import sys
from Configuration import Const as Const


class UPnPError(Exception):
    pass


class UPnPServer(threading.Thread):

    def __init__(self):
        self.stop = threading.Event()
        self.upnp_bin = Const.UPnP_SERVER
        self.proc = None
        if os.path.exists(self.upnp_bin) == False:
            raise UPnPError("Cannot find binary: %s" % self.upnp_bin)
        threading.Thread.__init__(self, target=self.run_exec)

    def run_exec(self):
        cmd = ["java", "-jar", "%s" % (self.upnp_bin)]
        _PIPE = subprocess.PIPE
        self.proc = subprocess.Popen(cmd, close_fds=True, stdin=_PIPE, stdout=_PIPE, stderr=_PIPE)
        try:
            while(not self.stop.wait(10)):
                self.proc.poll()
                return_code = self.proc.returncode
                if return_code != None:
                    if return_code == 0:
                        self.proc = None
                        break
                    if return_code != 0:
                        msg = "[Error] UPnP is closed unexpectedly: %d\n" % \
                                return_code
                        sys.stderr.write(msg)
                        break
        except KeyboardInterrupt, e:
            self.terminate()

    def terminate(self):
        self.stop.set()
        if self.proc != None:
            import signal
            self.proc.send_signal(signal.SIGINT) 
            return_code = self.proc.poll()
            if return_code == None:
                self.proc.terminate()
            elif return_code != 0:
                msg = "[Error] UPnP is closed unexpectedly: %d\n" % \
                        return_code
                sys.stderr.write(msg)
            else:
                print "terminate success!"


