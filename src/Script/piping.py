#!/usr/bin/env python
import xdelta3
import os
import commands
import filecmp
import sys
import subprocess
import getopt
import time
from datetime import datetime, timedelta
import telnetlib
import socket
import pylzma
import optparse
from optparse import OptionParser

application_names = ("moped", "face", "speech", "null")
WEB_SERVER_URL = 'http://dagama.isr.cs.cmu.edu/cloudlet'

# Overlya URL
MOPED_DISK = WEB_SERVER_URL + '/overlay/moped/overlay1/moped.qcow2.lzma'
MOPED_MEM = WEB_SERVER_URL + '/overlay/moped/overlay1/moped.mem.lzma'
FACE_DISK = WEB_SERVER_URL + '/overlay/face/overlay1/face.qcow2.lzma'
FACE_MEM = WEB_SERVER_URL + '/overlay/face/overlay1/face.mem.lzma'
SPEECH_DISK = WEB_SERVER_URL + '/overlay/speech/overlay1/speech.qcow2.lzma'
SPEECH_MEM = WEB_SERVER_URL + '/overlay/speech/overlay1/speech.mem.lzma'
NULL_DISK = WEB_SERVER_URL + '/overlay/null/overlay1/null.qcow2.lzma'
NULL_MEM = WEB_SERVER_URL + '/overlay/null/overlay1/null.mem.lzma'
# BASE VM PATH
MOPED_BASE_DISK = '/home/krha/Cloudlet/image/Ubuntu10_Base/ubuntu_base.qcow2'
MOPED_BASE_MEM = '/home/krha/Cloudlet/image/Ubuntu10_Base/ubuntu_base.mem'
NULL_BASE_DISK = MOPED_BASE_DISK
NULL_BASE_MEM = MOPED_BASE_MEM
FACE_BASE_DISK = '/home/krha/Cloudlet/image/WindowXP_Base/winxp-with-jre7_base.qcow2'
FACE_BASE_MEM = '/home/krha/Cloudlet/image/WindowXP_Base/winxp-with-jre7_base.mem'
SPEECH_BASE_DISK = FACE_BASE_DISK
SPEECH_BASE_MEM = FACE_BASE_MEM

def get_download_url(machine_name):
    url_disk = ''
    url_mem = ''
    base_disk = ''
    base_mem = ''
    if machine_name.lower() == "moped":
        url_disk = MOPED_DISK
        url_mem = MOPED_MEM
        base_disk = MOPED_BASE_DISK
        base_mem = MOPED_BASE_MEM
    elif machine_name.lower() == "face":
        url_disk = FACE_DISK
        url_mem = FACE_MEM
        base_disk = FACE_BASE_DISK
        base_mem = FACE_BASE_MEM
    elif machine_name.lower() == "null":
        url_disk = NULL_DISK
        url_mem = NULL_MEM
        base_disk = NULL_BASE_DISK
        base_mem = NULL_BASE_MEM
    elif machine_name.lower() == "speech":
        url_disk = SPEECH_DISK
        url_mem = SPEECH_MEM
        base_disk = SPEECH_BASE_DISK
        base_mem = SPEECH_BASE_MEM

    return url_disk, url_mem, base_disk, base_mem

def piping_synthesis(vm_name, bandwidth):
    disk_url, mem_url, base_disk, base_mem = get_download_url(vm_name)
    dest_disk = os.path.join('/tmp/', os.path.basename(disk_url))
    dest_mem = os.path.join('/tmp/', os.path.basename(mem_url))
    # download
    command_str = "wget --limit-rate=" + str(bandwidth) + " -O " + dest_disk +  " " + disk_url
    print command_str 
    proc = subprocess.Popen(command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    output = proc.stdout.readline()
    if len(output.strip()) != 0:
        print output
    proc.wait()
    return 0


def process_command_line(argv):
    parser = OptionParser()
    parser.add_option(
            '-b', '--bandwidth', action='store', type='int', dest='bandwidth', default=100,
            help='Set bandwidth for receiving overlay VM (Mbps).')
    parser.add_option(
            '-n', '--name', action='store', type='string', dest='vmname',
            help='Set VM name')

    settings, args = parser.parse_args(argv)

    if args:
        parser.error('program takes no command-line arguments; '
                '"%s" ignored. ' % (args,))
    if settings.vmname == None or settings.vmname not in application_names:
        parser.error('program need vm name to run. (ex. moped, speech, face)')

    return settings, args

def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    piping_synthesis(settings.vmname, settings.bandwidth*1000*1000)
    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
