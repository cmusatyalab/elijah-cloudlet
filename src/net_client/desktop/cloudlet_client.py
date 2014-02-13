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
from threading import Thread

WATTS_BIN = "~/cloudlet/src/measurement/power/wattsup"
OVERLAY_DIR = '/home/krha/cloudlet/image/overlay/old'

command_type = ["synthesis_cloud", "synthesis_mobile", "isr_cloud", "isr_mobile"]
application_names = ["moped", "face", "graphics", "speech", "mar", "null", "webserver"]
cloudlet_server_ip = "cloudlet.krha.kr"
cloudlet_server_port = 8021
isr_server_ip = "cloudlet.krha.kr"
isr_server_port = 9091
is_stop_thread = False
last_average_power = 0.0


delivery_server = "cloudlet.krha.kr"
APP_DIR = "/home/krha/cloudlet/provision/src/net_client/applications"
MOPED_client = "%s/moped_client.py -i ./input/moped -s %s -p 9092" % (APP_DIR, delivery_server)
GRAPHICS_client = "%s/graphics_client.py -i ./input/graphics/acc_input_1sec -s %s -p 9093" % (APP_DIR, delivery_server)
FACE_client = "java -jar %s/FACE/FacerecDesktopControlClient.jar %s 9876 ./input/face/" % (APP_DIR, delivery_server)
SPEECH_client = "java -jar %s/SPEECH/SpeechrecDesktopControlClient.jar %s 10191 ./input/speech" % (APP_DIR, delivery_server)
MAR_client = "%s/mar_client.py -i ./input/mar/ -s %s -p 9094" % (APP_DIR, delivery_server)


def convert_to_CDF(input_file):
    input_lines = open(input_file, "r").read().split("\n")
    rtt_list = []
    jitter_sum = 0.0
    start_time = 0.0
    end_time = 0.0
    for index, oneline in enumerate(input_lines):
        if len(oneline.split("\t")) != 6 and len(oneline.split("\t")) != 5:
            #sys.stderr.write("Error at input line at %d, %s\n" % (index, oneline))
            continue
        try:
            if float(oneline.split("\t")[2]) == 0:
                sys.stderr.write("Error at input line at %d, %s\n" % (index, oneline))
                continue
        except ValueError:
            continue
        try:
            rtt_list.append(float(oneline.split("\t")[3]))
            if not index == 0:
                # protect error case where initial jitter value is equals to latency
                jitter_sum += (float(oneline.split("\t")[4]))

            if start_time == 0.0:
                start_time = float(oneline.split("\t")[1])
            end_time = float(oneline.split("\t")[2])
        except ValueError:
            sys.stderr.write("Error at input line at %d, %s\n" % (index, oneline))
            continue

    rtt_sorted = sorted(rtt_list)
    total_rtt_number = len(rtt_sorted)
    cdf = []
    summary = "%f\t%f\t%f" % ( \
            rtt_sorted[int(total_rtt_number*0.01)], \
            rtt_sorted[int(total_rtt_number*0.5)], \
            rtt_sorted[int(total_rtt_number*0.99)])
    for index, value in enumerate(rtt_sorted):
        data = (value, 1.0 * (index+1)/total_rtt_number)
        cdf.append(data)
    return summary, cdf


def recv_all(sock, size):
    data = ''
    while len(data) < size:
        data += sock.recv(size - len(data))
    return data

def get_local_ipaddress():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("gmail.com",80))
    ipaddress = (s.getsockname()[0])
    s.close()
    return ipaddress

def process_command_line(argv):
    global command_type
    global application_names

    parser = OptionParser(usage="usage: ./cloudlet_client.py -c [%s]" % "|".join(command_type), version="Desktop Cloudlet Client")
    parser.add_option(
            '-c', '--commnad', action='store', type='string', dest='command',
            help="Set Command Type among (%s)" % ",".join(command_type))
    parser.add_option(
            '-a', '--app', action='store', type='string', dest='app',
            help="Set Application name among (%s)" % ",".join(application_names))
    parser.add_option(
            '-p', '--power', dest='power',
            help="Set power measurement")
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    if not settings.command:
        parser.error("Command is required :%s" % ' '.join(command_type))
    if not settings.command in command_type:
        parser.error("Command is required :%s" % ' '.join(command_type))
    if not settings.app:
        parser.error("Application name is required :%s" % ' '.join(application_names))
    if not settings.app in application_names:
        parser.error("Application name is required :%s" % ' '.join(application_names))

    return settings, args


