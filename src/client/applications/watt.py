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
import subprocess
import time
import paramiko
from optparse import OptionParser
from datetime import datetime

WATTS_BIN = "~/cloudlet/src/measurement/power/wattsup/wattsup"

def wait_until_finish(stdout, stderr, log=True, max_time=20):
    global LOG_FILE
    for x in xrange(max_time):
        ret1 = stdout.readline()

        if len(ret1) == 0:
            break
        time.sleep(0.01)

def run_application(server_ip, app_cmd, power_out_file):
    power_log = open(power_out_file, "w")
    power_log_sum = open(power_out_file + ".sum", "w")
    # Start WattsUP through SSH
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(server_ip, username='krha')
    command = "%s /dev/ttyUSB0" % WATTS_BIN
    ssh_stdin, ssh_stdout, ssh_stderr = ssh.exec_command(command)


    # Start Client App
    proc = subprocess.Popen(app_cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    start_time = datetime.now()
    power_sum = 0.0
    power_counter = 0
    while True:
        proc.poll()
        if proc.returncode == None:
            ret = ssh_stdout.readline()
            power_log.write("%s\t%s" % (str(datetime.now()), ret))
            print "current power : %f" % float(ret.split(",")[0])
            power_sum = power_sum + float(ret.split(",")[0])
            power_counter = power_counter + 1
            time.sleep(0.1)
            continue
        elif proc.returncode == 0:
            print "Client Finished"
            break;
        else:
            print "Client End with Error : %s" % (app_cmd)
            ssh.close()
            power_log.close()
            return 1


    # Stop WattsUP through SSH
    end_time = datetime.now()
    power_log_sum.write("%s\t%f\t(%f/%d)" % (str(end_time-start_time), power_sum/power_counter, power_sum, power_counter))
    ssh.close()
    power_log.close()
    return 0

def process_command_line(argv):
    parser = OptionParser(usage="usage: %prog -c [Client Type] -s [WattsUp connected Server IP]",
            version="Power measurement")
    parser.add_option(
            '-c', '--cleint', action='store', type='string', dest='client_name',
            help='Client Type Between moped and graphics')
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server_ip',
            help='Server IP that has connected to WattsUp Gear')
    settings, args = parser.parse_args(argv)

    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    if not settings.client_name:
        parser.error("we need client type")
    if not settings.server_ip:
        parser.error("we need server IP")

    return settings, args


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    client_list = [("server.krha.kr", 19093, "g_cloudlet"), ("23.21.103.194", 9093, "g_east")]
    for client in client_list:
        client_cmd = "./graphics_client.py -s %s -p %d > %s" % (client[0], client[1], client[2])
        print "RUNNING : %s" % (client_cmd)
        ret = run_application(settings.server_ip, client_cmd, client[2]+".power")
        if not ret == 0:
            print "Error at running %s" % (client_cmd)
            sys.exit(1)
        time.sleep(5)


if __name__ == "__main__":
    status = main()
    sys.exit(status)
