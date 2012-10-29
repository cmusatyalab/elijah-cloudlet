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
import json
import struct
import sys
import socket
from optparse import OptionParser

def process_command_line(argv):
    global command_type
    global application_names

    parser = OptionParser(usage="usage: ./cloudlet_client.py [option]",\
            version="Desktop Cloudlet Client")
    parser.add_option(
            '-b', '--base', action='store', type='string', dest='base',
            help="Set base VM name")
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server_ip',
            help="Set cloudlet server's IP address")
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    if not settings.base:
        parser.error("Need base VM name")

    return settings, args


def recv_all(sock, size):
    data = ''
    while len(data) < size:
        data += sock.recv(size - len(data))
    return data


def synthesis(address, port, base_name):
    overlay_disk_path = '/home/krha/cloudlet/image/ubuntu-12.04.1-server-i386/precise.overlay-img.lzma'
    overlay_mem_path = '/home/krha/cloudlet/image/ubuntu-12.04.1-server-i386/precise.overlay-mem.lzma'
    overlay_disk_size = os.path.getsize(overlay_disk_path)
    overlay_mem_size = os.path.getsize(overlay_mem_path)

    json_str = {"command":33, \
            "protocol-version": "1.0", \
            "VM":[{ \
                "overlay_name":'test', \
                "memory_snapshot_path": overlay_mem_path, \
                "memory_snapshot_size": overlay_mem_size, \
                "diskimg_path": overlay_disk_path, \
                "diskimg_size": overlay_disk_size, \
                "base_name": base_name
                }],\
            "Request_synthesis_core":"4" \
            }
    print json.dumps(json_str, indent=4)

    # connection
    try:
        print "Connecting to (%s, %d).." % (address, port)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setblocking(True)
        sock.connect((address, port))
    except socket.error, msg:
        sys.stderr.write("Error, %s\n" % msg)
        sys.exit(1)

    # send header
    json_data = json.dumps(json_str)
    sock.sendall(struct.pack("!I", len(json_data)))
    sock.sendall(json_data)

    # send data
    mem_data = open(overlay_mem_path, "rb").read()
    sock.sendall(mem_data)
    disk_data = open(overlay_disk_path, "rb").read()
    sock.sendall(disk_data)
    
    #recv
    data = sock.recv(4)
    ret_size = struct.unpack("!I", data)[0]
    ret_data = recv_all(sock, ret_size);
    json_ret = json.loads(ret_data)
    ret_value = json_ret['return']
    print ret_value
    if ret_value != "SUCCESS":
        print "Synthesis Failed"
        sys.exit(1)
    return 0


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    if settings.server_ip:
        cloudlet_server_ip = settings.server_ip
    else:
        cloudlet_server_ip = "cloudlet.krha.kr"
    cloudlet_server_port = 8021
    synthesis(cloudlet_server_ip, cloudlet_server_port, settings.base)


if __name__ == "__main__":
    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        is_stop_thread = True
        sys.exit(1)