def isr_launch(url, device, app_name):
    print "Requesting.. %s, %s, %s" % (url, device, app_name)
    #JSON data
    json_data = json.dumps({"run-type":device, "application":app_name})
    parameters = {'info':json_data}
    data = urllib.urlencode(parameters)
    request = urllib2.Request(url, data)
    try:
        response = urllib2.urlopen(request)
    except urllib2.HTTPError, msg:
        sys.stderr.write("Error at HTTP: %s\n" % msg)
        sys.exit(1)

    print response


def synthesis_from_cloud(url, app_name):
    print "Requesting.. %s, %s" % (url, app_name)
    #JSON data
    json_data = json.dumps({"run-type":"test", "application":app_name})
    parameters = {'info':json_data}
    data = urllib.urlencode(parameters)
    request = urllib2.Request(url, data)
    try:
        response = urllib2.urlopen(request)
    except urllib2.HTTPError, msg:
        sys.stderr.write("Error at HTTP: %s\n" % msg)
        sys.exit(1)

    print response


def run_application(app_name):
    cmd, output_file = get_app_cmd(app_name)
    return exec_application(cmd), output_file


def get_app_cmd(app_name):
    global application_names
    cmd = ''
    output_file = ''
    if not os.path.exists("./ret"):
        os.mkdir("./ret")
    time_str = datetime.now().strftime("%a:%X")
    if app_name == application_names[0]:    # moped
        output_file = "./ret/o_cloudlet_" + time_str 
        cmd = MOPED_client + " 2>&1 > %s" % str(output_file)
    elif app_name == application_names[1]:  # face
        output_file = "./ret/f_cloudlet_" + time_str
        cmd = FACE_client + " 2>&1 > %s" % output_file
    elif app_name == application_names[2]:  # physics
        output_file = "./ret/g_cloudlet_" + time_str
        cmd = GRAPHICS_client + " 2>&1 > %s" % output_file
    elif app_name == application_names[3]:  # speech
        output_file = "./ret/s_cloudlet_" + time_str
        cmd = SPEECH_client + " 2>&1 > %s" % output_file
    elif app_name == application_names[4]:  # speech
        output_file = "./ret/s_cloudlet_" + time_str
        cmd = MAR_client + " 2>&1 > %s" % output_file
    elif app_name == application_names[5]:  # null
        return 0, output_file
    elif app_name == application_names[6]:  # webserver
        cmd = "wget %s:9092/vmtest.ko" % delivery_server
    return cmd, output_file


def exec_application(cmd):
    print "Run client : %s" % (cmd)
    while True:
        proc = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        while True:
            output = proc.stdout.readline()
            if len(output) == 0:
                break;
            #sys.stdout.write(output)
            #sys.stdout.flush()
            time.sleep(0.001)
        proc.wait()
        if proc.returncode == 0:
            break
        else:
            time.sleep(0.1)

    return proc.returncode


