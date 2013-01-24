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
import time
import os
from rapid_client import synthesis
from optparse import OptionParser
import cloudlet_client
from synthesis_protocol import Protocol as protocol

application_names = ['moped', 'face', 'mar', 'speech', 'graphics']

def process_command_line(argv):
    global command_type
    global application_names

    parser = OptionParser(usage="usage: ./rapid_app.py [option]",\
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
    if (not settings.application) or (settings.application not in application_names):
        parser.error("Need application among [%s]" % ('|'.join(application_names)))
    
    return settings, args


def exec_moped():
    cmd, output_file = cloudlet_client.get_app_cmd("moped")
    cloudlet_client.exec_application(cmd)
    return cmd, output_file

def exec_face():
    cmd, output_file = cloudlet_client.get_app_cmd("face")
    cloudlet_client.exec_application(cmd)
    return cmd, output_file

def exec_graphics():
    cmd, output_file = cloudlet_client.get_app_cmd("graphics")
    cloudlet_client.exec_application(cmd)
    return cmd, output_file


def exec_speech():
    cmd, output_file = cloudlet_client.get_app_cmd("speech")
    cloudlet_client.exec_application(cmd)
    return cmd, output_file


def exec_mar():
    cmd, output_file = cloudlet_client.get_app_cmd("mar")
    cloudlet_client.exec_application(cmd)
    return cmd, output_file


def app_synthesis(ip, port, app_name, options=None):
    #synthesis option
    synthesis_options = dict()
    if options == None:
        synthesis_options[protocol.SYNTHESIS_OPTION_DISPLAY_VNC] = True
        synthesis_options[protocol.SYNTHESIS_OPTION_EARLY_START] = False
    else:
        synthesis_options.update(options)

    #overlay path
    OVERLAY_ROOT = "../../../image/overlay/"
    overlay_meta_path = None
    if len(app_name.split("_")) > 1:
        app_name, order, blob_size = app_name.split("_")
        overlay_meta_path = "%s/%s/%s/%s/overlay-meta" % \
                (OVERLAY_ROOT, app_name, order, blob_size)

    if os.path.exists(overlay_meta_path) == False:
        sys.stderr.write("Cannot find overlay at %s\n" % overlay_meta_path)
        sys.exit(1)

    #application method
    app_function = None
    if app_name== "moped": 
        app_function = exec_moped
    elif app_name == "face":
        app_function = exec_face
    elif app_name == "speech":
        app_function = exec_speech
    elif app_name == "mar":
        app_function = exec_mar
    elif app_name == "graphics":
        app_function = exec_graphics

    synthesis(ip, port, overlay_meta_path, app_function, synthesis_options)
    time.sleep(10)


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    if settings.server_ip:
        cloudlet_server_ip = settings.server_ip
    else:
        cloudlet_server_ip = "cloudlet.krha.kr"
    cloudlet_server_port = 8021

    app_name = "%s_access_1024" % settings.application
    app_synthesis(cloudlet_server_ip, cloudlet_server_port, app_name)


if __name__ == "__main__":
    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        is_stop_thread = True
        sys.exit(1)
