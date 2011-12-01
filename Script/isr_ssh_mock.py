#!/usr/bin/env python
import os, commands, filecmp, sys, subprocess, getopt, time
from datetime import datetime, timedelta
import pylzma

PARCEL_ROOT = /home/krha/Cloudlet/src/ISR/parcel

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
    print 'motd'
    print 'getconfig'
    print 'lock'
    print 'stat'
    print 'catlog'
    print 'checkparcel'
    print 'commit'
    print 'rollback'

def main(argv):
    if len(argv) < 2:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    operation = argv[1]
    operation_list = ("ls", "motd", "getconfig", "lock", "stat")
    if operation_cmd not in operation_list:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    try:
        optlist, args = getopt.getopt(argv[2:], 'hp:u:s:p:', ["help", "user", "server", "vmname"])
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
        elif o in ("-p", "--vmname"):
            vm_name = a
        else:
            assert False, "unhandled option"

    if operation in ("ls"):
        # let's say it is alwasy true
        sys.exit(0)
    elif operation in ("motd"):
        # I don't know what this command means yet,
        # will finish it later
        sys.exit(0)
    elif operation in ("getconfig"):
        # return configuration file
        if vm_name = None:
            print "error, No VM Name is specifoed"
            sys.exit(2)
        parcel_path = os.path.join(PARCEL_ROOT, vm_name, 'parcel.cgf')
        if os.path.exists(parcel_path) == False:
            print "error, No such Path for parcel : ", parcel_path
            sys.exit(2)
        parcel_file = open(parcel_path, "r")
        ret_string = parcel_file.read()
        print ret_string
        sys.exit(0)
    elif operation in ("lock"):
    elif operation in ("stat"):
    else:
        print "error, No such command : " + operation
        sys.exit(2)

    sys.exit(0)


if __name__ == "__main__":
    main(sys.argv)
