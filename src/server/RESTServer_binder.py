import subprocess
import threading
import os
import sys
from Configuration import Const as Const


class RESTServerError(Exception):
    pass


class RESTServer(threading.Thread):
    def __init__(self):
        self.stop = threading.Event()
        self.REST_bin = Const.REST_SERVER_BIN
        self.proc = None
        if os.path.exists(self.REST_bin) == False:
            raise RESTServerError("Cannot find binary: %s" % self.REST_bin)
        threading.Thread.__init__(self, target=self.run_exec)

    def run_exec(self):
        cmd = "python %s" % (self.REST_bin)
        _PIPE = subprocess.PIPE
        self.proc = subprocess.Popen(cmd, shell=True, \
                stdin=_PIPE, stdout=_PIPE, stderr=_PIPE)
        try:
            while(not self.stop.wait(10)):
                self.proc.poll()
                return_code = self.proc.returncode
                if return_code != None:
                    if return_code == 0:
                        self.proc = None
                        break
                    if return_code != 0:
                        msg = "[Error] RESTful API Server is closed unexpectedly\n"
                        sys.stderr.write(msg)
                        break
        except KeyboardInterrupt, e:
            pass

    def terminate(self):
        self.stop.set()
        if self.proc != None:
            import signal
            self.proc.send_signal(signal.SIGINT) 
            self.proc.wait()
            if self.proc.returncode == None:
                self.proc.terminate()
            elif self.proc.returncode != 0:
                msg = "[Error] RESTful Server closed unexpectedly: %d\n" % \
                        self.proc.returncode
                pass

