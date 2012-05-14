
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
from datetime import datetime
import time
import struct
import math
import urllib2
import urllib
import json
import subprocess
import paramiko
import cv
import cloudlet_client

MOPED_CLIENT_PATH = "/home/krha/cloudlet/src/client/applications/"
application_names = ["moped", "face", "graphics", "speech", "mar", "null"]

def FPV_capture(output_name):
    frame = cv.QueryFrame(camcapture)
    pass


def process_command_line(argv):
    global command_type
    global application_names

    parser = OptionParser(usage="usage: ./FPV_client.py", version="FPV Desktop Client")
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server', default="server.krha.kr",
            help="Set Server IP")
    parser.add_option(
            '-a', '--app', action='store', type='string', dest='app',
            help="Set Application name among (%s)" % ",".join(application_names))
    parser.add_option(
            '-p', '--port', dest='port', type='int', default='8081',
            help="Set Server Port")
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    if not settings.app:
        parser.error("Application name is required :%s" % ' '.join(application_names))
    if not settings.app in application_names:
        parser.error("Application name is required :%s" % ' '.join(application_names))

    return settings, args



def main():
    settings, args = process_command_line(sys.argv[1:])
    cloudlet_client.synthesis(settings.server, settings.port, settings.app)

    # Run Client



if __name__ == "__main__":
    if MOPED_CLIENT_PATH not in sys.path:
        sys.path.append(MOPED_CLIENT_PATH)
        import moped_client
    status = main()
    sys.exit(status)
