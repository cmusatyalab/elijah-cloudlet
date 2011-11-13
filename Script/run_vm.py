#!/usr/bin/env python
import os, commands, filecmp
import sys, getopt
from datetime import datetime, timedelta

CUR_DIR = os.getcwd()

def run_snapshot(disk_image, memory_image):
    command_str = "kvm -hda "
    command_str += disk_image
    command_str += " -m 512 -monitor stdio -enable-kvm -net nic -net user -serial none -parallel none -usb -usbdevice tablet -redir tcp:2222::22 "
    command_str += " -incoming \"exec:cat " + memory_image + "\""
#    print "[Debug] command : ", command_str

    ret = commands.getoutput(command_str)
#    print "[Debug] run command : ", ret

def run_image(disk_image):
    command_str = "kvm -hda "
    command_str += disk_image
    command_str += " -m 512 -monitor stdio -enable-kvm -net nic -net user -serial none -parallel none -usb -usbdevice tablet -redir tcp:2222::22 "
    print "[Debug] command : ", command_str

    ret = commands.getoutput(command_str)
#    print "[Debug] run command : ", ret

def print_help(program_name):
    print 'help: %s...' % program_name

def print_usage(program_name):
    print 'usage: %s [option].. [file]..  ' % program_name
    print ' -h, --help  print help'
    print ' -d, --disk [disk image]'
    print ' -m, --memory [memory image]'

def main(argv):
    try:
        optlist, args = getopt.getopt(argv[1:], 'hd:m:', ["help", "disk", "memory"])
    except getopt.GetoptError, err:
        print str(err)
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    # parse argument
    vm_image = None
    vm_memory = None
    for o, a in optlist:
        if o in ("-h", "--help"):
            print_help(os.path.basename(argv[0]))
        elif o in ("-d", "--disk"):
            vm_image = a
        elif o in ("-m", "--memory"):
            vm_memory = a
        else:
            assert False, "unhandled option"

    if len(optlist) == 0:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)


    # run snapshot
    if vm_memory != None:
        if vm_image == None:
            assert False, "Input disk image path"
        run_snapshot(vm_image, vm_memory)
    #run image
    elif vm_image != None:
        run_image(vm_image)



if __name__ == "__main__":
    main(sys.argv)
