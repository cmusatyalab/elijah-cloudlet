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
import json
import time
from optparse import OptionParser
from threading import Thread
from pprint import pprint
import cloudlet_client
import select

application = ['moped', 'face', 'mar', 'speech', 'graphics']

batch_mode = False


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

    parser = OptionParser(usage="usage: ./cloudlet_client.py [option]",\
            version="Desktop Cloudlet Client")
    parser.add_option(
            '-a', '--app', action='store', type='string', dest='application',
            help="Set base VM name")
    parser.add_option(
            '-b', '--batch', action='store_true', dest='batch', default=False,
            help='Automatic exit triggered by client')
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server_ip',
            help="Set cloudlet server's IP address")
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))

    if not settings.application:
        parser.error("Need application among [%s]" % ('|'.join(application)))
    
    return settings, args


def recv_all(sock, size):
    data = ''
    while len(data) < size:
        data += sock.recv(size - len(data))
    return data


def synthesis(address, port, application, batch=False, wait_time=0):
    global batch_mode
    batch_mode = batch
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

    start_cloudlet(sock, application, start_time)
    print "finished"
    time.sleep(wait_time)


def start_cloudlet(sock, application, start_time):
    global batch_mode
    blob_request_list = list()

    time_dict = dict()
    app_time_dict = dict()

    overlay_meta_path = None
    if application == 'moped':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/moped/overlay-meta'
    elif application == 'moped_random':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/moped_random/overlay-meta'
    elif application == 'mar':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/mar/overlay-meta'
    elif application == 'speech':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/speech/overlay-meta'
    elif application == 'speech_random':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/speech_random/overlay-meta'
    elif application == 'face':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/face/overlay-meta'
    elif application == 'graphics':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/graphics/overlay-meta'

    if len(application.split("_")) > 1:
        app_name, order, blob_size = application.split("_")
        overlay_meta_path = "/home/krha/cloudlet/image/overlay/%s/%s/%s/overlay-meta" % \
                (app_name, order, blob_size)

    if overlay_meta_path == None:
        raise Exception("NO valid application name: %s" % application)

    print "Overlay Meta: %s" % (overlay_meta_path)
    # modify overlay path
    meta_info = msgpack.unpackb(open(overlay_meta_path, "r").read())
    for blob in meta_info['overlay_files']:
        filename = os.path.basename(blob['overlay_name'])
        uri = "%s/%s" % (application, filename)
        blob['overlay_name'] = uri

    # send header
    header = msgpack.packb(meta_info)
    sock.sendall(struct.pack("!I", len(header)))
    sock.sendall(header)
    time_dict['send_end_time'] = time.time()

    application_name = application.split("_")[0]
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
                ret_size = struct.unpack("!I", data)[0]
                ret_data = recv_all(sock, ret_size);
                json_ret = json.loads(ret_data)
                command = json_ret.get("command")
                if command ==  0x01:    # RET_SUCCESS
                    print "Synthesis SUCCESS"
                    time_dict['recv_success_time'] = time.time()
                    #run application thread
                    print "app started"
                    app_thread = Thread(target=application_thread, args=(sock, application_name, app_time_dict))
                    app_thread.start()
                elif command == 0x02:   # RET_FAIL
                    print "Synthesis Failed"
                elif command == 0x03:    # request blob
                    #print "Request: %s" % (json_ret.get("blob_uri"))
                    blob_request_list.append(str(json_ret.get("blob_uri")))
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
    app_start_time = time_dict['app_start']
    app_end_time = time_dict['app_end']
    client_info = {'Transfer':(send_end-start_time), \
            'Response': (recv_end-start_time), \
            'App end': (app_end_time-start_time), \
            'App run': (app_end_time-app_start_time)}
    pprint(client_info)

    header = msgpack.packb(client_info)
    sock.sendall(struct.pack("!I", len(header)))
    sock.sendall(header)


def application_thread(sock, application, time_dict):
    global batch_mode
    time_dict['app_start'] = time.time()
    cloudlet_client.run_application(application)
    time_dict['app_end'] = time.time()
    print "Application Finished"

    if batch_mode == False:
        # wait for user input to quit
        while True:
            user_input = raw_input("type 'q' to quit : ")
            if user_input.strip() == 'q':
                break;


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    if settings.server_ip:
        cloudlet_server_ip = settings.server_ip
    else:
        cloudlet_server_ip = "cloudlet.krha.kr"

    cloudlet_server_port = 8021
    synthesis(cloudlet_server_ip, cloudlet_server_port, settings.application, batch=settings.batch)


if __name__ == "__main__":
    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        is_stop_thread = True
        sys.exit(1)
