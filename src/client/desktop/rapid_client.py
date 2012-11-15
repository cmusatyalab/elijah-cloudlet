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
import cloudlet_client

application = ['moped', 'moped_random', 'face', 'mar', 'speech']

def process_command_line(argv):
    global command_type
    global application_names

    parser = OptionParser(usage="usage: ./cloudlet_client.py [option]",\
            version="Desktop Cloudlet Client")
    parser.add_option(
            '-a', '--app', action='store', type='string', dest='application',
            help="Set base VM name")
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


def synthesis(address, port, application):
    # connection
    start_time = time.time()
    try:
        print "Connecting to (%s, %d).." % (address, port)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setblocking(True)
        sock.connect((address, port))
    except socket.error, msg:
        sys.stderr.write("Error, %s\n" % msg)
        sys.exit(1)

    send_end_time = {}
    recv_end_time = {}
    sender = Thread(target=send_thread, args=(sock, application, send_end_time))
    recv = Thread(target=recv_thread, args=(sock, application, recv_end_time))

    sender.start()
    recv.start()

    print "Waiting for Thread joinining"
    sender.join()
    recv.join()
    send_end = send_end_time['time']
    recv_end = recv_end_time['time']
    print "Transfer %f-%f = %f" % (send_end, start_time, (send_end-start_time))
    print "Response %f-%f = %f" % (recv_end, start_time, (recv_end-start_time))
    app_start_time = recv_end_time['app_start']
    app_end_time = recv_end_time['app_end']
    print "App End  %f-%f = %f" % (app_end_time, start_time, (app_end_time-start_time))
    print "App      %f-%f = %f" % (app_end_time, app_start_time, (app_end_time-app_start_time))


def send_thread(sock, application, time_dict):
    if application == 'moped':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/moped/overlay-meta'
    elif application == 'moped_random':
        # moped + random 100MB file
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/moped_random/overlay-meta'
    elif application == 'mar':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/mar/overlay-meta'
    elif application == 'speech':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/speech/overlay-meta'
    elif application == 'face':
        overlay_meta_path = '/home/krha/cloudlet/image/overlay/face/overlay-meta'
    else:
        raise Exception("NO valid application name: %s" % application)

    # modify overlay path
    local_ip = sock.getsockname()[0]
    meta_info = msgpack.unpackb(open(overlay_meta_path, "r").read())
    for blob in meta_info['overlay_files']:
        filename = os.path.basename(blob['overlay_name'])
        url = "http://%s/overlay/%s/%s" % (local_ip, application, filename)
        blob['overlay_name'] = url

    # send header
    header = msgpack.packb(meta_info)
    sock.sendall(struct.pack("!I", len(header)))
    sock.sendall(header)
    time_dict['time'] = time.time()


def recv_thread(sock, application, time_dict):
    if application == "moped_random":
        application = "moped"

    #recv
    data = sock.recv(4)
    ret_size = struct.unpack("!I", data)[0]
    ret_data = recv_all(sock, ret_size);
    json_ret = json.loads(ret_data)
    ret_value = json_ret['return']
    print ret_value
    if ret_value != "SUCCESS":
        print "Synthesis Failed"
    time_dict['time'] = time.time()

    #run application
    time_dict['app_start'] = time.time()
    cloudlet_client.run_application(application)
    time_dict['app_end'] = time.time()


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    if settings.server_ip:
        cloudlet_server_ip = settings.server_ip
    else:
        cloudlet_server_ip = "cloudlet.krha.kr"
    cloudlet_server_port = 8021
    synthesis(cloudlet_server_ip, cloudlet_server_port, settings.application)


if __name__ == "__main__":
    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        is_stop_thread = True
        sys.exit(1)
