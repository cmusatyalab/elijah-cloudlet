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

import sys
import os
import struct
import time
import socket
import pprint
import msgpack
from protocol import Protocol as protocol
from Const import Const as Const


CLOUDLET_DOMAIN = "findcloudlet.org"
CLOUDLET_PORT = 8021
RET_FAILED  = 0
RET_SUCCESS = 1


class CloudletFindError(Exception):
    pass


class CloudletResourceStatic(object):
    def __init__(self):
        self.number_cpu = None
        self.cpu_clock_speed_mhz = None
        self.mem_total_mb = None

    def __repr__(self):
        return pprint.pformat(self.__dict__)

    def __str__(self):
        return pprint.pformat(self.__dict__)

    def update(self, data):
        self.number_cpu = data.get(Const.MACHINE_NUMBER_TOTAL_CPU, None)
        self.cpu_clock_speed_ghz = data.get(Const.MACHINE_CLOCK_SPEED, None)
        self.mem_total_mb = data.get(Const.MACHINE_MEM_TOTAL, None)


class CloudletResourceDynamic(object):
    def __init__(self):
        self.mem_free_mb = None
        self.cpu_usage_percent = None

    def __repr__(self):
        return pprint.pformat(self.__dict__)

    def __str__(self):
        return pprint.pformat(self.__dict__)

    def update(self, data):
        self.cpu_usage_percent = data.get(Const.TOTAL_CPU_USE_PERCENT, None)
        self.mem_free_mb = data.get(Const.TOTAL_FREE_MEMORY, None)


class Cloudlet(object):
    def __init__(self, ip_address=None):
        self.ip_v4 = ip_address
        self.port_number = CLOUDLET_PORT
        self.resource_static = CloudletResourceStatic()
        self.resource_dynamic = CloudletResourceDynamic()

    def __str__(self):
        ret_str = pprint.pformat(self.__dict__)
        return ret_str

    def update(self, data):
        self.resource_static.update(data)
        self.resource_dynamic.update(data)


class Util(object):
    @staticmethod
    def connect(cloudlet_t):
        if not cloudlet_t.ip_v4:
            API.discovery_err_str = "No IP is specified"
            return None
        try:
            address = (cloudlet_t.ip_v4, CLOUDLET_PORT)
            time_out = 10
            #print "Connecting to (%s).." % str(address)
            sock = socket.create_connection(address, time_out)
            sock.setblocking(True)
        except socket.error, msg:
            API.discovery_err_str = str(msg) + ("\nCannot connect to (%s)\n" % str(address))
            return None
        return sock

    @staticmethod
    def recvall(sock, size):
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


