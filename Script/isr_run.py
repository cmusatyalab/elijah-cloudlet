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

ISR_ORIGIN_SRC_PATH = '/home/krha/Cloudlet/src/ISR/src'
ISR_ANDROID_SRC_PATH = '/home/krha/Cloudlet/src/ISR/src-mock'
LAUNCH_COMMAND = 'Launching KVM...'
launch_start = datetime.now()
launch_end = datetime.now()

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

    if ret != 0:
        return False, "Cannot remove hoard, %s, %s" % (uuid, vm_name)
    return True, ''


# resume VM, wait until finish (close window)
def resume_vm(user_name, server_address, vm_name):
    global launch_start
    global launch_end
    command_str = 'isr resume ' + vm_name + ' -s ' + server_address + ' -u ' + user_name + ' -F'
    print command_str
    launch_start = datetime.now()
    print 'launch start : ', str(launch_start)
    proc = subprocess.Popen(command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    while True:
        time.sleep(0.1)
        output = proc.stdout.readline()
        if len(output.strip()) != 0 and output.find("[krha]") == -1:
            sys.stdout.write(output)
        if output.strip().find(LAUNCH_COMMAND) == 0:
            launch_end = datetime.now()
            print 'launch_end : ', str(launch_end)
            break;

    ret = proc.wait()
    if ret == 0:
        return True, ''
    return False, 'Failed to Resume VM'


# stop VM
def stop_vm(user_name, server_address, vm_name):
    command_str = 'isr clean ' + vm_name + ' -s ' + server_address + ' -u ' + user_name
    print command_str
    proc = subprocess.Popen(command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    proc.stdin.write('y\n')
    ret = proc.wait()

    if ret == 0:
        return True, ''
    return False, "Cannot clean up resumed VM"


# Exit with error message
def exit_error(error_message):
    print 'Error, ', error_message
    sys.exit(1)


def print_usage(program_name):
    print 'usage\t: %s [cloud|mobile] [-u username] [-s server_address] [-m VM name]' % program_name
    print 'example\t: ./isr_run.py cloud -u cloudlet -s dagama.isr.cs.cmu.edu -m face'

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
    ret, err = resume_vm(user_name, server_address, vm_name)
    if ret == False:
        exit_error(err)
       
    end_time = datetime.now()

    # step4. stop VM and clean up
    ret, err = stop_vm(user_name, server_address, vm_name)
    ret, err = remove_cache(user_name, server_address, vm_name)
    if ret == False:
        exit_error(err)

    print "SUCCESS"
    print "[Total VM Run Time] : ", str(end_time-start_time)
    print '[Launch Time] : ', str(launch_end-launch_start)

    sys.exit(0)


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
    ret, err = resume_vm(user_name, server_address, vm_name)
    if ret == False:
        exit_error(err)
       
    end_time = datetime.now()

    # step4. stop VM and clean up
    ret, err = stop_vm(user_name, server_address, vm_name)
    ret, err = remove_cache(user_name, server_address, vm_name)
    if ret == False:
        exit_error(err)

    print "SUCCESS"
    print "[Total VM Run Time] : ", str(end_time-start_time)
    print '[Launch Time] : ', str(launch_end-launch_start)

    sys.exit(0)

    pass


def main(argv):
    if len(argv) < 3:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    target = ("cloud", "mobile")
    operation = argv[1]
    if not operation in target:
        print "Error, specify between clould and mobile"
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    try:
        optlist, args = getopt.getopt(argv[2:], 'hu:s:m:', ["help", "user", "server", "machine"])
    except getopt.GetoptError, err:
        print str(err)
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    # required input variables
    user_name = None
    server_address = None
    vm_name = None

    # parse argument
    for o, a in optlist:
        if o in ("-h", "--help"):
            print_usage(os.path.basename(argv[0]))
            sys.exit(0)
        elif o in ("-u", "--user"):
            user_name = a
        elif o in ("-s", "--server"):
            server_address = a
        elif o in ("-m", "--machine"):
            vm_name = a
        else:
            assert False, "unhandled option"

    if user_name == None or server_address == None or vm_name == None:
        print_usage(os.path.basename(argv[0]))
        print "username : %s, server_address = %s, vm_name = %s" % (user_name, server_address, vm_name)
        sys.exit(2)
    
    if operation == "cloud":
        do_cloud_isr(user_name, vm_name, server_address)
    elif operation == "mobile":
        do_mobile_isr(user_name, vm_name, server_address)

    else:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    sys.exit(1)

if __name__ == "__main__":
    main(sys.argv)
