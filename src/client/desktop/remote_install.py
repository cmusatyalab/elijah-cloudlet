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

import time
import socket
import struct
import sys
import cloudlet_client

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

    cloudlet_client.run_application(app_name)


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
