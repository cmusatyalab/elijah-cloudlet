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
import subprocess
import time
import paramiko
from optparse import OptionParser
from datetime import datetime

WATTS_BIN = "~/cloudlet/src/measurement/power/wattsup"

def wait_until_finish(stdout, stderr, log=True, max_time=20):
    global LOG_FILE
    for x in xrange(max_time):
        ret1 = stdout.readline()

        if len(ret1) == 0:
            break
        time.sleep(0.01)

def run_application(cloud_ip, cloud_port, server_cmd, watts_ip, client_cmd, power_out_file):
    power_log = open(power_out_file, "w")
    power_log_sum = open(power_out_file + ".sum", "w")

    # Start Server Application through SSH
    '''
    ssh_server = paramiko.SSHClient()
    ssh_server.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    print "connection info %s:%d" % (cloud_ip, cloud_port)
    ssh_server.connect(cloud_ip, port=cloud_port, username='ubuntu')
    ssh_stdin, ssh_stdout, ssh_stderr = ssh_server.exec_command(server_cmd)
    '''

    # Start WattsUP through SSH
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    print "connecting to %s for watt measurement" % (watts_ip)
    ssh.connect(watts_ip, username='krha')
    command = "%s /dev/ttyUSB0" % WATTS_BIN
    print command
    ssh_stdin, ssh_stdout, ssh_stderr = ssh.exec_command(command)

    # Start Client App
    proc = subprocess.Popen(client_cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    start_time = datetime.now()
    power_sum = 0.0
    power_counter = 0
    while True:
        proc.poll()
        if proc.returncode == None:
            ret = ssh_stdout.readline()
            power_value = float(ret.split(",")[0])
            if power_value == 0.0:
                continue
            if power_value < 1.0 or power_value > 30.0:
                print "Error at Power Measurement with %f" % (power_value)
                sys.exit(1)
            power_log.write("%s\t%s" % (str(datetime.now()), ret))
            print "current power : %f" % power_value
            power_sum = power_sum + power_value
            power_counter = power_counter + 1
            time.sleep(0.1)
            continue
        elif proc.returncode == 0:
            print "Client Finished"
            break;
        else:
            print "Client End with Error : %s" % (client_cmd)
            ssh.close()
            power_log.close()
            return 1


    # Stop WattsUP through SSH
    end_time = datetime.now()
    power_log_sum.write("%s\t%f\t(%f/%d)" % \
            (str(end_time-start_time), power_sum/power_counter, power_sum, power_counter))
    #ssh_server.close()
    ssh.close()
    power_log.close()
    return 0

def process_command_line(argv):
    parser = OptionParser(usage="usage: %prog -c [Client Type] -s [WattsUp connected Server IP]",
            version="Power measurement")
    parser.add_option(
            '-i', '--input', action='store', type='string', dest='input',
            help='Input path for application')
    parser.add_option(
            '-a', '--app', action='store', type='string', dest='app', default="graphics",
            help='Client Type Between moped and graphics')
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='watts_server', default="dagama.isr.cs.cmu.edu",
            help='Server IP that has connected to WattsUp Gear')
    settings, args = parser.parse_args(argv)

    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    if not settings.app:
        parser.error("we need client type")
    if not settings.watts_server:
        parser.error("we need server IP")

    return settings, args

def turn_cores(core_on):
    return

    for index in xrange(1,4):
        print "cpu " + str(index)
        if core_on:
            open("/sys/devices/system/cpu/cpu%d/online" % (index), "w").write("1\n")
        else:
            open("/sys/devices/system/cpu/cpu%d/online" % (index), "w").write("0\n")
        time.sleep(1)
    time.sleep(10)


def batch_graphics_test(watts_server, input_file):
    if not input_file:
        input_file = 'acc_input_10min'
    cloud_list = [("cloudlet.krha.kr", 19093, "g_hail", 2221), \
            ("server.krha.kr", 19093, "g_cage", 2221), \
            ("23.21.103.194", 9093, "g_east", 22), \
            ("184.169.142.70", 9093, "g_west", 22), \
            ("176.34.100.63", 9093, "g_eu", 22), \
            ("46.137.209.173", 9093, "g_asia", 22)]
    for cloud in cloud_list:
        client_cmd = "./graphics_client.py -i %s -s %s -p %d > %s" % (input_file, cloud[0], cloud[1], cloud[2])
        server_cmd = "./cloudlet/src/app/graphics/bin/linux/x86_64/release/cloudlet_test -j 4"
        print "RUNNING : %s" % (client_cmd)
        ret = run_application(cloud[0], cloud[3], server_cmd, watts_server, client_cmd, cloud[2]+".power")
        if not ret == 0:
            print "Error at running %s" % (client_cmd)
            sys.exit(1)
        time.sleep(30)


