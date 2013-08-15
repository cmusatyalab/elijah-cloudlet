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

import time
import socket
import struct
import sys
import cloudlet_client
import subprocess

def main(argv):
    start_time = time.time()
    if len(sys.argv) != 4:
        sys.stderr.write("Resume VM and wait for first run\n \
                1) ip_adress\n \
                2) port_number\n \
                3) app_name\n")
        sys.exit(1)
    # create overlay
    ip_address = sys.argv[1]
    port = sys.argv[2]
    app_name = sys.argv[3]

    print "Connecting to %s:%d.." % (ip_address, int(port))
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((ip_address, int(port)))

    # waiting for socket command
    client_socket.sendall(struct.pack("!I", len(app_name)))
    client_socket.sendall(struct.pack("%ds" % len(app_name), app_name))

    # run installation script
    if app_name == "moped":
        script = "/var/www/download/moped_inst.sh"
    elif app_name == "speech":
        script = "/var/www/download/speech_inst.sh"
    elif app_name == "graphics":
        script = "/var/www/download/graphics_inst.sh"
    else:
        sys.stderr.write("No valid application name : %s" % app_name)
        sys.exit(1)

    '''
    _PIPE = subprocess.PIPE
    proc = subprocess.Popen(script, stdin=_PIPE, shell=True)
    proc.wait()
    '''

    cloudlet_client.run_application(app_name)
    print "first response time for %s is %f" % (app_name, time.time()-start_time)


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
