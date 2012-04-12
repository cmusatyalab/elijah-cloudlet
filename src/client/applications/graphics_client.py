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
from select import select
from threading import Thread

token_id = 0
total_recv_number = 0;

sender_time_stamps = []
receiver_time_stamps = []

def recv_all(sock, size):
    data = ''
    while len(data) < size:
        data += sock.recv(size - len(data))
    return data


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog [option]", version="MOPED Desktop Client")
    parser.add_option(
            '-i', '--input', action='store', type='string', dest='input_file',
            help='Set Input image directory')
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server_address', default="localhost",
            help='Set Input image directory')
    parser.add_option(
            '-p', '--port', action='store', type='int', dest='server_port', default=9093,
            help='Set Input image directory')
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    return settings, args


def recv_data(sock):
    global token_id
    global total_recv_number
    # recv
    print "index\tstart\tend\tduration\tjitter\tout"
    try:
        while True:
            #data = sock.recv(4)
            #client_id = struct.unpack("!I", data)[0]
            data = sock.recv(4)
            server_token_id = struct.unpack("!I", data)[0]
            data = sock.recv(4)
            ret_size = struct.unpack("!I", data)[0]
            #print "Client ID : %d, Recv size : %d" % (server_token_id, ret_size)
            token_id = server_token_id
            #receiver_time_stamps.append((client_id, time.time()))
            
            if not ret_size == 0:
                ret_data = recv_all(sock, ret_size)
                total_recv_number = total_recv_number + 1
                if not ret_size == len(ret_data):
                    sys.stderr.write("Error, returned value size : %d" % (len(ret_data)))
                    sys.exit(1)
            else:
                sys.stderr.write("Error, return size must not be zero")
                sys.exit(1)

            # print result
            '''
            end_time_request = time.time() * 1000.0
            prev_duration = current_duration
            current_duration = end_time_request-start_time_request
            if prev_duration == -1: # fisrt response
                print "%d\t%014.2f\t%014.2f\t%014.2f\t0\t%014.2f" % (index, start_time_request,\
                        end_time_request, \
                        end_time_request-start_time_request,\
                        len(ret_data))
                        
            else:
                print "%d\t%014.2f\t%014.2f\t%014.2f\t%014.2f\t%014.2f" % (index, round(start_time_request, 3), \
                        end_time_request, \
                        current_duration, \
                        math.fabs(current_duration-prev_duration), \
                        len(ret_data))
            '''

    except socket.error:
        print "Socket Closed and Closing Recv Thread"


def send_request(sock, input_data):
    global token_id

    # send requests
    if input_data:
        loop_length = len(input_data)
    else:
        loop_length = 1000

    index = 0
    last_sent_time = 0
    while True:
        read_ready, write_ready, others = select([sock], [sock], [])
        if sock in write_ready:
            if index == loop_length-1:
                break;

            # send acc data
            if (time.time() - last_sent_time) > 0.020:
                if input_data:
                    if len(input_data[index].split("  ")) != 3:
                        print "Error input : %s" % input_data[index]
                        continue
                    x_acc = float(input_data[index].split("  ")[1])
                    y_acc = float(input_data[index].split("  ")[2])
                else:
                    x_acc = -9.0 
                    y_acc = -1.0

                #sender_time_stamps.append((index, time.time()))
                #sent_size = sock.send(struct.pack("!iiff", index, token_id, x_acc, y_acc))
                sent_size = sock.send(struct.pack("!iff", token_id, x_acc, y_acc))
                last_sent_time = time.time()
                index += 1
                print "[%03d/%d] Sent ACK(%d), acc (%f, %f)" % (index, loop_length, token_id, x_acc, y_acc)

    sock.close()


def connect(address, port, input_data):
    # connection
    try:
        print "Connecting to (%s, %d).." % (address, port)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.setblocking(True)
        sock.connect((address, port))
    except socket.error, msg:
        sys.stderr.write("Error, %s\n" % msg[1])
        sys.exit(1)

    sender = Thread(target=send_request, args=(sock,input_data))
    recv = Thread(target=recv_data, args=(sock,))

    start_client_time = time.time()
    sender.start()
    recv.start()

    print "Waiting for end of acc data transmit"
    sender.join()
    duration = time.time() - start_client_time

    print "Total Time: %s, Average FPS: %5.2f" % \
            (str(duration), total_recv_number/duration)

def main(argv=None):
    global LOCAL_IPADDRESS
    settings, args = process_command_line(sys.argv[1:])
    input_accs = None
    if settings.input_file and os.path.exists(settings.input_file):
        input_accs = open(settings.input_file, "r").read().split("\n")

    connect(settings.server_address, settings.server_port, input_accs)

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
