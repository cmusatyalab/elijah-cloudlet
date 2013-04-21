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
        cmd = "java -jar %s" % (self.upnp_bin)
        _PIPE = subprocess.PIPE
        self.proc = subprocess.Popen(cmd, shell=True, stdin=_PIPE, stdout=_PIPE, stderr=_PIPE)
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