def get_overlay_info(app_name):
    global OVERLAY_DIR
    base_name = ''
    overlay_disk_path = ''
    overlay_disk_size = ''
    overlay_mem_path = ''
    overlay_mem_size = ''

    if app_name == application_names[0]:    # moped
        base_name = 'ubuntu'
        overlay_disk_path = "%s/%s/moped/precise.overlay.4cpu.1024mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/moped/precise.overlay.4cpu.1024mem.mem.lzma" % (OVERLAY_DIR, base_name)
        overlay_mem_size = os.path.getsize(overlay_mem_path)
    elif app_name == application_names[1]:  # face
        base_name = 'window7'
        overlay_disk_path = "%s/%s/face/window7.overlay.4cpu.1024mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/face/window7.overlay.4cpu.1024mem.mem.lzma" % (OVERLAY_DIR, base_name)
        overlay_mem_size = os.path.getsize(overlay_mem_path)
    elif app_name == application_names[2]:  # physics
        base_name = 'ubuntu'
        overlay_disk_path = "%s/%s/graphics/precise.overlay.4cpu.1024mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/graphics/precise.overlay.4cpu.1024mem.mem.lzma" % (OVERLAY_DIR, base_name)
        overlay_mem_size = os.path.getsize(overlay_mem_path)
    elif app_name == application_names[3]:  # speech
        base_name = 'ubuntu'
        overlay_disk_path = "%s/%s/speech/precise.overlay.4cpu.1024mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/speech/precise.overlay.4cpu.1024mem.mem.lzma" % (OVERLAY_DIR, base_name)
        overlay_mem_size = os.path.getsize(overlay_mem_path)
    elif app_name == application_names[4]:  # mar
        base_name = 'window7'
        overlay_disk_path = "%s/%s/mar/window7.overlay.4cpu.1024mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/mar/window7.overlay.4cpu.1024mem.mem.lzma" % (OVERLAY_DIR, base_name)
        overlay_mem_size = os.path.getsize(overlay_mem_path)
    elif app_name == application_names[5]:  # null
        base_name = 'ubuntu11.10-server'
        overlay_disk_path = "%s/%s/null/ubuntu-11.overlay.4cpu.1024mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/null/ubuntu-11.overlay.4cpu.1024mem.mem.lzma" % (OVERLAY_DIR, base_name)
        overlay_mem_size = os.path.getsize(overlay_mem_path)
        
    return (base_name, overlay_disk_path, overlay_disk_size, overlay_mem_path, overlay_mem_size) 


