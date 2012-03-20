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
import SocketServer
import socket
import urllib2
from optparse import OptionParser
from datetime import datetime
from multiprocessing import Process, Queue, Pipe, JoinableQueue
import subprocess
import pylzma
import json
import tempfile
import struct

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
    for each_input in inputs:
        binary = open(each_input, 'r').read();
        length = os.path.getsize(each_input)
        if len(binary) != length:
            sys.stderr.write("Error, input length is wrong");
            sys.exit(1)

        # send
        sent_size = sock.send(struct.pack("!I", length))
        if not sent_size == 4:
            sys.strerr.write("Error, send wrong size of int : %d" % sent_size)
            sys.exit(1)
        sent_size = sock.send(binary)
        if not sent_size == length:
            sys.strerr.write("Error, send wrong size of file : %d" % sent_size)
            sys.exit(1)
        
        #recv
        data = sock.recv(4)
        ret_size = struct.unpack("!I", data)[0]
        print "recv size : %d" % ret_size
        if ret_size == 0:
            print "no object is found"
        else:
            ret_data = sock.recv(ret_size)
            print "Return obj : %s" % ret_data


def main(argv=None):
    global LOCAL_IPADDRESS
    settings, args = process_command_line(sys.argv[1:])

    files = [os.path.join(settings.input_dir, file) for file in os.listdir(settings.input_dir) if file[-3:] == "jpg" or file[-3:] == "JPG"]  
    send_request(settings.server_address, settings.server_port, files)

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
