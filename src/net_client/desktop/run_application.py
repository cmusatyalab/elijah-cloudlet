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
from optparse import OptionParser
import time
import cloudlet_client

application_names = ["moped", "face", "graphics", "speech", "mar", "null", "webserver"]

def process_command_line(argv):
    global command_type
    global application_names

    parser = OptionParser(usage="usage: ./run_application.py -a [app_name]" ,\
            version="Desktop application client")
    parser.add_option(
            '-a', '--app', action='store', type='string', dest='app',
            help="Set Application name among (%s)" % ",".join(application_names))
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    if not settings.app:
        parser.error("Application name is required among (%s)" % ' '.join(application_names))
    return settings, args


def main(argv=None):
    global command_type
    global cloudlet_server_ip
    global cloudlet_server_port
    global is_stop_thread
    global last_average_power
    app_start_time = time.time()

    settings, args = process_command_line(sys.argv[1:])

    # run application
    while True:
        # Try to connect to Server program at VM until it gets some data
        ret, output_file = cloudlet_client.run_application(settings.app)
        if ret == 0:
            break;
        else:
            print "waiting for client connection"
        time.sleep(0.1)
    app_end_time = time.time()

    application_run_time = app_end_time-app_start_time
    print "application run time: %f\n" % (application_run_time)
    return 0


if __name__ == "__main__":
    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        is_stop_thread = True
        sys.exit(1)
