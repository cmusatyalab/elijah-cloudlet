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
from optparse import OptionParser
import json
import struct
import socket

# Overlya URL
BASE_DIR = '/home/krha/Cloudlet/image/overlay'
MOPED_DISK = BASE_DIR + '/overlay/moped/overlay1/moped.qcow2.lzma'
MOPED_MEMORY = BASE_DIR + '/overlay/moped/overlay1/moped.mem.lzma'

def run_client(server_address, server_port):
    request_option = {'CPU-core':'2', 'Memory-Size':'4GB'}
    VM_info = {"base_name":"ubuntuLTS", "type":"baseVM", "version":"linux"}
    VM_info['diskimg_size'] = str(os.path.getsize(MOPED_DISK))
    VM_info['memory_snapshot_size'] = str(os.path.getsize(MOPED_MEMORY))
    request_option['VM'] = [VM_info]
    json_str = json.dumps(request_option)

    disk_file = open(MOPED_DISK, "rb")
    memory_file = open(MOPED_MEMORY, "rb")

    # Create the Request object
    print "JSON format : \n" +  json.dumps(request_option, indent=4)
    print "connecting to (%s)" % (server_address)

    # Actually do the request, and get the response
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((server_address, server_port))

        # Send Size
        length32int = struct.pack("!I", len(json_str))
        sock.send(length32int);

        # Send JSON Header
        sock.send(json_str)
        sock.send(disk_file.read())
        sock.send(memory_file.read())

        data = sock.recv(4)
        json_length = struct.unpack("!I", data)[0]
        print "JSON Length : " + str(json_length)
        json_str = sock.recv(json_length)
        print "JSON String : " + json_str

    except socket.error:
        print "Connection Error to %s" % (server_address + ":"  + str(server_port))
    finally:
        sock.close()


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog" + " [option]",
            version="Cloudlet (Android)Emulated Client")
    parser.add_option(
            '-s', '--server', type='string', action='store', dest='address', default="localhost",
            help='Set Server Address, default is  localhost')
    parser.add_option(
            '-p', '--port', type='int', action='store', dest='port', default="8021",
            help='Set Server port number, default is 8021')

    settings, args = parser.parse_args(argv)
    if args:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))

    return settings, args


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    run_client(settings.address, settings.port)
    return 0

if __name__ == "__main__":
    status = main()
    sys.exit(status)