def synthesis(address, port, app_name):
    (base_name, overlay_disk_path, overlay_disk_size, overlay_mem_path, overlay_mem_size) = get_overlay_info(app_name)
    json_str = {"command":33, \
            "protocol-version": "1.0", \
            "VM":[{ \
                "overlay_name":app_name, \
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
    disk_data = open(overlay_disk_path, "rb").read()
    sock.sendall(disk_data)
    mem_data = open(overlay_mem_path, "rb").read()
    sock.sendall(mem_data)
    
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


def server_run(address, port, command, server_output_file):
    return
    global is_stop_thread
    if command == command_type[0]:     #synthesis from cloud
        server_cmd = "/home/krha/cloudlet/src/server/cloudlet_cloud.py run"
    elif command == command_type[1]:   #synthesis from mobile
        server_cmd = "/home/krha/cloudlet/src/server/synthesis.py run -c /home/krha/cloudlet/src/server/config/VM_config.json"
    elif command == command_type[2]:   #ISR from cloud
        server_cmd = "/home/krha/cloudlet/src/server/isr_run.py run -u test -s dagama.isr.cs.cmu.edu"
    elif command == command_type[3]:   #ISR from mobile
        server_cmd = "/home/krha/cloudlet/src/server/isr_run.py run -u test -s netbook.krha.kr"
    cmd = "ssh -A -p %d krha@%s %s" % (port, address, server_cmd)
    print cmd
    proc = subprocess.Popen(cmd.split(), stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    server_log = open(server_output_file, "w")
    while is_stop_thread == False:
        time.sleep(1)
        if not proc.returncode:
            continue
        if proc.returncode != 0:
            print proc.stderr.readline()
            break
        if proc.returncode == 0:
            print "Server closed"
            break

    print "Terminate Server"
    server_log.close()
    proc.terminate()
    return 0

def energy_measurement(address, port, power_out_file):
    import paramiko
    global is_stop_thread
    global last_average_power

    # Start WattsUP through SSH
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
    ssh.connect(address, username='krha')
    command = "%s /dev/ttyUSB0" % WATTS_BIN
    ssh_stdin, ssh_stdout, ssh_stderr = ssh.exec_command(command)

    start_time = datetime.now()
    power_sum = 0.0
    power_counter = 0
    power_log = open(power_out_file, "w")
    power_log_sum = open(power_out_file + ".sum", "w")
    while is_stop_thread == False:
        ret = ssh_stdout.readline()
        if not ret:
            continue
        power_value = float(ret.split(",")[0])
        if power_value == 0.0:
            continue
        if power_value < 1.0 or power_value > 30.0:
            print "Error at Power Measurement with %f" % (power_value)
            sys.exit(1)
        power_log.write("%s\t%s" % (str(datetime.now()), ret))
        #print "current power : %f" % power_value
        power_sum = power_sum + power_value
        power_counter = power_counter + 1
        time.sleep(0.1)

    # Stop WattsUP through SSH
    end_time = datetime.now()
    message = "%s\t%f\t(%f/%d)" % (str(end_time-start_time), power_sum/power_counter, power_sum, power_counter)
    power_log_sum.write(message)
    ssh.close()
    power_log.close()
    power_log_sum.close()
    last_average_power = power_sum/power_counter
    print "Average Power for %s: %s" % (power_out_file, message)

    return 0


def main(argv=None):
    global command_type
    global cloudlet_server_ip
    global cloudlet_server_port
    global is_stop_thread
    global last_average_power
    is_stop_thread = False

    settings, args = process_command_line(sys.argv[1:])
    time_str = datetime.now().strftime("%a:%X")
    '''
    # This thread launch server-side counterpart using ssh command execute
    # We DO NOT recommend to use it beacuse it might cause problem related to X
    server_thread = Thread(target=server_run, args=("server.krha.kr", \
            22, settings.command, "./ret/%s.%s.server.%s" % (settings.command, settings.app, time_str)))
    server_thread.start()
    '''
    if settings.power:
        energy_thread = Thread(target=energy_measurement, args=("dagama.isr.cs.cmu.edu", \
                22, "./ret/%s.%s.VM.%s" % (settings.command, settings.app, time_str)))
        energy_thread.start()
        time.sleep(5)

    vm_start_time = time.time()
    if settings.command == command_type[0]:     #synthesis from cloud
        url = "http://%s:%d/cloudlet" % (cloudlet_server_ip, isr_server_port)
        synthesis_from_cloud(url, settings.app)
    elif settings.command == command_type[1]:   #synthesis from mobile
        synthesis(cloudlet_server_ip, cloudlet_server_port, settings.app)
    elif settings.command == command_type[2]:   #ISR from cloud
        url = "http://%s:%d/isr" % (cloudlet_server_ip, isr_server_port)
        isr_launch(url, "cloud", settings.app)
    elif settings.command == command_type[3]:   #ISR from mobile
        url = "http://%s:%d/isr" % (cloudlet_server_ip, isr_server_port)
        isr_launch(url, "mobile", settings.app)
    vm_end_time = time.time()

    # run application
    app_start_time = time.time()
    while True:
        # Try to connect to Server program at VM until it gets some data
        ret, output_file = run_application(settings.app)
        if ret == 0:
            break;
        else:
            print "waiting for client connection"
        time.sleep(0.1)
    app_end_time = time.time()

    # wait for energy measurement
    is_stop_thread = True
    if settings.power:
        print "Finish VM Delivery and Wait for cool down 10 seconds for Application power measurement"
        energy_thread.join()
        vm_power = last_average_power
        time.sleep(10)

    # Print out measurement
    vm_time = vm_end_time-vm_start_time
    app_time = app_end_time-app_start_time
    first_response_time = app_end_time-vm_start_time
    #summary, cdf = convert_to_CDF(output_file)
    message = "-------------------------------------\n"
    message += "VM_time\tApp_time\n"
    message += "%f\t%f\n" % (vm_time, app_time)
    print message
    print "first response time: %f\n", (first_response_time)
    open(output_file + ".log", "w").write(message)
    return 0


if __name__ == "__main__":
    try:
        status = main()
        sys.exit(status)
    except KeyboardInterrupt:
        is_stop_thread = True
        sys.exit(1)
