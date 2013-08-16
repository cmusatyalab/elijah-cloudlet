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

import os
import getopt
import urllib2
import commands
import sys
from multiprocessing import Process, Queue, Pipe, JoinableQueue
import subprocess
from datetime import datetime, timedelta
from flask import Flask, flash, request,render_template, Response,session,g
from flask import Flask, request, session, g, redirect, url_for, abort, render_template, flash, Response
import json
from cloudlet import run_snapshot, stop_vm, recover_snapshot
from cloudlet import network_worker, decomp_worker, delta_worker

CHUNK_SIZE = 1024*16
END_OF_FILE = "Overlay Transfer End Marker"

# Global constant
# VM Overlay List
WEB_SERVER_URL = 'http://dagama.isr.cs.cmu.edu'
MOPED_DISK = WEB_SERVER_URL + '/cloudlet/overlay/ubuntu11.10-server/moped/ubuntu-11.overlay.4cpu.4096mem.qcow2.lzma'
MOPED_MEM = WEB_SERVER_URL + '/cloudlet/overlay/ubuntu11.10-server/moped/ubuntu-11.overlay.4cpu.4096mem.mem.lzma'
GRAPHICS_DISK = WEB_SERVER_URL + '/cloudlet/overlay/ubuntu11.10-server/graphics/ubuntu-11.overlay.4cpu.4096mem.qcow2.lzma'
GRAPHICS_MEM = WEB_SERVER_URL + '/cloudlet/overlay/ubuntu11.10-server/graphics/ubuntu-11.overlay.4cpu.4096mem.mem.lzma'
NULL_DISK = WEB_SERVER_URL + '/cloudlet/overlay/ubuntu11.10-server/null/ubuntu-11.overlay.4cpu.4096mem.mem.lzma'
NULL_MEM = WEB_SERVER_URL + '/cloudlet/overlay/ubuntu11.10-server/null/ubuntu-11.overlay.4cpu.4096mem.mem.lzma'
FACE_DISK = WEB_SERVER_URL + '/cloudlet/overlay/window7/face/window7-enterprise-i386.overlay.4cpu.4096mem.qcow2.lzma'
FACE_MEM = WEB_SERVER_URL + '/cloudlet/overlay/window7/face/window7-enterprise-i386.overlay.4cpu.4096mem.mem.lzma'
SPEECH_DISK = WEB_SERVER_URL + '/cloudlet/overlay/window7/speech/window7-enterprise-i386.overlay.4cpu.4096mem.qcow2.lzma'
SPEECH_MEM = WEB_SERVER_URL + '/cloudlet/overlay/window7/speech/window7-enterprise-i386.overlay.4cpu.4096mem.mem.lzma'
MAR_DISK = WEB_SERVER_URL + '/cloudlet/overlay/window7/mar/window7-enterprise-i386.overlay.4cpu.4096mem.qcow2.lzma'
MAR_MEM = WEB_SERVER_URL + '/cloudlet/overlay/window7/mar/window7-enterprise-i386.overlay.4cpu.4096mem.mem.lzma'
# BASE VM PATH
UBUNTU_BASE_DISK = '/home/krha/cloudlet/image/ubuntu-11.10-x86_64-server/ubuntu-11.base.img'
UBUNTU_BASE_MEM = '/home/krha/cloudlet/image/ubuntu-11.10-x86_64-server/ubuntu-11.base.mem'
WINDOW_BASE_DISK = '/home/krha/cloudlet/image/window7-enterprise-x86/window7-enterprise-i386.base.img'
WINDOW_BASE_MEM = '/home/krha/cloudlet/image/window7-enterprise-x86/window7-enterprise-i386.base.mem'

application_names = ("moped", "graphics", "face", "speech", "mar", "null")
VM_INFO = {\
        'moped':(MOPED_DISK, MOPED_MEM, UBUNTU_BASE_DISK, UBUNTU_BASE_MEM, 'linux'), \
        'graphics':(GRAPHICS_DISK, GRAPHICS_MEM, UBUNTU_BASE_DISK, UBUNTU_BASE_MEM, 'linux'), \
        'null':(NULL_DISK, NULL_MEM, UBUNTU_BASE_DISK, UBUNTU_BASE_MEM, 'linux'), \
        'face':(FACE_DISK, FACE_MEM, WINDOW_BASE_DISK, WINDOW_BASE_MEM, 'window'), \
        'speech':(SPEECH_DISK, SPEECH_MEM, WINDOW_BASE_DISK, WINDOW_BASE_MEM, 'window'), \
        'mar':(MAR_DISK, MAR_MEM, WINDOW_BASE_DISK, WINDOW_BASE_MEM, 'window') \
        }


WEB_SERVER_PORT_NUMBER = 9091
VM_TELNET_COMMAND_PORT_NUMBER = 19999
vm_name = ''


