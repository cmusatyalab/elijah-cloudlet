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
import threading
import select
from synthesis_protocol import Protocol as protocol
from optparse import OptionParser
from pprint import pprint
from discovery.discovery_api import API
from discovery.discovery_api import Cloudlet
from discovery.discovery_api import Util
from discovery import discovery_api


class RapidClientError(Exception):
    pass


def process_command_line(argv):
    global command_type
    global application_names

    parser = OptionParser(usage="usage: ./cloudlet_client.py -o overlay_path [option]",
            version="Desktop Client for Cloudlet")
    parser.add_option(
            '-o', '--overlay-path', action='store', type='string', dest='overlay_path',
            help="Set overlay path (overlay meta path)")
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server_ip', \
            default=None, help="Set cloudlet server's IP address")
    parser.add_option(
            '-c', '--cloudlet-discover', action='store', type='string', \
            dest='discovery_server', default=None, \
            help="Set cloudlet discovery server address")
    parser.add_option(
            '-d', '--display', action='store_false', dest='display_vnc', default=True,
            help='Turn on VNC display of VM (Default True)')
    parser.add_option(
            '-e', '--early_start', action='store_true', dest='early_start', default=False,
            help='Turn on early start mode for faster application execution (Default False)')
    settings, args = parser.parse_args(argv)

    if settings.overlay_path == None:
        parser.error("Need path to overlay-meta file")
    if (not settings.server_ip) and (not settings.discovery_server):
        message = "You need either specify cloudlet ip(option -s) or enable to discover cloudlet service(option -c)"
        parser.error(message)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    return settings, args


def recv_all(sock, size):
    data = ''
    while len(data) < size:
        data += sock.recv(size - len(data))
    return data


def synthesis(address, port, overlay_path, app_function, synthesis_option):
    if os.path.exists(overlay_path) == False:
        sys.stderr.write("Invalid overlay path: %s\n" % overlay_path)
        sys.exit(1)

    # get session
    cloudlet = Cloudlet(ip_address=address)
    session_id = API.associate_with_cloudlet(cloudlet)
    if session_id == discovery_api.RET_FAILED:
        sys.stderr.write("Cannot create session : ")
        sys.stderr.write(API.discovery_err_str + "\n")
        sys.exit(1)

    start_cloudlet(cloudlet, session_id, overlay_path, app_function, synthesis_option)
    print "finished"


