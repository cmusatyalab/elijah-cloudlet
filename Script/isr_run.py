#!/usr/bin/env python
import xdelta3
import os, commands, filecmp, sys, subprocess, getopt, time
from datetime import datetime, timedelta
import telnetlib
import pylzma

LAUNCH_COMMAND = 'Launching KVM...'
launch_start = datetime.now()
launch_end = datetime.now()


# Limiting Up/Down traffic bandwidth
def bandwidth_limit(bandwidth):
    command_str = 'sudo wondershaper eth0 ' + str(bandwidth) + ' ' + str(bandwidth)
    print command_str
    ret, ret_string = commands.getstatusoutput(command_str)
    print ret_string


# Reset traffic bandwidth limitation
def bandwidth_reset():
    command_str = 'sudo wondershaper clear eth0'
    print command_str
    ret, ret_string = commands.getstatusoutput(command_str)
    print ret_string


# command Login
def login(user_name, server_address):
    return True, ''
'''
    command_str = 'isr auth -s ' + server_address + ' -u ' + user_name
    ret, ret_string = commands.getstatusoutput(command_str)

    if ret == 0:
        return True, ''
    return False, "Cannot connected to Server %s, %s" % (server_address, ret_string)
'''

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
    command_str = 'isr resume ' + vm_name + ' -s ' + server_address + ' -u ' + user_name + ' -D -F'
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
    bandwidth_reset()
    sys.exit(1)


def print_usage(program_name):
    print 'usage\t: %s [-b Bandwidth kb/s] [-u username] [-s server_address] [-m VM name]' % program_name
    print 'example\t: ./isr_run.py -u test1 -s dagama.isr.cs.cmu.edu -m windowXP -b 100000'


def main(argv):
    if len(argv) < 2:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)
    try:
        optlist, args = getopt.getopt(argv[1:], 'hb:u:s:m:', ["help", "bandwidth", "user", "server", "machine"])
    except getopt.GetoptError, err:
        print str(err)
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    # required input variables
    user_name = None
    server_address = None
    vm_name = None
    bandwidth = -1

    # parse argument
    for o, a in optlist:
        if o in ("-h", "--help"):
            print_usage(os.path.basename(argv[0]))
            sys.exit(0)
        elif o in ("-b", "--bandwidth"):
            bandwidth = int(a)
        elif o in ("-u", "--user"):
            user_name = a
        elif o in ("-s", "--server"):
            server_address = a
        elif o in ("-m", "--machine"):
            vm_name = a
        else:
            assert False, "unhandled option"

    if user_name == None or server_address == None or vm_name == None or bandwidth == -1:
        print_usage(os.path.basename(argv[0]))
        print "username : %s, server_address = %s, vm_name = %s" % (user_name, server_address, vm_name)
        sys.exit(2)

    # setup bandwidth limitation
    bandwidth_limit(bandwidth)

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
    if ret == False:
        exit_error(err)

    print "SUCCESS"
    print "[Total VM Run Time] : ", str(end_time-start_time)
    print '[Launch Time] : ', str(launch_end-launch_start)
    bandwidth_reset()
    sys.exit(0)


if __name__ == "__main__":
    main(sys.argv)
