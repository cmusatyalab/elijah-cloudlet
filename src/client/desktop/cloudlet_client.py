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
from threading import Thread

WATTS_BIN = "~/cloudlet/src/measurement/power/wattsup"
OVERLAY_DIR = '/home/krha/cloudlet/image/overlay'

command_type = ["synthesis_cloud", "synthesis_mobile", "isr_cloud", "isr_mobile"]
application_names = ["moped", "face", "graphics", "speech", "null"]
cloudlet_server_ip = "server.krha.kr"
cloudlet_server_port = 8021
isr_server_ip = "server.krha.kr"
isr_server_port = 9091
is_stop_thread = False
last_average_power = 0.0

APP_DIR = "/home/krha/cloudlet/src/client/applications"
face_data = "/home/krha/cloudlet/src/client/desktop/face_input"
speech_data = "/home/krha/cloudlet/src/client/desktop/speech_input/"
MOPED_client = "%s/moped_client.py -i %s/object_images/ -s %s -p 9092 > ./ret/o_cloudlet" % (APP_DIR, APP_DIR, cloudlet_server_ip)
GRAPHICS_client = "%s/graphics_client.py -i %s/acc_input_50sec -s %s -p 9093 > ./ret/g_cloudlet" % (APP_DIR, APP_DIR, cloudlet_server_ip)
FACE_client = "java -jar %s/FACE/FacerecDesktopControlClient.jar %s 9876 %s > ./ret/f_cloudlet" % (APP_DIR, cloudlet_server_ip, face_data)
SPEECH_client = "java -jar %s/SPEECH/SpeechrecDesktopControlClient.jar %s 10191 %s > ./ret/s_cloudlet" % (APP_DIR, cloudlet_server_ip, speech_data)

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

    parser = OptionParser(usage="usage: ./cloudlet_client.py [%s]" % "|".join(command_type), version="Desktop Cloudlet Client")
    parser.add_option(
            '-c', '--commnad', action='store', type='string', dest='command',
            help="Set Command Type among (%s)" % ",".join(command_type))
    parser.add_option(
            '-a', '--app', action='store', type='string', dest='app',
            help="Set Application name among (%s)" % ",".join(application_names))
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
    global application_names
    cmd = ''
    if app_name == application_names[0]:    # moped
        cmd = MOPED_client
    elif app_name == application_names[1]:  # face
        cmd = FACE_client
    elif app_name == application_names[2]:  # physics
        cmd = GRAPHICS_client
    elif app_name == application_names[3]:  # speech
        cmd = SPEECH_client
    elif app_name == application_names[4]:  # null
        return 0

    print "Run client : %s" % (cmd)
    proc = subprocess.Popen(cmd, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    while True:
        output = proc.stdout.readline()
        if len(output) == 0:
            break;
        sys.stdout.write(output)
        sys.stdout.flush()
        time.sleep(0.01)
    proc.wait()
    return proc.returncode

def get_overlay_info(app_name):
    global OVERLAY_DIR
    base_name = ''
    overlay_disk_path = ''
    overlay_disk_size = ''
    overlay_mem_path = ''
    overlay_mem_size = ''

    if app_name == application_names[0]:    # moped
        base_name = 'ubuntu11.10-server'
        overlay_disk_path = "%s/%s/moped/ubuntu-11.overlay.4cpu.4096mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/moped/ubuntu-11.overlay.4cpu.4096mem.mem.lzma" % (OVERLAY_DIR, base_name)
        overlay_mem_size = os.path.getsize(overlay_mem_path)
    elif app_name == application_names[1]:  # face
        base_name = 'window7'
        overlay_disk_path = "%s/%s/face/window7-enterprise-i386.overlay.4cpu.4096mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/face/window7-enterprise-i386.overlay.4cpu.4096mem.mem.lzma" % (OVERLAY_DIR, base_name)
        overlay_mem_size = os.path.getsize(overlay_mem_path)
    elif app_name == application_names[2]:  # physics
        base_name = 'ubuntu11.10-server'
        overlay_disk_path = "%s/%s/graphics/ubuntu-11.overlay.4cpu.4096mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/graphics/ubuntu-11.overlay.4cpu.4096mem.mem.lzma" % (OVERLAY_DIR, base_name)
        overlay_mem_size = os.path.getsize(overlay_mem_path)
    elif app_name == application_names[3]:  # speech
        base_name = 'window7'
        overlay_disk_path = "%s/%s/speech/window7-enterprise-i386.overlay.4cpu.4096mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/speech/window7-enterprise-i386.overlay.4cpu.4096mem.mem.lzma" % (OVERLAY_DIR, base_name)
        overlay_mem_size = os.path.getsize(overlay_mem_path)
    elif app_name == application_names[4]:  # null
        base_name = 'ubuntu11.10-server'
        overlay_disk_path = "%s/%s/null/ubuntu-11.overlay.4cpu.4096mem.qcow2.lzma" % (OVERLAY_DIR, base_name)
        overlay_disk_size = os.path.getsize(overlay_disk_path)
        overlay_mem_path = "%s/%s/null/ubuntu-11.overlay.4cpu.4096mem.mem.lzma" % (OVERLAY_DIR, base_name)
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


def energy_measurement(address, port, power_out_file):
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
    energy_thread = Thread(target=energy_measurement, args=("dagama.isr.cs.cmu.edu", 22, "%s.%s.VM" % (settings.command, settings.app)))
    energy_thread.start()
    time.sleep(2)

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

    is_stop_thread = True
    print "Finish VM Delivery and Wait for cool down 10 seconds for Application power measurement"
    energy_thread.join()
    vm_power = last_average_power
    time.sleep(10)

    # run application
    is_stop_thread = False
    energy_thread = Thread(target=energy_measurement, args=("dagama.isr.cs.cmu.edu", 22, "%s.%s.APP" % (settings.command, settings.app)))
    energy_thread.start()
    app_start_time = time.time()
    run_application(settings.app)
    app_end_time = time.time()

    # wait power meter logging
    is_stop_thread = True
    print "Waiting for stop power measurement"
    energy_thread.join()
    app_power = last_average_power

    # Print out measurement
    vm_time = vm_end_time-vm_start_time
    app_time = app_end_time-app_start_time
    print "VM_time\tVM_power\tVM_J\tApp_time\App_power\tApp_J"
    print "%f\t%f\t%f\t%f\t%f\t%f" % (vm_time, vm_power, (vm_power*vm_time), app_time, app_power, (app_power*app_time))
    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
