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

def get_local_ipaddress():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("gmail.com",80))
    ipaddress = (s.getsockname()[0])
    s.close()
    return ipaddress


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog [option]", version="MOPED Desktop Client")
    parser.add_option(
            '-i', '--input', action='store', type='string', dest='input_dir',
            help='Set Input image directory')
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server_address', default="localhost",
            help='Set Input image directory')
    parser.add_option(
            '-p', '--port', action='store', type='int', dest='server_port', default=9092,
            help='Set Input image directory')
    parser.add_option(
            '-r', '--repeat', action='store', type='int', dest='conn_repeat', default=100,
            help='Repeat connecting number')
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    if not settings.input_dir:
        parser.error("input directory does no exists at :%s" % (settings.input_dir))
    if not os.path.isdir(settings.input_dir):
        parser.error("input directory does no exists at :%s" % (settings.input_dir))

    return settings, args


def send_request(address, port, inputs, conn_repeat):
    # connection
    conn_count = 0
    connect_start_time = time.time()
    while conn_count < conn_repeat:
        try:
            print "Connecting..."
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.setblocking(True)
            sock.connect((address, port))
            conn_count += 1
            break
        except socket.error, msg:
            print "Connection failed, retry"
            sock.close()
            time.sleep(0.1)

    if (sock == None) or (sock.fileno() <= 0):
        sys.stderr.write("Connection faild to (%s:%d)\n" % (address, port))
        sys.exit(1)

    connect_end_time = time.time()
    print "Connecting to (%s, %d) takes %f seconds" % \
            (address, port, (connect_end_time-connect_start_time))


    # send requests
    current_duration = -1
    print "image\tstart\tend\tduration\tjitter"
    for each_input in inputs:
        start_time_request = time.time() * 1000.0
        binary = open(each_input, 'rb').read();
        ret_data = moped_request(sock, binary)

        # print result
        end_time_request = time.time() * 1000.0
        prev_duration = current_duration
        current_duration = end_time_request-start_time_request

        if prev_duration == -1: # fisrt response
            print "%s\t%014.2f\t%014.2f\t%014.2f\t0" % (each_input, start_time_request,\
                    end_time_request, \
                    end_time_request-start_time_request)
        else:
            print "%s\t%014.2f\t%014.2f\t%014.2f\t%014.2f" % (each_input, round(start_time_request, 3), \
                    end_time_request, \
                    current_duration, \
                    math.fabs(current_duration-prev_duration))


def moped_request(sock, data):
    length = len(data)

    # send
    sock.sendall(struct.pack("!I", length))
    sock.sendall(data)
    
    #recv
    data = sock.recv(4)
    ret_size = struct.unpack("!I", data)[0]
    
    ret_data = ''
    if not ret_size == 0:
        ret_data = sock.recv(ret_size)
        return ret_data

    return None

def main(argv=None):
    global LOCAL_IPADDRESS
    settings, args = process_command_line(sys.argv[1:])

    files = [os.path.join(settings.input_dir, file) for file in os.listdir(settings.input_dir) if file[-3:] == "jpg" or file[-3:] == "JPG"]  
    send_request(settings.server_address, settings.server_port, files, settings.conn_repeat)

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
