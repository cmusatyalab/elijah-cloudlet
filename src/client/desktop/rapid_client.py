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
import msgpack
import struct
import sys
import socket
import time
from synthesis_protocol import Protocol as protocol
from optparse import OptionParser
from threading import Thread
from pprint import pprint
import select

class RapidClientError(Exception):
    pass

class BlobHeader(object):
    def __init__(self, blob_uri, blob_size):
        self.blob_uri = blob_uri
        self.blob_size = blob_size

    def get_serialized(self):
        # blob_size         :   unsigned int
        # blob_name         :   unsigned short
        # blob_name_size    :   variable string
        data = struct.pack("!IH%ds" % len(self.blob_uri), \
                self.blob_size, len(self.blob_uri), self.blob_uri)
        return data


def process_command_line(argv):
    global command_type
    global application_names

    parser = OptionParser(usage="usage: ./cloudlet_client.py -o overlay_path -s cloudlet_server_ip [option]",
            version="Desktop Client for Cloudlet")
    parser.add_option(
            '-o', '--overlay-path', action='store', type='string', dest='overlay_path',
            help="Set overlay path (overlay meta path)")
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server_ip',
            help="Set cloudlet server's IP address")
    settings, args = parser.parse_args(argv)

    if settings.overlay_path == None:
        parser.error("Need path to overlay-meta file")
    if settings.server_ip == None:
        parser.error("Need Cloudlet's server IP")
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    return settings, args


def recv_all(sock, size):
    data = ''
    while len(data) < size:
        data += sock.recv(size - len(data))
    return data


def synthesis(address, port, overlay_path, wait_time=0):
    if os.path.exists(overlay_path) == False:
        sys.stderr.write("Invalid overlay path: %s\n" % overlay_path)
        sys.exit(1)

    # connection
    start_time = time.time()
    try:
        print "Connecting to (%s, %d).." % (address, port)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        #sock.setblocking(True)
        sock.settimeout(10)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        sock.connect((address, port))
    except socket.error, msg:
        sys.stderr.write("Error, %s\n" % msg)
        sys.exit(1)

    start_cloudlet(sock, overlay_path, start_time)
    print "finished"
    time.sleep(wait_time)


def start_cloudlet(sock, overlay_meta_path, start_time):
    blob_request_list = list()

    time_dict = dict()
    app_time_dict = dict()

    print "Overlay Meta: %s" % (overlay_meta_path)

    # modify overlay path
    meta_info = msgpack.unpackb(open(overlay_meta_path, "r").read())
    '''
    for blob in meta_info['overlay_files']:
        filename = os.path.basename(blob['overlay_name'])
        uri = "%s" % (filename)
        blob['overlay_name'] = uri
    '''

    # send header
    header = msgpack.packb({
        protocol.KEY_COMMAND : protocol.MESSAGE_COMMAND_SEND_META,
        protocol.KEY_META_SIZE : os.path.getsize(overlay_meta_path)
        })
    #import pdb; pdb.set_trace()
    sock.sendall(struct.pack("!I", len(header)))
    sock.sendall(header)
    sock.sendall(open(overlay_meta_path, "r").read())
    time_dict['send_end_time'] = time.time()

    app_thread = None
    total_blob_count = len(meta_info['overlay_files'])
    sent_blob_list = list()

    while True:
        inputready, outputready, exceptrdy = select.select([sock], [sock], [], 0.01)
        for i in inputready:
            if i == sock:
                data = sock.recv(4)
                if not data:
                    break
                msg_size = struct.unpack("!I", data)[0]
                msg_data = recv_all(sock, msg_size);
                message = msgpack.unpackb(msg_data)
                command = message.get(protocol.KEY_COMMAND)
                if command ==  protocol.MESSAGE_COMMAND_SUCCESS:    # RET_SUCCESS
                    print "Synthesis SUCCESS"
                    time_dict['recv_success_time'] = time.time()
                    #run user input waiting thread 
                    app_thread = Thread(target=application_thread, args=(sock, app_time_dict))
                    app_thread.start()
                elif command == protocol.MESSAGE_COMMAND_FAIELD:   # RET_FAIL
                    print "Synthesis Failed"
                elif command == protocol.MESSAGE_COMMAND_ON_DEMAND:    # request blob
                    print "Request: %s" % (message.get(protocol.KEY_REQUEST_SEGMENT))
                    blob_request_list.append(str(message.get(protocol.KEY_REQUEST_SEGMENT)))
                else:
                    print "protocol error:%d" % (command)

        # send data
        for i in outputready:
            if (i == sock) and (len(sent_blob_list) < total_blob_count):
                # check request list
                requested_uri = None
                if len(blob_request_list) == 0:
                    continue

                requested_uri = blob_request_list.pop(0)
                if requested_uri not in sent_blob_list:
                    sent_blob_list.append(requested_uri)
                else:
                    raise RapidClientError("sending duplicated blob: %s" % requested_uri)

                filename = os.path.basename(requested_uri)
                blob_path = os.path.join(os.path.dirname(overlay_meta_path), filename)
                blob_data = open(blob_path, "rb").read()
                blob_header = BlobHeader(requested_uri, os.path.getsize(blob_path))

                sock.sendall(blob_header.get_serialized())
                sock.sendall(blob_data)

                if len(sent_blob_list) == total_blob_count:
                    time_dict['send_end_time'] = time.time()

        # check condition
        if (app_thread != None) and (app_thread.isAlive() == False) and (len(sent_blob_list) == total_blob_count):
            break

    app_thread.join()
    time_dict.update(app_time_dict)

    send_end = time_dict['send_end_time']
    recv_end = time_dict['recv_success_time']
    client_info = {'Transfer':(send_end-start_time), \
            'Synthesis Success': (recv_end-start_time)}
    pprint(client_info)

    # send close signal to cloudlet server
    header = msgpack.packb(client_info)
    sock.sendall(struct.pack("!I", len(header)))
    sock.sendall(header)


def application_thread(sock, time_dict):
    time_dict['app_start'] = time.time()
    while True:
        user_input = raw_input("type 'q' to quit : ")
        if user_input.strip() == 'q':
            break;
    time_dict['app_end'] = time.time()


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])

    port = 8021
    synthesis(settings.server_ip, port, settings.overlay_path)


if __name__ == "__main__":
    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        is_stop_thread = True
        sys.exit(1)
