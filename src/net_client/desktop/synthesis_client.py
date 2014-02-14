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

import struct
import socket
import time
import threading
import select
from optparse import OptionParser
from pprint import pprint

# import msgpack
import_msgpack = False
try:
    import msgpack
    import_msgpack = True
except ImportError as e:
    pass
if import_msgpack is False:
    from cloudlet import msgpack as msgpack
    import_msgpack = True


class Protocol(object):
    #
    # Command List "command": command_number
    #
    KEY_COMMAND                 = "command"
    # client -> server
    MESSAGE_COMMAND_SEND_META           = 0x11
    MESSAGE_COMMAND_SEND_OVERLAY        = 0x12
    MESSAGE_COMMAND_FINISH              = 0x13
    MESSAGE_COMMAND_GET_RESOURCE_INFO   = 0x14
    MESSAGE_COMMAND_SESSION_CREATE      = 0x15
    MESSAGE_COMMAND_SESSION_CLOSE       = 0x16
    # server -> client as return
    MESSAGE_COMMAND_SUCCESS             = 0x01
    MESSAGE_COMMAND_FAIELD              = 0x02
    # server -> client as command
    MESSAGE_COMMAND_ON_DEMAND           = 0x03
    MESSAGE_COMMAND_SYNTHESIS_DONE      = 0x04

    #
    # other keys
    #
    KEY_ERROR                   = "error"
    KEY_META_SIZE               = "meta_size"
    KEY_REQUEST_SEGMENT         = "blob_uri"
    KEY_REQUEST_SEGMENT_SIZE    = "blob_size"
    KEY_FAILED_REASON           = "reasons"
    KEY_PAYLOAD                 = "payload"
    KEY_SESSION_ID             = "session_id"
    KEY_REQUESTED_COMMAND       = "requested_command"

    # synthesis option
    KEY_SYNTHESIS_OPTION        = "synthesis_option"
    SYNTHESIS_OPTION_DISPLAY_VNC = "option_display_vnc"
    SYNTHESIS_OPTION_EARLY_START = "option_early_start"
    SYNTHESIS_OPTION_SHOW_STATISTICS = "option_show_statistics"


class ClientError(Exception):
    pass


