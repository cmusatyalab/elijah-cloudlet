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

import xdelta3
import os
import commands
import filecmp
import sys
import subprocess
import getopt
import time
import socket
from datetime import datetime, timedelta
import telnetlib
import pylzma
from flask import Flask,flash, request,render_template, Response,session,g
from flask import Flask, request, session, g, redirect, url_for, abort, render_template, flash, Response
import re
import json
from cloudlet import telnet_connection_waiting

# global constant and variable
WEB_SERVER_PORT_NUMBER = 9091
ISR_ORIGIN_SRC_PATH = '/home/krha/cloudlet/src/ISR/src-pipe'
ISR_ANDROID_SRC_PATH = '/home/krha/cloudlet/src/ISR/src-mock'
ISR_ANDROID_PARCEL_PATH = '/home/krha/cloudlet/src/ISR/parcel'
VNC_PATH = '/home/krha/.isr/'
user_name = ''
server_address = ''
launch_start = datetime.now()
launch_end = datetime.now()
application_names = ( \
        "moped", "face", "null", "graphics", "speech", "mar", \
        "boot-moped", "boot-face", "boot-graphics", "boot-speech", "boot-mar" \
        )

# web server configuration
app = Flask(__name__)
app.config.from_object(__name__)

# web server for receiving command
@app.route("/isr", methods=['POST'])
def isr():
    global user_name
    global server_address

    print "Receive isr_info (run-type, application name) from client"
    json_data = request.form["info"]
    metadata = json.loads(json_data)

    run_type = metadata['run-type'].lower()
    application_name = metadata['application'].lower()
    
    if run_type in ("cloud", "mobile") and application_names:
        # Run application
        if run_type == "cloud":
            print "Client request : %s, %s --> connecting to %s with %s" % (run_type, application_name, server_address, user_name)
            ret = do_cloud_isr(user_name, application_name, server_address)
        elif run_type == "mobile":
            print "Client request : %s, %s --> connecting to %s with %s" % (run_type, application_name, server_address, user_name)
            ret = do_mobile_isr(user_name, application_name, server_address)
        
        if ret:
            print "SUCCESS"
            return "SUCCESS"

    ret_msg = "Wrong parameter " + run_type + ", " + application_name
    print ret_msg
    return ret_msg


def recompile_isr(src_path):
    command_str = 'cd %s && sudo make && sudo make install' % (src_path)
    print command_str
    ret1, ret_string = commands.getstatusoutput(command_str)
    if ret1 != 0:
        raise "Cannot compile ISR"
    return True


# command Login
def login(user_name, server_address):
    command_str = 'isr auth -s ' + server_address + ' -u ' + user_name
    ret, ret_string = commands.getstatusoutput(command_str)

    if ret == 0:
        return True, ''
    return False, "Cannot connected to Server %s, %s" % (server_address, ret_string)

def get_uuid(user_name, server_address, vm_name):
    # list cache
    command_str = 'isr lshoard -l -s ' + server_address + ' -u ' + user_name
    print command_str
    ret, ret_string = commands.getstatusoutput(command_str)
    if ret != 0:
        return False, "Cannot list up VM hoard"

    # find UUID, which has vm_name
    lines = ret_string.split('\n')
    uuid = None
    for index, line in enumerate(lines):
        if line.find(vm_name) != -1 and len(lines) > (index+1):
            uuid = lines[index+1].lstrip().split(" ")[0]

    return uuid


# remove all cache
def remove_cache(user_name, server_address, vm_name):
    uuid = get_uuid(user_name, server_address, vm_name)
    if uuid != None:
        # erase disk cache
        command_str = 'isr rmhoard ' + uuid + ' -s ' + server_address + ' -u ' + user_name
        ret, ret_string = commands.getstatusoutput(command_str)

    # erase memory
    '''
    mem_dir = '/home/krha/.isr/hoard/img/'
    if os.path.exists(mem_dir):
        command_str = "rm -rf %s*" %(mem_dir)
        print command_str
        ret, ret_string = commands.getstatusoutput(command_str)
    '''

    mem_dir = '/home/krha/.isr/'
    if os.path.exists(mem_dir):
        command_str = "rm -rf %s" % (mem_dir)
        print command_str
        ret, ret_string = commands.getstatusoutput(command_str)

    return True, ''


