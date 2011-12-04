#!/usr/bin/env python
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
import json

# global constant and variable
WEB_SERVER_PORT_NUMBER = 9095
ISR_ORIGIN_SRC_PATH = '/home/krha/Cloudlet/src/ISR/src'
ISR_ANDROID_SRC_PATH = '/home/krha/Cloudlet/src/ISR/src-mock'
user_name = ''
server_address = ''
launch_start = datetime.now()
launch_end = datetime.now()

# web server configuration
app = Flask(__name__)
app.config.from_object(__name__)

# web server for receiving command
@app.route("/isr", methods=['POST'])
def isr():
    global user_name
    global server_address

    print "Receive isr_info (run-type, application name) from client"
    json_data = request.form["isr_info"]
    metadata = json.loads(json_data)

    run_type = metadata['run-type'].lower()
    application_name = metadata['application'].lower()
    if run_type in ("cloud", "mobile") and  application_name in ("moped", "face", "null"):
        # Run application
        if run_type == "cloud":
            print "Client request : %s, %s --> connecting to %s with %s" % (run_type, application_name, server_address, user_name)
            do_cloud_isr(user_name, application_name, server_address)
        elif run_type == "mobile":
            print "Client request : %s, %s --> connecting to %s with %s" % (run_type, application_name, server_address, user_name)
            do_mobile_isr(user_name, application_name, server_address)
        
        print "SUCCESS"
        return json.dumps('SUCCESS')
   
    return json.dump("False run_type " + run_type)


def recompile_isr(src_path):
    command_str = 'cd %s && sudo make && sudo make install' % (src_path)
    print command_str
    ret1, ret_string = commands.getstatusoutput(command_str)
    if ret1 != 0:
        raise "Cannot compile ISR"
    return True

# Traffic shaping is not working for ingress traffic.
# So this must be done at server side.
# You can restric traffic between server and client using traffic_shaping script at "SERVER SIDE"
'''
# Limiting Up/Down traffic bandwidth
def bandwidth_limit(bandwidth, dest_ip):
    bandwidth = bandwidth / 4.0 / 10.0; # tc module is not accuracy especially for download

    command_str = 'sudo tc qdisc add dev eth0 root handle 1: htb default 30'
    ret1, ret_string = commands.getstatusoutput(command_str)
    command_str = 'sudo tc class add dev eth0 parent 1: classid 1:1 htb rate ' + str(bandwidth) + 'mbit'
    ret2, ret_string = commands.getstatusoutput(command_str)
    command_str = 'sudo tc class add dev eth0 parent 1: classid 1:2 htb rate ' + str(bandwidth) + 'mbit'
    ret3, ret_string = commands.getstatusoutput(command_str)
    command_str = 'sudo tc filter add dev eth0 protocol ip parent 1:0 prio 1 u32 match ip dst ' + dest_ip + '/32 flowid 1:1'
    ret4, ret_string = commands.getstatusoutput(command_str)
    command_str = 'sudo tc filter add dev eth0 protocol ip parent 1:0 prio 1 u32 match ip src ' + dest_ip + '/32 flowid 1:2'
    ret5, ret_string = commands.getstatusoutput(command_str)

    if ret1 != 0 or ret2 != 0 or ret3 !=0 or ret4 !=0 or ret5 !=0:
        print 'Error, BW is not limited'
        return False
    print 'BW is limited to %s Mbit/s between localhost and %s' % (str(bandwidth*10*4.0), dest_ip)
    return True


# Reset traffic bandwidth limitation
def bandwidth_reset():
    command_str = 'sudo tc qdisc del dev eth0 root'
    ret, ret_string = commands.getstatusoutput(command_str)
    print 'BW restriction is cleared'
    return ret_string
'''

# command Login
def login(user_name, server_address):
    command_str = 'isr auth -s ' + server_address + ' -u ' + user_name
    ret, ret_string = commands.getstatusoutput(command_str)

    if ret == 0:
        return True, ''
    return False, "Cannot connected to Server %s, %s" % (server_address, ret_string)


# remove all cache
def remove_cache(user_name, server_address, vm_name):
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

    if uuid == None:
        return True, ''
    
    # erase cache
    command_str = 'isr rmhoard ' + uuid + ' -s ' + server_address + ' -u ' + user_name
    print command_str
    ret, ret_string = commands.getstatusoutput(command_str)

    return True