class Client(object):
    RET_FAILED = 0
    RET_SUCCESS = 1
    CLOUDLET_PORT = 8021

    def __init__(self, ip, port, overlay_path, app_function=None, synthesis_option=dict()):
        self.ip = ip
        self.port = port
        self.overlay_path = overlay_path
        self.app_function = app_function
        self.synthesis_option = synthesis_option or dict()
        self.time_dict = dict()

        if os.path.exists(self.overlay_path) is False:
            msg = "Invalid overlay path: %s\n" % self.overlay_path
            sys.stderr.write(msg)
            raise ClientError(msg)

        # get session
        self.session_id = Client.associate_with_cloudlet(ip, port)
        if self.session_id == Client.RET_FAILED:
            msg = "Cannot create session at Cloudlet (%s:%d)\n" % (ip, port)
            sys.stderr.write(msg)
            raise ClientError(msg)


    def provisioning(self):
        # connection
        self.start_provisioning_time = time.time()
        sock = Client.connect(self.ip, self.port)
        if not sock:
            msg = "Cannot connect to Cloudlet (%s:%d)\n" % (self.ip, self.port)
            sys.stderr.write(msg)
            raise ClientError(msg)

        blob_request_list = list()

        sys.stdout.write("Overlay Meta: %s\n" % (self.overlay_path))
        sys.stdout.write("Session ID: %ld\n" % (self.session_id))
        meta_info = Client.decoding(open(self.overlay_path, "r").read())

        # send header
        header_dict = {
            Protocol.KEY_COMMAND : Protocol.MESSAGE_COMMAND_SEND_META,
            Protocol.KEY_META_SIZE : os.path.getsize(self.overlay_path),
            Protocol.KEY_SESSION_ID: self.session_id,
            }
        if len(self.synthesis_option) > 0:
            header_dict[Protocol.KEY_SYNTHESIS_OPTION] = self.synthesis_option
        header = Client.encoding(header_dict)
        sock.sendall(struct.pack("!I", len(header)))
        sock.sendall(header)
        sock.sendall(open(self.overlay_path, "r").read())
        self.time_dict['send_end_time'] = time.time()

        # recv header
        data = Client.recv_all(sock, 4)
        msg_size = struct.unpack("!I", data)[0]
        msg_data = Client.recv_all(sock, msg_size)
        message = Client.decoding(msg_data)
        command = message.get(Protocol.KEY_COMMAND, None)
        if command != Protocol.MESSAGE_COMMAND_SUCCESS:
            msg = "Failed to send overlay meta header: %s" %\
                    message.get(Protocol.KEY_FAILED_REASON, None)
            sys.stderr.write("[ERROR] %s\n" % msg)
            raise ClientError(msg)

        self.app_thread = None
        total_blob_count = len(meta_info['overlay_files'])
        sent_blob_list = list()
        is_synthesis_finished = False

        while True:
            inputready, outputready, exceptrdy = select.select([sock], [sock],
                    [], 0.01)
            for i in inputready:
                if i == sock:
                    data = sock.recv(4)
                    if not data:
                        break
                    msg_size = struct.unpack("!I", data)[0]
                    msg_data = Client.recv_all(sock, msg_size)
                    message = Client.decoding(msg_data)
                    command = message.get(Protocol.KEY_COMMAND)
                    if command == Protocol.MESSAGE_COMMAND_SUCCESS:
                        # RET_SUCCESS
                        pass
                    elif command == Protocol.MESSAGE_COMMAND_FAIELD:
                        # RET_FAIL
                        sys.stderr.write("Synthesis Failed\n")

                    if command ==  Protocol.MESSAGE_COMMAND_SYNTHESIS_DONE:
                        # RET_SUCCESS
                        sys.stdout.write("Synthesis SUCCESS\n")
                        self.time_dict['recv_success_time'] = time.time()
                        is_synthesis_finished = True
                        #run user input waiting thread
                        if self.app_function is not None:
                            self.app_thread = ApplicationThread(self.app_function)
                            self.app_thread.start()
                    elif command == Protocol.MESSAGE_COMMAND_ON_DEMAND:
                        # request blob
                        #sys.stdout.write("Request: %s\n" % (message.get(Protocol.KEY_REQUEST_SEGMENT)))
                        blob_request_list.append(str(message.get(Protocol.KEY_REQUEST_SEGMENT)))
                    else:
                        sys.stderr.write("Protocol error:%d\n" % (command))

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
                        raise ClientError("sending duplicated blob: %s" % requested_uri)

                    filename = os.path.basename(requested_uri)
                    blob_path = os.path.join(os.path.dirname(self.overlay_path), filename)
                    segment_info = {
                            Protocol.KEY_COMMAND : Protocol.MESSAGE_COMMAND_SEND_OVERLAY,
                            Protocol.KEY_REQUEST_SEGMENT : requested_uri,
                            Protocol.KEY_REQUEST_SEGMENT_SIZE : os.path.getsize(blob_path),
                            Protocol.KEY_SESSION_ID: self.session_id,
                            }

                    # send close signal to cloudlet server
                    header = Client.encoding(segment_info)
                    sock.sendall(struct.pack("!I", len(header)))
                    sock.sendall(header)
                    sock.sendall(open(blob_path, "rb").read())

                    if len(sent_blob_list) == total_blob_count:
                        self.time_dict['send_end_time'] = time.time()

            # check condition
            if (len(sent_blob_list) == total_blob_count):
                # sent all vm overlay
                if (self.app_function is not None):
                    # wait until application finishes
                    if (self.app_thread is not None) and (self.app_thread.isAlive() is False):
                        break
                else:
                    # No application is instantiated
                    # wait until synthesis done
                    if is_synthesis_finished == True:
                        break


    def terminate(self):
        sock = Client.connect(self.ip, self.port)
        if not sock:
            msg = "Cannot connect to Cloudlet (%s:%d)\n" % (self.ip, self.port)
            sys.stderr.write(msg)
            raise ClientError(msg)

        if self.app_function is not None and self.app_thread is not None:
            self.app_thread.join()
            self.time_dict.update(self.app_thread.time_dict)

        send_end = self.time_dict['send_end_time']
        recv_end = self.time_dict['recv_success_time']
        app_start = self.time_dict['app_start']
        app_end = self.time_dict['app_end']
        client_info = {
                Protocol.KEY_COMMAND : Protocol.MESSAGE_COMMAND_FINISH,
                Protocol.KEY_SESSION_ID : self.session_id,
                'Transfer End':(send_end-self.start_provisioning_time),
                'Synthesis Success': (recv_end-self.start_provisioning_time),
                'App Start': (app_start-self.start_provisioning_time),
                'App End': (app_end-self.start_provisioning_time)
                }
        pprint(client_info)

        # send close signal to cloudlet server
        header = Client.encoding(client_info)
        sock.sendall(struct.pack("!I", len(header)))
        sock.sendall(header)

        # recv finish success (as well as residue) from server
        data = Client.recv_all(sock, 4)
        msg_size = struct.unpack("!I", data)[0]
        msg_data = Client.recv_all(sock, msg_size)
        message = Client.decoding(msg_data)
        command = message.get(Protocol.KEY_COMMAND)
        if command != Protocol.MESSAGE_COMMAND_SUCCESS:
            raise ClientError("finish success error: %d" % command)

        # close session
        if Client.disassociate(self.ip, self.port, self.session_id) is False:
            print "Failed to close session"


    @staticmethod
    def connect(ip, port):
        if not ip:
            return None
        try:
            address = (ip, port)
            time_out = 10
            #print "Connecting to (%s).." % str(address)
            sock = socket.create_connection(address, time_out)
            sock.setblocking(True)
        except socket.error as msg:
            return None
        return sock

    @staticmethod
    def recv_all(sock, size):
        data = ''
        while len(data) < size:
            data += sock.recv(size - len(data))
        return data

    @staticmethod
    def encoding(data):
        return msgpack.packb(data)

    @staticmethod
    def decoding(data):
        return msgpack.unpackb(data)

    @staticmethod
    def associate_with_cloudlet(ip, port):
        '''
        :param cloudlet_t: cloudlet_t instance that has ip_address of the cloudlet
        :type cloudlet_t: cloudlet_t

        :return: session id or -1 if it failed
        :rtype: long
        '''
        sock = Client.connect(ip, port)
        if not sock:
            return Client.RET_FAILED

        # send request
        header_dict = {
            Protocol.KEY_COMMAND: Protocol.MESSAGE_COMMAND_SESSION_CREATE
            }
        header = Client.encoding(header_dict)
        sock.sendall(struct.pack("!I", len(header)))
        sock.sendall(header)

        # recv response
        recv_size, = struct.unpack("!I", Client.recv_all(sock, 4))
        data = Client.recv_all(sock, recv_size)
        data = Client.decoding(data)
        session_id = data.get(Protocol.KEY_SESSION_ID, None)
        if not session_id:
            session_id = Client.RET_FAILED
            reason = data.get(Protocol.KEY_FAILED_REASON, None)
            msg = "Cannot create session: %s" % str(reason)
            raise ClientError(msg)
        sock.close()
        return session_id

    @staticmethod
    def disassociate(ip, port, session_id):
        '''
        :param session_id: session_id that was returned when associated
        :type session_id: long

        :return: N/A
        '''
        sock = Client.connect(ip, port)
        if not sock:
            return Client.RET_FAILED

        # send request
        header_dict = {
            Protocol.KEY_COMMAND: Protocol.MESSAGE_COMMAND_SESSION_CLOSE,
            Protocol.KEY_SESSION_ID: session_id,
            }
        header = Client.encoding(header_dict)
        sock.sendall(struct.pack("!I", len(header)))
        sock.sendall(header)

        # recv response
        recv_size, = struct.unpack("!I", Client.recv_all(sock, 4))
        data = Client.recv_all(sock, recv_size)
        data = Client.decoding(data)

        sock.close()
        is_success = data.get(Protocol.KEY_COMMAND, False)
        if is_success == Protocol.MESSAGE_COMMAND_SUCCESS:
            return Client.RET_SUCCESS
        return Client.RET_FAILED