# resume VM
def resume_vm(user_name, server_address, vm_name):
    time_start = datetime.now()
    time_end = datetime.now()
    time_transfer_start = datetime.now()
    time_transfer_end = datetime.now()
    time_decomp_mem_start = datetime.now()
    time_decomp_mem_end = datetime.now()
    time_kvm_start = datetime.now()
    time_kvm_end = datetime.now()

    time_str = datetime.now().strftime("%s:%X")
    log_filename = "%s-%s-%s" % (str(server_address), str(vm_name), str(time_str))
    log_file = open(log_filename, "w")

    command_str = 'isr resume ' + vm_name + ' -s ' + server_address + ' -u ' + user_name + ' -F'
    print command_str
    print "VM Resume Process start : " + str(time_start)
    proc = subprocess.Popen(command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    time_start = datetime.now()
    while True:
        time.sleep(0.01)
        output = proc.stdout.readline()
        if len(output.strip()) != 0 and output.find("[krha]") == -1:
            sys.stdout.write(output)

        # time stamping using log from isr_client
        # Not reliable but fast for simple test
        if output.strip().find("Connecting to server") == 0:
            time_transfer_start = datetime.now()
        elif output.strip().find("[Transfer]") == 0:
            time_transfer_end = datetime.now()
            time_decomp_mem_start = datetime.now()
        #elif output.strip().find("[Decomp]") == 0:
        elif output.strip().find("Launching KVM") == 0:
            time_decomp_mem_end = datetime.now()
            time_kvm_start = datetime.now()
            break;

    # waiting for TCP socket open
    # predefined for test, it is opened at ISR/vmm/kvm
    # So, no multiple ISR Client at one machine
    telnet_port = 9998 
    for i in xrange(2000):
        command_str = "netstat -an | grep 127.0.0.1:" + str(telnet_port)
        proc = subprocess.Popen(command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        output = proc.stdout.readline()
        if output.find("LISTEN") != -1:
            break;
        time.sleep(0.01)

    # Getting VM Status information through Telnet
    telnet_connection_waiting(telnet_port)
    time_kvm_end = datetime.now()
    time_end = datetime.now()

    transfer_diff = time_transfer_end-time_transfer_start
    decomp_diff = time_decomp_mem_end-time_decomp_mem_start
    kvm_diff = time_kvm_end-time_kvm_start
    total_diff = time_end-time_start
    ret_msg = "----------------------------------------------------------\n"
    ret_msg += 'Transfer\tDecompression\tDelta apply\tVM Boot\tKVM resume\n'
    ret_msg += "%04d.%06d\t" % (transfer_diff.seconds, transfer_diff.microseconds)
    ret_msg += "%04d.%06d\t" % (decomp_diff.seconds, decomp_diff.microseconds)
    print ret_msg

    message = "Return from Resume\n"
    message += "[Time] Transfer Time      : %04d.%06d\n" % (transfer_diff.seconds, transfer_diff.microseconds)
    message += "[Time] Decomp (Overlapped): %04d.%06d\n" % (decomp_diff.seconds, decomp_diff.microseconds)
    message += "[Time] VM Resume          : %04d.%06d\n" % (kvm_diff.seconds, kvm_diff.microseconds)
    message += "[Time] Total Time         : %04d.%06d\n" % (total_diff.seconds, total_diff.microseconds)
    print message


    log_file.write(message)
    log_file.close()

    return ret_msg, kvm_diff


# stop VM
def stop_vm(user_name, server_address, vm_name):
    command_str = 'isr clean ' + vm_name + ' -s ' + server_address + ' -u ' + user_name + ' -f'
    print command_str
    proc = subprocess.Popen(command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    proc.stdin.write('y\n')
    proc.wait()

    return True

# Exit with error message
def exit_error(error_message):
    print 'Error, ', error_message
    sys.exit(1)

def do_cloud_isr(user_name, vm_name, server_address):
    # compile ISR again, because we have multiple version of ISR such as mock android
    # This is not good approach, but easy for simple test
    # I'll gonna erase this script after submission :(
    recompile_isr(ISR_ORIGIN_SRC_PATH)

    # step1. login
    ret, err = login(user_name, server_address)
    if not ret:
        print err
        return False

    # step2. remove all cache
    '''
    ret, err = remove_cache(user_name, server_address, vm_name)
    if not ret:
        return False
    '''

    # step3. resume VM, wait until finish (close window)
    ret_message, kvm_time = resume_vm(user_name, server_address, vm_name)

    return True, ret_message, kvm_time


def do_mobile_isr(user_name, vm_name, server_address):
    # compile ISR again, because we have multiple version of ISR such as mock android
    # This is not good approach, but easy for simple test
    # I'll gonna erase this script after submission :(
    recompile_isr(ISR_ANDROID_SRC_PATH)

    # Change parcel to indicate it to mobile address, which is specified at server_address
    trick_parcel_address(ISR_ANDROID_PARCEL_PATH, vm_name, server_address)

    # step1. remove all cache
    '''
    ret, err = remove_cache(user_name, server_address, vm_name)
    if not ret:
        return False
    '''

    # step2. resume VM, wait until finish (close window)
    ret_message, kvm_time = resume_vm(user_name, server_address, vm_name)

    return True, ret_message, kvm_time


def trick_parcel_address(parcel_dir, vm_name, server_address):
    parcel_path = os.path.join(parcel_dir, vm_name, 'parcel.cfg')
    print parcel_path
    if not os.path.exists(parcel_path):
        print "Error, check you parcel file location : " + parcel_path
        sys.exit(2)
        return False
    lines = []
    fr = open(parcel_path, 'r')
    for line in fr:
        key = line.split("=")[0].strip()
        if key == "RPATH":
            lines.append("RPATH = http://" + server_address + ":80\n")
        elif key == "SERVER":
            lines.append("SERVER = " + server_address + "\n")
        else:
            lines.append(line)
    fr.close()
    fw = open(parcel_path, 'w')
    fw.write(''.join(lines))
    fw.close()


def isr_clean_all(server_address, user_name):
    global application_names

    # kill all process that has 'isr'
    # BAD, but it is almost only way that I can clean cache
    command_str = 'ps aux | grep isr'
    ret1, ret_string = commands.getstatusoutput(command_str)
    for line in ret_string.split('\n'):
        if line.find('isr') != -1 and line.find('isr_run.py') == -1 and line.find('vi ') == -1:
            pid = re.search('[A-Za-z]+\s+(\d+).*', line).groups(0)[0]
            command_str = 'kill -9 ' + pid
            print 'kill /isr + \t', command_str
            commands.getoutput(command_str)

    for vm_name in application_names:
        ret = stop_vm(user_name, server_address, vm_name)
        ret = remove_cache(user_name, server_address, vm_name)

def print_usage(program_name):
    print 'usage\t: %s [run|clean] [-u username] [-s server_address] ' % program_name
    print 'example\t: isr_run.py run -u cloudlet -s dagama.isr.cs.cmu.edu'


def main(argv):
    global user_name
    global server_address

    if len(argv) < 3:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    operation = argv[1].lower()
    if not operation in ("clean", "run"):
        print "No supporing operation : ", operation
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    try:
        optlist, args = getopt.getopt(argv[2:], 'hu:s:', ["help", "user", "server"])
    except getopt.GetoptError, err:
        print str(err)
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    # required input variables
    user_name = None
    server_address = None

    # parse argument
    for o, a in optlist:
        if o in ("-h", "--help"):
            print_usage(os.path.basename(argv[0]))
            sys.exit(0)
        elif o in ("-u", "--user"):
            user_name = a
        elif o in ("-s", "--server"):
            server_address = a
        else:
            assert False, "unhandled option"

    if user_name == None or server_address == None:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    # Always strat from the clean state
    isr_clean_all(server_address, user_name)
    if operation == "clean":
        sys.exit(0)


if __name__ == "__main__":
    main(sys.argv)
    #do_mobile_isr(user_name, "moped", server_address)
    app.run(host='0.0.0.0', port=WEB_SERVER_PORT_NUMBER, processes = 10)

