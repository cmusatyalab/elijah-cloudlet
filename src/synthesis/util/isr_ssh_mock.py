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

import os, commands, filecmp, sys, subprocess, getopt, time
from datetime import datetime, timedelta
import hashlib

PARCEL_ROOT = '/home/krha/cloudlet/src/ISR/parcel'
SERVER_ROOT = '/home/krha/cloudlet/src/ISR/web'

def printout_stat(stat, statfile):
    ret_string = 'DEV=' + str(stat.st_dev) + '\n'
    ret_string = ret_string + 'INO=' + str(stat.st_ino) + '\n'
    ret_string = ret_string + 'SIZE=' + str(stat.st_size) + '\n'
    ret_string = ret_string + 'MODE=' + str(stat.st_mode) + '\n'
    ret_string = ret_string + 'NLINK=' + str(stat.st_nlink) + '\n'
    ret_string = ret_string + 'UID=' + str(stat.st_uid) + '\n'
    ret_string = ret_string + 'GID=' + str(stat.st_gid) + '\n'
    ret_string = ret_string + 'RDEV=' + str(stat.st_rdev) + '\n'
    ret_string = ret_string + 'SIZE=' + str(stat.st_size) + '\n'
    ret_string = ret_string + 'ATIME=' + str(stat.st_atime) + '\n'
    ret_string = ret_string + 'MTIME=' + str(stat.st_mtime) + '\n'
    ret_string = ret_string + 'CTIME=' + str(stat.st_ctime) + '\n'
    ret_string = ret_string + 'BLKSIZE=' + str(stat.st_blksize) + '\n'
    ret_string = ret_string + 'BLOCKS=' + str(stat.st_blocks) + '\n'

    '''
    h = hashlib.sha1()
    h.update(statfile.read())
    hash_value = h.hexdigest()
    '''
    ret_string = ret_string + 'SHA1=' + 'd3d67358ce505866504ba489d00c888be58d2581'

    return ret_string

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
    print '\toperation list\n'
    print '\tls'
    print '\tmotd'
    print '\tgetconfig'
    print '\tlock'
    print '\tstat'
    print '\tcatlog'
    print '\tcheckparcel'
    print '\tcommit'
    print '\trollback'

def main(argv):
    if len(argv) < 2:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    operation = argv[1]
    operation_list = ("ls", "motd", "getconfig", "lock", "stat")
    if operation not in operation_list:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    try:
        optlist, args = getopt.getopt(argv[2:], 'hp:u:s:p:arn:f:C:', ["help", "user", "server", "vmname", "acquire", "hostname", "filepath"])
    except getopt.GetoptError, err:
        print str(err)
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    # required input variables
    user_name = None
    server_address = None
    vm_name = None
    acquire = False
    release = False
    host_name = None
    filename = None

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
        elif o in ("-a", "--acquire"):
            acquire = True;
        elif o in ("-r", "--release"):
            release = True;
        elif o in ("-n", "--hostname"):
            host_name = a
        elif o in ("-f", "--filepath"):
            filename = a
        elif o in ("-C"):
            a
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
        if vm_name == None:
            print "error, No VM Name is specifoed"
            sys.exit(2)
        parcel_path = os.path.join(PARCEL_ROOT, vm_name, 'parcel.cfg')
        if os.path.exists(parcel_path) == False:
            print "error, No such Path for parcel : ", parcel_path
            sys.exit(2)
        parcel_file = open(parcel_path, "r")
        ret_string = parcel_file.read()
        sys.stdout.write(ret_string)
        sys.exit(0)
    elif operation in ("lock"):
        # always give face lock
        if acquire == True:
            sys.stdout.write("lock acquired")
        sys.exit(0)
    elif operation in ("stat"):
        # returns file status
        if filename == None:
            print "error, no file path file stats"
            sys.exit(2)
        file_path = os.path.join(SERVER_ROOT, 'keyring.enc')
        if os.path.exists(file_path) == False:
            print "error, No such file for stat : ", file_path
            sys.exit(2)
        stat = os.stat(file_path)
        statfile = open(file_path, 'r')
        ret_string = printout_stat(stat, statfile)
        sys.stdout.write(ret_string)
        sys.exit(0)
    else:
        print "error, No such command : " + operation
        sys.exit(2)

    sys.exit(0)


if __name__ == "__main__":
    main(sys.argv)