class ApplicationThread(threading.Thread):
    def __init__(self, client_method):
        self.client_method = client_method
        threading.Thread.__init__(self, target=self.start_app)
        self.time_dict = dict()

    def start_app(self):
        self.time_dict['app_start'] = time.time()
        self.client_method()
        self.time_dict['app_end'] = time.time()


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
            '-d', '--display', action='store_false', dest='display_vnc', default=True,
            help='Turn on VNC display of VM (Default True)')
    parser.add_option(
            '-e', '--early_start', action='store_true', dest='early_start', default=False,
            help='Turn on early start mode for faster application execution (Default False)')
    settings, args = parser.parse_args(argv)

    if settings.overlay_path == None:
        parser.error("Need path to overlay-meta file")
    if (not settings.server_ip):
        message = "You need to specify Cloudlet ip(option -s)"
        parser.error(message)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    return settings, args


def default_app_function():
    while True:
        user_input = raw_input("type 'q' to quit : ")
        if user_input.strip() == 'q':
            break;


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])

    port = 8021
    synthesis_option = dict()
    synthesis_option[Protocol.SYNTHESIS_OPTION_DISPLAY_VNC] = settings.display_vnc
    synthesis_option[Protocol.SYNTHESIS_OPTION_EARLY_START] = settings.early_start
    cloudlet_ip = None
    if settings.server_ip:
        cloudlet_ip = settings.server_ip
    else:
        message = "You need to specify cloudlet ip(option -s)"
        sys.stderr.write(message)
        sys.exit(1)

    client = Client(cloudlet_ip, port, settings.overlay_path,\
            app_function=None, synthesis_option=synthesis_option)
    try:
        client.provisioning()
        sys.stdout.write("SUCCESS in Provisioning\n")
    except ClientError as e:
        sys.stderr.write(str(e))
    return 0


if __name__ == "__main__":
    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        is_stop_thread = True
        sys.exit(1)