# resume VM, wait until finish (close window)
def resume_vm(user_name, server_address, vm_name):
    time_start = datetime.now()
    time_end = datetime.now()
    time_transfer_start = datetime.now()
    time_transfer_end = datetime.now()
    time_decomp_mem_start = datetime.now()
    time_decomp_mem_end = datetime.now()
    time_kvm_start = datetime.now()
    time_kvm_end = datetime.now()

    command_str = 'isr resume ' + vm_name + ' -s ' + server_address + ' -u ' + user_name + ' -F'
    print command_str
    time_start = datetime.now()
    proc = subprocess.Popen(command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    while True:
        time.sleep(0.1)
        output = proc.stdout.readline()
        if len(output.strip()) != 0 and output.find("[krha]") == -1:
            sys.stdout.write(output)

        # time stamping using log from isr_client
        # Not reliable but fast for simple test
        if output.strip().find("Fetching keyring") == 0:
            time_transfer_start = datetime.now()
        elif output.strip().find("Decrypting and uncompressing") == 0:
            time_transfer_end = datetime.now()
            time_decomp_mem_start = datetime.now()
        elif output.strip().find("Updating hoard cache") == 0:
            time_decomp_mem_end = datetime.now()
            time_kvm_start = datetime.now()
        elif output.strip().find("Launching KVM") == 0:
            time_kvm_end = datetime.now()
            break;

    ret = proc.wait()
    time_end = datetime.now()
    if ret == 0:
        return True, str(time_end-time_start), str(time_transfer_end-time_transfer_start), str(time_decomp_mem_end-time_decomp_mem_start), str(time_kvm_end-time_kvm_start)
    return False, '','',''


# stop VM
def stop_vm(user_name, server_address, vm_name):
    command_str = 'isr clean ' + vm_name + ' -s ' + server_address + ' -u ' + user_name
    print command_str
    proc = subprocess.Popen(command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    proc.stdin.write('y\n')
    ret = proc.wait()

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
    if ret == False:
        exit_error(err)

    # step2. remove all cache
    ret, err = remove_cache(user_name, server_address, vm_name)
    if ret == False:
        exit_error(err)

    # step3. resume VM, wait until finish (close window)
    start_time = datetime.now()
    ret, total_time, transfer_time, decomp_time, kvm_time = resume_vm(user_name, server_address, vm_name)
    if ret == False:
        exit_error(err)
    end_time = datetime.now()

    # step4. clean all state
    ret = stop_vm(user_name, server_address, vm_name)
    ret = remove_cache(user_name, server_address, vm_name)

    print "SUCCESS"
    print "[Total Time] : ", total_time 
    print '[Transfer Time(Memory)] : ', transfer_time
    print '[Decompression Time] : ', decomp_time
    print '[KVM Time] : ', kvm_time


def do_mobile_isr(user_name, vm_name, server_address):
    # compile ISR again, because we have multiple version of ISR such as mock android
    # This is not good approach, but easy for simple test
    # I'll gonna erase this script after submission :(
    recompile_isr(ISR_ANDROID_SRC_PATH)

    # step2. remove all cache
    ret, err = remove_cache(user_name, server_address, vm_name)
    if ret == False:
        exit_error(err)

    # step3. resume VM, wait until finish (close window)
    start_time = datetime.now()
    rret, total_time, transfer_time, decomp_time, kvm_time = resume_vm(user_name, server_address, vm_name)
    if ret == False:
        exit_error(err)
    end_time = datetime.now()

    # step4. clean all state
    ret = stop_vm(user_name, server_address, vm_name)
    ret = remove_cache(user_name, server_address, vm_name)

    print "SUCCESS"
    print "[Total Time] : ", total_time 
    print '[Memory Transfer Time] : ', transfer_time
    print '[Decompression Time] : ', decomp_time
    print '[KVM Time] : ', kvm_time


def isr_clean_all(server_address, user_name):
    vm_names = ("face", "moped", "null")
    for vm_name in vm_names:
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

    if operation == "clean":
        isr_clean_all(server_address, user_name)
        sys.exit(0)


if __name__ == "__main__":
    main(sys.argv)
    app.run(host='0.0.0.0', port=WEB_SERVER_PORT_NUMBER, processes = 10)
