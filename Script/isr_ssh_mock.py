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


# Exit with error message
def exit_error(error_message):
    print 'Error, ', error_message
    bandwidth_reset()
    sys.exit(1)


def print_usage(program_name):
    print 'usage\t: %s operation [-p parcelname]' % program_name
    print 'operation list\n\n'
    print 'ls'
    print 'catlog'
    print 'checkparcel'
    print 'commit'
    print 'getconfig'
    print 'lock'
    print 'ls'
    print 'motd'
    print 'rollback'
    print 'stat'


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