class API(object):
    discovery_err_str = None

    @staticmethod
    def find_nearby_cloudlets(cloudlet_list_ret):
        '''Step 1. Find several candidate using DNS
        :param cloudlet_list_ret: cloudlet_t objects will be returned at this list
        :type cloudlet_list_ret: list

        :return: success/fail
        :rtype: int
        '''
        addr_list = socket.getaddrinfo(CLOUDLET_DOMAIN, 80, socket.AF_INET, 0, socket.IPPROTO_TCP)
        if addr_list == None or len(addr_list) == 0:
            API.discovery_err_str = "No Available Cloudlet"
            return RET_FAILED
        for each_addr in addr_list:
            new_cloudlet = Cloudlet(ip_address=each_addr[-1][0])
            cloudlet_list_ret.append(new_cloudlet)
        return RET_SUCCESS

    @staticmethod
    def get_cloudlet_info(cloudlet_t):
        '''Step 1. fill out cloudlet_t field with more detailed information
        :param cloudlet_t: cloudlet_t instance that has ip_address of the cloudlet
        :type cloudlet_t: cloudlet_t

        :return: success/fail
        :rtype: int
        '''
        sock = Util.connect(cloudlet_t)
        if not sock:
            API.discovery_err_str = 'Failed to connect'
            return RET_FAILED

        # send request
        header_dict = {
            protocol.KEY_COMMAND : protocol.MESSAGE_COMMAND_GET_RESOURCE_INFO,
            }
        header = Util.encoding(header_dict)
        sock.sendall(struct.pack("!I", len(header)))
        sock.sendall(header)

        # recv response
        recv_size, = struct.unpack("!I", Util.recvall(sock, 4))
        data = Util.recvall(sock, recv_size)
        data = Util.decoding(data)
        sock.close()
        #import pdb;pdb.set_trace()
        if data.get(protocol.KEY_COMMAND) == protocol.MESSAGE_COMMAND_SUCCESS:
            cloudlet_t.update(data.get(protocol.KEY_PAYLOAD, dict()))
            return RET_SUCCESS
        else:
            API.discovery_err_str = data.get(protocol.KEY_FAILED_REASON, None)
            return RET_FAILED

    @staticmethod
    def associate_with_cloudlet(cloudlet_t):
        '''Step 2. Associate with given cloudlet
        :param cloudlet_t: cloudlet_t instance that has ip_address of the cloudlet
        :type cloudlet_t: cloudlet_t

        :return: session id or -1 if it failed
        :rtype: long
        '''
        sock = Util.connect(cloudlet_t)
        if not sock:
            API.discovery_err_str = "Failed to connect"
            return RET_FAILED

        # send request
        header_dict = {
            protocol.KEY_COMMAND : protocol.MESSAGE_COMMAND_SESSION_CREATE
            }
        header = Util.encoding(header_dict)
        sock.sendall(struct.pack("!I", len(header)))
        sock.sendall(header)

        # recv response
        recv_size, = struct.unpack("!I", Util.recvall(sock, 4))
        data = Util.recvall(sock, recv_size)
        data = Util.decoding(data)
        session_id = data.get(protocol.KEY_SESSIOIN_ID, None)
        if not session_id:
            reason = data.get(protocol.KEY_FAILED_REASON, None)
            if reason:
                API.discovery_err_str = str(reason)
            else:
                API.discovery_err_str = "Cannot create session"
        sock.close()
        return session_id

    @staticmethod
    def disassociate(cloudlet_t, session_id):
        '''Step 2. disassociate with given cloudlet
        :param session_id: session_id that was returned when associated
        :type session_id: long

        :return: N/A
        '''
        sock = Util.connect(cloudlet_t)
        if not sock:
            API.discovery_err_str = "Failed to connect"
            return RET_FAILED

        # send request
        header_dict = {
            protocol.KEY_COMMAND : protocol.MESSAGE_COMMAND_SESSION_CLOSE,
            protocol.KEY_SESSIOIN_ID: session_id,
            }
        header = Util.encoding(header_dict)
        sock.sendall(struct.pack("!I", len(header)))
        sock.sendall(header)

        # recv response
        recv_size, = struct.unpack("!I", Util.recvall(sock, 4))
        data = Util.recvall(sock, recv_size)
        data = Util.decoding(data)

        sock.close()
        ret_session_id = data.get(protocol.KEY_SESSIOIN_ID, None)
        if session_id == ret_session_id:
            return RET_SUCCESS
        
        API.discovery_err_str = "Failed to connect"
        return RET_FAILED


    @staticmethod
    def get_cost_of_cache_state_by_filename(session_id, file_names):
        '''Step 3. get the expected cost of warnming the cache
        :param session_id: session id
        :type session_id: long
        :param file_names: list of file name that will be used for running application
        :type file_names: list of string

        :return: expected cost. Less is better
        :rtype: long
        '''
        pass

    @staticmethod
    def get_cost_of_cache_state_by_hash(session_id, hash_list):
        '''Step 3. get the expected cost of warnming the cache
        :param session_id: session id
        :type session_id: long
        :param hash_list: list of hash that will be used for running application
        :type hash_list: list of hash

        :return: expected cost. Less is better
        :rtype: long
        '''
        pass

def _wait_for_next():
    print ""
    raw_input("")

def main(argv):

    cloudlet_list = list()
    print "Step 1. Get nearby Cloudlet from DNS"
    if RET_SUCCESS == API.find_nearby_cloudlets(cloudlet_list):
        for each_cloudlet in cloudlet_list:
            print each_cloudlet
    else:
        print API.discovery_err_str
        sys.exit(1)

    _wait_for_next()
    print "Step 2. get cloudlet info"
    each_cloudlet = cloudlet_list[0]
    if RET_SUCCESS == API.get_cloudlet_info(each_cloudlet):
        print each_cloudlet
    else:
        print API.discovery_err_str
        sys.exit(1)

    _wait_for_next()
    print "Step 3. get session ID"
    session_id = API.associate_with_cloudlet(each_cloudlet)
    if session_id != RET_FAILED:
        print "Session ID: %d\n" % (session_id)
    else:
        print API.discovery_err_str
        sys.exit(1)

    _wait_for_next()
    print "Step 4. disassociate"
    ret = API.disassociate(each_cloudlet, session_id)
    if ret:
        print "Seesion(%d) is closed" % session_id
    else:
        print "Seesion(%d) is failed to close" % session_id

    return 0


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
