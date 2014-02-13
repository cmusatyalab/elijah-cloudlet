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

import sys
import time
import os
from synthesis_client import Client
from optparse import OptionParser
import cloudlet_client
from cloudlet.synthesis_protocol import Protocol as protocol

sys.path.append("../src/")

application_names = ['moped', 'face', 'mar', 'speech', 'graphics']

def process_command_line(argv):
    global command_type
    global application_names

    parser = OptionParser(usage="usage: ./rapid_app.py -a app_name [option]",\
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
    OVERLAY_ROOT = "../../../../image/overlay/"
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

    client = Client(ip, port, overlay_meta_path, app_function, synthesis_options)
    client.provisioning()
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
