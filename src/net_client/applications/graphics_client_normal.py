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

import os
import sys
import socket
from optparse import OptionParser
import time
import struct
from select import select
from threading import Thread
import math

token_id = 0
overlapped_acc_ack = 0
sender_time_stamps = {}
receiver_time_stamps = {}   # recored corresponding receive time for a sent acc
receiver_time_list = []    # All time stamp whenever it received new frame data

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


def recv_data(sock, last_client_id):
    global token_id
    global receiver_time_stamps
    global receiver_time_list
    global overlapped_acc_ack

    # recv
    print "index\tstart\tend\tduration\tjitter\tout"
    # recv initial simulation variable
    while True:
        data = recv_all(sock, 8)
        if not data:
            print "recved data is null"
            time.sleep(0.1)
            continue
        else:
            recv_data = struct.unpack("!II", data)
            #print "container size : (%d %d)" % (recv_data[0], recv_data[1])
            break;

    start_time = time.time()
    while True:
        data = sock.recv(4)
        client_id = struct.unpack("!I", data)[0]
        data = sock.recv(4)
        server_token_id = struct.unpack("!I", data)[0]
        data = sock.recv(4)
        ret_size = struct.unpack("!I", data)[0]
        #print "Client ID : %d, Server_token: %d, Recv size : %d" % (client_id, server_token_id, ret_size)
        token_id = server_token_id
        #if server_token_id%100 == 0:
        #    print "id: %d, FPS: %4.2f" % (token_id, token_id/(time.time()-start_time))

        
        if not ret_size == 0:
            ret_data = recv_all(sock, ret_size)
            recv_time = time.time() * 1000
            if not receiver_time_stamps.get(client_id):
                #print "Add client id to time_stamp list %d" % (client_id)
                receiver_time_stamps[client_id] = recv_time
            else:
                overlapped_acc_ack += 1
            receiver_time_list.append(recv_time)
            if not ret_size == len(ret_data):
                sys.stderr.write("Error, returned value size : %d" % (len(ret_data)))
                sys.exit(1)
        else:
            sys.stderr.write("Error, return size must not be zero")
            sys.exit(1)

        if client_id == last_client_id:
            break




def send_request(sock, input_data):
    global token_id

    # send requests
    if input_data:
        loop_length = len(input_data)
    else:
        loop_length = 1000

    index = 0
    last_sent_time = 0
    try:
        while True:
            read_ready, write_ready, others = select([sock], [sock], [])
            if sock in write_ready:
                if index == loop_length-1:
                    break;

                # send acc data
                if (time.time() - last_sent_time) > 0.020:
                    if len(input_data[index].split("  ")) != 3:
                        print "Error input : %s" % input_data[index]
                        continue
                    x_acc = float(input_data[index].split("  ")[1])
                    y_acc = float(input_data[index].split("  ")[2])

                    sender_time_stamps[index] = time.time()*1000
                    sock.send(struct.pack("!IIff", index, token_id, x_acc, y_acc))
                    last_sent_time = time.time()
                    index += 1
                    #print "[%03d/%d] Sent ACK(%d), acc (%f, %f)" % (index, loop_length, token_id, x_acc, y_acc)
    except Exception, e:
        sock.close()
        sys.exit(1)

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
    recv = Thread(target=recv_data, args=(sock,len(input_data)))

    start_client_time = time.time()
    sender.start()
    recv.start()

    #print "Waiting for end of acc data transmit"
    sender.join()
    recv.join()

    # print result
    prev_duration = -1
    current_duration = -1
    missed_sending_id = 0
    index = 0
    if len(receiver_time_stamps) == 0:
        sys.stderr.write("failed connection")
        sys.exit(1)

    for client_id, start_time in sender_time_stamps.items():
        end_time = receiver_time_stamps.get(client_id)
        if not end_time:
            #sys.stderr.write("Cannot find corresponding end time at %d" % (client_id))
            missed_sending_id += 1
            continue

        prev_duration = current_duration
        current_duration = end_time-start_time
        if prev_duration == -1: # fisrt response
            print "%d\t%014.2f\t%014.2f\t%014.2f\t0\t%s" % (client_id, start_time,\
                    end_time, \
                    end_time-start_time,\
                    "true")
        else:
            print "%d\t%014.2f\t%014.2f\t%014.2f\t%014.2f\t%s" % (client_id, round(start_time, 3), \
                    end_time, \
                    current_duration, \
                    receiver_time_list[index]-receiver_time_list[index-1], \
                    "true")
        index += 1

    # Expect more jitter value if server sent duplicated acc index
    for left_index in xrange(index+1, len(receiver_time_list)):
        print "%d\t%014.2f\t%014.2f\t%014.2f\t%014.2f\t%s" % (left_index, 0, \
                0, \
                0, \
                receiver_time_list[left_index]-receiver_time_list[left_index-1], \
                "true")

    duration = time.time() - start_client_time
    print "Number of missed acc ID (Server only sent lasted acc ID): %d" % (missed_sending_id)
    print "Number of response with duplicated acc id: %d" % (overlapped_acc_ack)
    print "Total Time: %s, Total Recv Frame#: %d, Average FPS: %5.2f" % \
            (str(duration), len(receiver_time_list), len(receiver_time_list)/duration)


def main(argv=None):
    global LOCAL_IPADDRESS
    settings, args = process_command_line(sys.argv[1:])
    input_accs = None
    if settings.input_file and os.path.exists(settings.input_file):
        input_accs = open(settings.input_file, "r").read().split("\n")
    else:
        sys.stderr.write("invalid input file : %s" % settings.input_file)
        return 1

    connect(settings.server_address, settings.server_port, input_accs)

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