def graphics_local(watts_server, input_file):
    if not input_file:
        input_file = 'acc_input_10min'

    raw_input("Prepare local server and Enter. ")
    cloud = ("localhost", 9093, "g_local", 22)
    client_cmd = "./graphics_client.py -i %s -s %s -p %d > %s" % (input_file, cloud[0], cloud[1], cloud[2])
    print "RUNNING : %s" % (client_cmd)
    ret = run_application(cloud[0], cloud[3], '', watts_server, client_cmd, cloud[2]+".power")
    if not ret == 0:
        print "Error at running %s" % (client_cmd)
        sys.exit(1)


def batch_object_test(watts_server, input_dir):
    if not input_dir:
        input_dir = 'object_images/'

    cloud_list = [("cloudlet.krha.kr", 19092, "o_hail", 2221), \
            ("server.krha.kr", 19092, "o_cage", 2221), \
            ("23.21.103.194", 9092, "o_east", 22), \
            ("184.169.142.70", 9092, "o_west", 22), \
            ("176.34.100.63", 9092, "o_eu", 22), \
            ("46.137.209.173", 9092, "o_asia", 22)]
    for cloud in cloud_list:
        client_cmd = "./moped_client.py -i %s -s %s -p %d > %s" % (input_dir, cloud[0], cloud[1], cloud[2])
        server_cmd = "./cloudlet/src/app/moped/moped_server -j 4 > run_log"
        print "RUNNING : %s" % (client_cmd)
        ret = run_application(cloud[0], cloud[3], server_cmd, watts_server, client_cmd, cloud[2]+".power")
        if not ret == 0:
            print "Error at running %s" % (client_cmd)
            sys.exit(1)
        time.sleep(30)


def object_local(watts_server, input_dir):
    if not input_dir:
        input_dir = 'object_images/'

    raw_input("Prepare local server and Enter. ")
    cloud = ("localhost", 9092, "g_local", 22)
    client_cmd = "./moped_client.py -i %s -s %s -p %d > %s" % (input_dir, cloud[0], cloud[1], cloud[2])
    print "RUNNING : %s" % (client_cmd)
    ret = run_application(cloud[0], cloud[3], '', watts_server, client_cmd, cloud[2]+".power")
    if not ret == 0:
        print "Error at running %s" % (client_cmd)
        sys.exit(1)


def batch_speech(watts_server, input_dir):
    if not input_dir:
        input_dir = './SPEECH/input'

    cloud_list = [#("cloudlet.krha.kr", 10191, "s_hail", 2221)]#, \
#            ("server.krha.kr", 10191, "s_cage", 2221), \
            ("23.21.103.194", 10191, "s_east", 22), \
            ("184.169.142.70", 10191, "s_west", 22), \
            ("176.34.100.63", 10191, "s_eu", 22), \
            ("46.137.209.173", 10191, "s_asia", 22)]
    for cloud in cloud_list:
        client_cmd = "java -jar SPEECH/SpeechrecDesktopControlClient.jar %s %d %s > %s" % (cloud[0], cloud[1], input_dir, cloud[2])
        print "RUNNING : %s" % (client_cmd)
        ret = run_application(cloud[0], cloud[3], None, watts_server, client_cmd, cloud[2]+".power")
        if not ret == 0:
            print "Error at running %s" % (client_cmd)
            sys.exit(1)
        time.sleep(30)


def speech_local(watts_server, input_dir):
    if not input_dir:
        input_dir = './SPEECH/input'

    raw_input("Prepare local server and Enter. ")
    cloud = ("localhost", 10191, "s_local", 22)
    client_cmd = "java -jar SPEECH/SpeechrecDesktopControlClient.jar %s %d %s > %s" % (cloud[0], cloud[1], input_dir, cloud[2])
    print "RUNNING : %s" % (client_cmd)
    ret = run_application(cloud[0], cloud[3], '', watts_server, client_cmd, cloud[2]+".power")
    if not ret == 0:
        print "Error at running %s" % (client_cmd)
        sys.exit(1)


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    if settings.app == "graphics":
        batch_graphics_test(settings.watts_server, settings.input)
    elif settings.app == "graphics_local":
        graphics_local(settings.watts_server, settings.input)
    elif settings.app == "object":
        batch_object_test(settings.watts_server, settings.input)
    elif settings.app == "object_local":
        object_local(settings.watts_server, settings.input)
    elif settings.app == "speech":
        batch_speech(settings.watts_server, settings.input)
    elif settings.app == "speech_local":
        speech_local(settings.watts_server, settings.input)
    else:
        sys.stderr("Not valid command\n")
        sys.exit(0)


if __name__ == "__main__":
    status = main()
    sys.exit(status)