# Web Server configuration
app = Flask(__name__)
app.config.from_object(__name__)

# Web Server for receiving command
@app.route('/cloudlet', methods=['POST'])
def cloudlet():
    global vm_name

    print "Receive cloudlet info (run-type, application name) from client"
    json_data = request.form["info"]
    metadata = json.loads(json_data)

    run_type = metadata['run-type'].lower()
    vm_name = metadata['application'].lower()
    print "received info %s, %s" % (run_type, vm_name)
    
    if not vm_name in application_names:
        return "FAILED"

    ## execute
    piping_synthesis(vm_name)
    return "SUCCESS"

def piping_synthesis(vm_name):
    global VM_INFO
    disk_url = VM_INFO[vm_name.lower()][0]
    mem_url = VM_INFO[vm_name.lower()][1]
    base_disk = VM_INFO[vm_name.lower()][2]
    base_mem = VM_INFO[vm_name.lower()][3]
    os_type = VM_INFO[vm_name.lower()][4]

    recover_file = []
    delta_processes = []
    tmp_dir = './'
    time_transfer = Queue()
    time_decomp = Queue()
    time_delta = Queue()

    print "[INFO] Chunk size : %d" % (CHUNK_SIZE)

    start_time = datetime.now()
    for (overlay_url, base_name) in ((disk_url, base_disk), (mem_url, base_mem)):
        download_queue = JoinableQueue()
        decomp_queue = JoinableQueue()
        (download_pipe_in, download_pipe_out) = Pipe()
        (decomp_pipe_in, decomp_pipe_out) = Pipe()
        out_filename = os.path.join(tmp_dir, overlay_url.split("/")[-1] + ".recover")
        recover_file.append(out_filename)
        
        url = urllib2.urlopen(overlay_url)
        download_process = Process(target=network_worker, args=(url, download_queue, time_transfer, CHUNK_SIZE))
        decomp_process = Process(target=decomp_worker, args=(download_queue, decomp_queue, time_decomp))
        delta_process = Process(target=delta_worker, args=(decomp_queue, time_delta, base_name, out_filename))
        delta_processes.append(delta_process)
        
        download_process.start()
        decomp_process.start()
        delta_process.start()

    for delta_p in delta_processes:
        delta_p.join()

    telnet_port = 9999
    vnc_port = 2
    exe_time = run_snapshot(recover_file[0], recover_file[1], telnet_port, vnc_port, wait_vnc_end=False, terminal_mode=True, os_type=os_type)

    # Print out Time Measurement
    disk_transfer_time = time_transfer.get()
    mem_transfer_time = time_transfer.get()
    disk_decomp_time = time_decomp.get()
    mem_decomp_time = time_decomp.get()
    disk_delta_time = time_delta.get()
    mem_delta_time = time_delta.get()
    disk_transfer_start_time = disk_transfer_time['start_time']
    disk_transfer_end_time = disk_transfer_time['end_time']
    #disk_decomp_end_time = disk_decomp_time['end_time']
    #disk_delta_end_time = disk_delta_time['end_time']
    mem_transfer_start_time = mem_transfer_time['start_time']
    mem_transfer_end_time = mem_transfer_time['end_time']
    mem_decomp_end_time = mem_decomp_time['end_time']
    mem_delta_end_time = mem_delta_time['end_time']

    transfer_diff = mem_transfer_end_time-disk_transfer_start_time
    decomp_diff = mem_decomp_end_time-mem_transfer_end_time
    delta_diff = mem_delta_end_time-mem_decomp_end_time
    total_diff = datetime.now()-start_time
    message = '\n'
    message += 'Transfer\tDecomp\tDelta\tBoot\tResume\tTotal\n'
    message += "%04d.%06d\t" % (transfer_diff.seconds, transfer_diff.microseconds)
    message += "%04d.%06d\t" % (decomp_diff.seconds, decomp_diff.microseconds)
    message += "%04d.%06d\t" % (delta_diff.seconds, delta_diff.microseconds)
    message += "N/A\t"
    message += str(exe_time).split(":")[-1]
    message += "\n"
    print message


def print_usage(program_name):
    print 'usage\t: %s [run|clean] ' % program_name
    print 'example\t: ./cloudlet_cloud.py run '


def main(argv):
    global server_address

    if len(argv) < 2:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    operation = argv[1].lower()
    if not operation in ("clean", "run"):
        print "No supporing operation : ", operation
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)


    # operation handling
    if operation == "clean":
        stop_vm(VM_TELNET_COMMAND_PORT_NUMBER)
    elif operation == "run":

        #download_overlay('moped', VM_TELNET_COMMAND_PORT_NUMBER) 
        app.run(host='0.0.0.0', port=WEB_SERVER_PORT_NUMBER, processes=10)


if __name__ == "__main__":
    main(sys.argv)
