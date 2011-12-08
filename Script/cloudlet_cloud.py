#!/usr/bin/env python
import os
import getopt
import commands
import sys
import subprocess
from datetime import datetime, timedelta
from flask import Flask, flash, request,render_template, Response,session,g
from flask import Flask, request, session, g, redirect, url_for, abort, render_template, flash, Response
import json
from cloudlet import run_snapshot, stop_vm, recover_snapshot

# Global constant
# VM Overlay List
WEB_SERVER_URL = 'http://dagama.isr.cs.cmu.edu/cloudlet'
MOPED_DISK = WEB_SERVER_URL + '/moped_2048/ubuntu_base_overlay.qcow2.lzma'
MOPED_MEM = WEB_SERVER_URL + '/moped_2048/ubuntu_base_overlay.mem.lzma'
FACE_DISK = WEB_SERVER_URL + '/FACE_2048/winxp-with-python_base_overlay.qcow2.lzma'
FACE_MEM = WEB_SERVER_URL + '/FACE_2048/winxp-with-python_base_overlay.mem.lzma'
NULL_DISK = WEB_SERVER_URL + '/NULL_2048/ubuntu_base_overlay.qcow2.lzma'
NULL_MEM = WEB_SERVER_URL + '/NULL_2048/ubuntu_base_overlay.mem.lzma'

WEB_SERVER_PORT_NUMBR = 9096
VM_TELNET_COMMAND_PORT_NUMBER = 19999
bandwidth = 0
vm_name = ''

# BASE VM PATH
MOPED_BASE_DISK = '/home/krha/Cloudlet/image/MOPED_BaseVM/ubuntu_base.qcow2'
MOPED_BASE_MEM = '/home/krha/Cloudlet/image/MOPED_BaseVM/ubuntu_base.mem'
FACE_BASE_DISK = '/home/krha/Cloudlet/image/FACE_BaseVM/winxp-with-python_base.qcow2'
FACE_BASE_MEM = '/home/krha/Cloudlet/image/FACE_BaseVM/winxp-with-python_base.mem'
NULL_BASE_DISK = '/home/krha/Cloudlet/image/NULL_ubuntu/ubuntu_base.qcow2'
NULL_BASE_MEM = '/home/krha/Cloudlet/image/NULL_ubuntu/ubuntu_base.mem'


# Web Server configuration
app = Flask(__name__)
app.config.from_object(__name__)

# Web Server for receiving command
@app.route('/cloudlet_from_cloud', methods=['POST'])
def cloudlet_from_cloud():
    global vm_name
    print "Receive cloudlet downloading request"
    json_data = request.form["cloudlet_info"]
    metadata = json.loads(json_data)

    vm_name = metadata['vm_name'].lower()
    print "Client request : %s " % (vm_name)

    ## execute
    disk_name, mem_name = download_overaly(vm_name)
    return "SUCCESS"


def execute_download_process(source, dest):
    global bandwidth
    command_str = "wget --limit-rate=" + str(bandwidth) + " -O " + dest +  " " + source
    proc = subprocess.Popen(command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    output = proc.stdout.readline()
    if len(output.strip()) != 0:
        print output
    proc.wait()
    return True

def download_overaly(machine_name, telnet_por):
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

    dest_disk = os.path.join('/tmp/', os.path.basename(url_disk))
    dest_mem = os.path.join('/tmp/', os.path.basename(url_mem))
    prev_time = datetime.now()
    ret1 = execute_download_process(url_disk, dest_disk)
    ret2 = execute_download_process(url_mem, dest_mem)
    if ret1 != True or ret2 != True:
        return '', ''
    print "[krha] overlay transfer time: " + str(datetime.now() - prev_time)

    recover_img, recover_mem = recover_snapshot(base_disk, base_mem, dest_disk, dest_mem)
    prev_time = datetime.now()
    run_snapshot(recover_img, recover_mem, VM_TELNET_COMMAND_PORT_NUMBER, 5, wait_vnc_end=False)
    print '[Time] Run Snapshot - ', str(datetime.now()-prev_time)

    return dest_disk, dest_mem


def print_usage(program_name):
    print 'usage\t: %s [run|clean] [-b bandwidth(Mbps)]' % program_name
    print 'example\t: ./cloudlet_cloud.py run -b 10 '


def main(argv):
    global server_address
    global bandwidth

    if len(argv) < 2:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    operation = argv[1].lower()
    if not operation in ("clean", "run"):
        print "No supporing operation : ", operation
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    try:
        optlist, args = getopt.getopt(argv[2:], 'b:', ["bandwidth"])
    except getopt.GetoptError, err:
        print str(err)
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)


    # operation handling
    if operation == "clean":
        stop_vm(VM_TELNET_COMMAND_PORT_NUMBER)
    elif operation == "run":
        # parse argument
        for o, a in optlist:
            if o in ("-b", "--bandwidth"):
                # bandwidth convertion from Mbps Kbyte/s
                bandwidth = (int)(int(a)/8.0*1000*1000)

        # required input variables
        if bandwidth <= 0:
            print "Invalid bandwidth, it must be bigger than 0"
            sys.exit(2)

        download_overaly('null', VM_TELNET_COMMAND_PORT_NUMBER) 
        #app.run(host='0.0.0.0', port=WEB_SERVER_PORT_NUMBER, processes=10)


if __name__ == "__main__":
    main(sys.argv)