def start_cloudlet(cloudlet, session_id, overlay_meta_path, app_function, synthesis_options=dict()):
    # connection
    start_time = time.time()
    sock = Util.connect(cloudlet)
    if not sock:
        print API.discovery_err_str
        sys.exit(1)

    blob_request_list = list()
    time_dict = dict()
    app_time_dict = dict()

    print "Overlay Meta: %s" % (overlay_meta_path)
    print "Session ID: %ld" % (session_id)
    meta_info = Util.decoding(open(overlay_meta_path, "r").read())

    # send header
    header_dict = {
        protocol.KEY_COMMAND : protocol.MESSAGE_COMMAND_SEND_META,
        protocol.KEY_META_SIZE : os.path.getsize(overlay_meta_path),
        protocol.KEY_SESSION_ID: session_id,
        }
    if len(synthesis_options) > 0:
        header_dict[protocol.KEY_SYNTHESIS_OPTION] = synthesis_options
    header = Util.encoding(header_dict)
    sock.sendall(struct.pack("!I", len(header)))
    sock.sendall(header)
    sock.sendall(open(overlay_meta_path, "r").read())
    time_dict['send_end_time'] = time.time()

    # recv header
    data = recv_all(sock, 4)
    msg_size = struct.unpack("!I", data)[0]
    msg_data = recv_all(sock, msg_size);
    message = Util.decoding(msg_data)
    command = message.get(protocol.KEY_COMMAND, None)
    if command != protocol.MESSAGE_COMMAND_SUCCESS:
        sys.stderr.write("[ERROR] Failed to send overlay meta header\n")
        sys.stderr.write("[ERROR] Reason : %s\n" % message.get(protocol.KEY_FAILED_REASON, None))
        sys.exit(1)

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
                message = Util.decoding(msg_data)
                command = message.get(protocol.KEY_COMMAND)
                if command ==  protocol.MESSAGE_COMMAND_SUCCESS:    # RET_SUCCESS
                    pass
                elif command == protocol.MESSAGE_COMMAND_FAIELD:   # RET_FAIL
                    sys.stderr.write("Synthesis Failed\n")
                if command ==  protocol.MESSAGE_COMMAND_SYNTHESIS_DONE:    # RET_SUCCESS
                    sys.stdout.write("Synthesis SUCCESS\n")
                    time_dict['recv_success_time'] = time.time()
                    #run user input waiting thread 
                    app_thread = client_thread(app_function)
                    app_thread.start()
                elif command == protocol.MESSAGE_COMMAND_ON_DEMAND:    # request blob
                    #sys.stdout.write("Request: %s\n" % (message.get(protocol.KEY_REQUEST_SEGMENT)))
                    blob_request_list.append(str(message.get(protocol.KEY_REQUEST_SEGMENT)))
                else:
                    sys.stderr.write("protocol error:%d\n" % (command))

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
                segment_info = {
                        protocol.KEY_COMMAND : protocol.MESSAGE_COMMAND_SEND_OVERLAY,
                        protocol.KEY_REQUEST_SEGMENT : requested_uri,
                        protocol.KEY_REQUEST_SEGMENT_SIZE : os.path.getsize(blob_path),
                        protocol.KEY_SESSION_ID: session_id,
                        }

                # send close signal to cloudlet server
                header = Util.encoding(segment_info)
                sock.sendall(struct.pack("!I", len(header)))
                sock.sendall(header)
                sock.sendall(open(blob_path, "rb").read())


                if len(sent_blob_list) == total_blob_count:
                    time_dict['send_end_time'] = time.time()

        # check condition
        if (app_thread != None) and (app_thread.isAlive() == False) and (len(sent_blob_list) == total_blob_count):
            break

    app_thread.join()
    time_dict.update(app_thread.time_dict)
    time_dict.update(app_time_dict)

    send_end = time_dict['send_end_time']
    recv_end = time_dict['recv_success_time']
    app_start = time_dict['app_start']
    app_end = time_dict['app_end']
    client_info = {
            protocol.KEY_COMMAND : protocol.MESSAGE_COMMAND_FINISH,
            protocol.KEY_SESSION_ID : session_id,
            'Transfer End':(send_end-start_time), 
            'Synthesis Success': (recv_end-start_time),
            'App Start': (app_start-start_time),
            'App End': (app_end-start_time)
            }
    pprint(client_info)

    # send close signal to cloudlet server
    header = Util.encoding(client_info)
    sock.sendall(struct.pack("!I", len(header)))
    sock.sendall(header)

    # recv finish success (as well as residue) from server
    data = recv_all(sock, 4)
    msg_size = struct.unpack("!I", data)[0]
    msg_data = recv_all(sock, msg_size);
    message = Util.decoding(msg_data)
    command = message.get(protocol.KEY_COMMAND)
    if command != protocol.MESSAGE_COMMAND_SUCCESS:
        raise RapidClientError("finish sucess errror: %d" % command)

    # close session
    if API.disassociate(cloudlet, session_id) == False:
        print API.discovery_err_str


class client_thread(threading.Thread):
    def __init__(self, client_method):
        self.client_method = client_method
        threading.Thread.__init__(self, target=self.start_app)
        self.time_dict = dict()

    def start_app(self):
        self.time_dict['app_start'] = time.time()
        self.client_method()
        self.time_dict['app_end'] = time.time()


def default_app_function():
    while True:
        user_input = raw_input("type 'q' to quit : ")
        if user_input.strip() == 'q':
            break;


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])

    port = 8021
    synthesis_options = dict()
    synthesis_options[protocol.SYNTHESIS_OPTION_DISPLAY_VNC] = settings.display_vnc
    synthesis_options[protocol.SYNTHESIS_OPTION_EARLY_START] = settings.early_start
    app_function = default_app_function
    cloudlet_ip = None
    if settings.server_ip:
        cloudlet_ip = settings.server_ip
    elif settings.discovery_server:
        cloudlet_list = list()
        if discovery_api.RET_FAILED == API.find_nearby_cloudlets(cloudlet_list):
            sys.stderr.write(API.discovery_err_str)
            sys.exit(1)
        for index, each_cloudlet in enumerate(cloudlet_list):
            API.get_cloudlet_info(each_cloudlet)
            print "%d : %s" % (index, each_cloudlet)
        print ""
        while True:
            user_input = raw_input("Choose Cloudlet (0~%d): " % (len(cloudlet_list)-1))
            if not user_input.isdigit():
                continue
            selected_number = int(user_input)
            if 0 <= selected_number < len(cloudlet_list):
                break
        cloudlet_ip = cloudlet_list[selected_number].ip_v4
    else:
        message = "You need either specify cloudlet ip(option -s) or enable to discover cloudlet service(option -c)"
        sys.stderr.write(message)
        sys.exit(1)
        
    synthesis(cloudlet_ip, port, settings.overlay_path, app_function, synthesis_options)
    return 0


if __name__ == "__main__":
    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        is_stop_thread = True
        sys.exit(1)
