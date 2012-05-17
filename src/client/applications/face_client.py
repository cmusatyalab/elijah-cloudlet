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
import time
import struct
import math

# protocol commands
MESSAGE_TYPE_JPEG_IMAGE = 1
MESSAGE_TYPE_IMAGE_REPONSE = 2

def get_local_ipaddress():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("gmail.com",80))
    ipaddress = (s.getsockname()[0])
    s.close()
    return ipaddress


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog [option]", version="Face Recognition Desktop Client")
    parser.add_option(
            '-d', '--dir', action='store', type='string', dest='input_dir',
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
    
    if not settings.input_dir:
        parser.error("input directory does no exists at :%s" % (settings.input_dir))
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
    print "image\tstart\tend\tduration\tjitter"
    for each_input in inputs:
        start_time_request = time.time() * 1000.0
        binary = open(each_input, 'rb').read();

        ret_data = face_request(sock, binary)

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


def face_request(sock, data):
    if len(data) > 100*1000: # 100k
        sys.stderr.write("Error, Server cannot support image bigger than 100KB\n")
        return 'Server cannot support > 100 KB iamges'

    # send
    sock.sendall(struct.pack("!I", MESSAGE_TYPE_JPEG_IMAGE))
    sock.sendall(struct.pack("!I", len(data)))
    sock.sendall(data)
    
    #recv
    message_type = struct.unpack("!I", sock.recv(4))[0]
    message_size = struct.unpack("!I", sock.recv(4))[0]
    recvedData = struct.unpack("!IIIIIIIII", sock.recv(4*9))
    #detectTimeinMs = struct.unpack("!I", sock.recv(4(8))
    #objectsFound = struct.unpack("!I", sock.recv(4))[0]
    #drawRect = struct.unpack("!I", sock.recv(4))[0]
    #havePerson = struct.unpack("!I", sock.recv(4))[0]
    #rect_x = struct.unpack("!I", sock.recv(4))[0]
    #rect_y = struct.unpack("!I", sock.recv(4))[0]
    #rect_width = struct.unpack("!I", sock.recv(4))[0]
    #rect_height = struct.unpack("!I", sock.recv(4))[0]
    #confidence = struct.unpack("!I", sock.recv(4))[0]
    
    found_name = ''
    recv_size = message_size-36 # 36=9*4
    if not recv_size == 0:
        found_name = sock.recv(recv_size)
    
    return found_name 


def main(argv=None):
    global LOCAL_IPADDRESS
    settings, args = process_command_line(sys.argv[1:])

    files = [os.path.join(settings.input_dir, file) for file in os.listdir(settings.input_dir) if file[-3:] == "jpg" or file[-3:] == "JPG"]  
    send_request(settings.server_address, settings.server_port, files)

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
