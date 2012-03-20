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
import sys
import socket
from optparse import OptionParser
import subprocess
import json
import tempfile
import time
import struct
import math
import random

def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog [option]", version="MOPED Desktop Client")
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server_address', default="localhost",
            help='Set Input image directory')
    parser.add_option(
            '-p', '--port', action='store', type='int', dest='server_port', default=9093,
            help='Set Input image directory')
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    if not os.path.isdir(settings.input_dir):
        parser.error("input directory does no exists at :%s" % (settings.input_dir))
    
    return settings, args


def send_request(address, port, inputs):
    # connection
    try:
        print "Connecting to (%s, %d).." % (address, port)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setblocking(True)
        sock.connect((address, port))
    except socket.error, msg:
        sys.stderr.write("Error, %s\n" % msg[1])
        sys.exit(1)

    # send requests
    current_duration = -1
    print "index\tstart\tend\tduration\tjitter"
    for index in xrange(100):
        start_time_request = time.time()

        # send acc data
        x_acc = random.uniform(-27, 27)
        y_acc = random.uniform(-27, 27)
        sent_size = sock.send(struct.pack("!f!f", x_acc, y_acc))
        if not sent_size == 8:
            sys.strerr.write("Error, send wrong size of acc data: %d" + sent_size)
            sys.exit(1)
        
        #recv
        data = sock.recv(4)
        ret_size = struct.unpack("!I", data)[0]
        
        if not ret_size == 0:
            ret_data = sock.recv(ret_size)
            print "returned value : %d" % (len(ret_data))
        else:
            sys.strerr.write("Error, return size must not be zero")
            sys.exit(1)

        # print result
        end_time_request = time.time()
        prev_duration = current_duration
        current_duration = end_time_request-start_time_request

        if prev_duration == -1: # fisrt response
            print "%d\t%05.3f\t%05.3f\t%05.3f\t0" % (index, start_time_request,\
                    end_time_request, \
                    end_time_request-start_time_request)
        else:
            print "%d\t%05.3f\t%05.3f\t%05.3f\t%05.3f" % (index, round(start_time_request, 3), \
                    end_time_request, \
                    current_duration, \
                    math.fabs(current_duration-prev_duration))

def main(argv=None):
    global LOCAL_IPADDRESS
    settings, args = process_command_line(sys.argv[1:])

    files = [os.path.join(settings.input_dir, file) for file in os.listdir(settings.input_dir) if file[-3:] == "jpg" or file[-3:] == "JPG"]  
    send_request(settings.server_address, settings.server_port, files)

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
