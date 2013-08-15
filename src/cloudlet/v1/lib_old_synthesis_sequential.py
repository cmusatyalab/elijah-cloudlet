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
import sys
from tempfile import NamedTemporaryFile
import SocketServer
import socket
import urllib2
from optparse import OptionParser
from datetime import datetime
from multiprocessing import Process, Queue, Pipe, JoinableQueue
import subprocess
import pylzma
import json
import tempfile
from cloudlet.synthesis import run_snapshot
import struct

# PIPLINING
CHUNK_SIZE = 1024*16
END_OF_FILE = "Overlay Transfer End Marker"
operation_mode = ('run', 'mock')
application_names = ("moped", "face", "speech", "mar", "null")

# Web server for Andorid Client
def get_local_ipaddress():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("gmail.com",80))
    ipaddress = (s.getsockname()[0])
    s.close()
    return ipaddress

#LOCAL_IPADDRESS = get_local_ipaddress()
LOCAL_IPADDRESS = "192.168.2.2"
SERVER_PORT_NUMBER = 8021
BaseVM_list = []

# Overlya URL
WEB_SERVER_URL = 'http://dagama.isr.cs.cmu.edu/cloudlet'
MOPED_DISK = WEB_SERVER_URL + '/overlay/moped/overlay1/moped.qcow2.lzma'
MOPED_MEM = WEB_SERVER_URL + '/overlay/moped/overlay1/moped.mem.lzma'
FACE_DISK = WEB_SERVER_URL + '/overlay/face/overlay1/face.qcow2.lzma'
FACE_MEM = WEB_SERVER_URL + '/overlay/face/overlay1/face.mem.lzma'
SPEECH_DISK = WEB_SERVER_URL + '/overlay/speech/overlay1/speech.qcow2.lzma'
SPEECH_MEM = WEB_SERVER_URL + '/overlay/speech/overlay1/speech.mem.lzma'
NULL_DISK = WEB_SERVER_URL + '/overlay/null/overlay1/null.qcow2.lzma'
NULL_MEM = WEB_SERVER_URL + '/overlay/null/overlay1/null.mem.lzma'
# BASE VM PATH
MOPED_BASE_DISK = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.base.img'
MOPED_BASE_MEM = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.base.mem'
NULL_BASE_DISK = MOPED_BASE_DISK
NULL_BASE_MEM = MOPED_BASE_MEM
FACE_BASE_DISK = '/home/krha/cloudlet/image/WindowXP_Base/winxp-with-jre7_base.qcow2'
FACE_BASE_MEM = '/home/krha/cloudlet/image/WindowXP_Base/winxp-with-jre7_base.mem'
SPEECH_BASE_DISK = FACE_BASE_DISK
SPEECH_BASE_MEM = FACE_BASE_MEM

def get_download_url(machine_name):
    url_disk = ''
    url_mem = ''
    base_disk = ''
    base_mem = ''
    os_type = ''
    if machine_name.lower() == "moped":
        url_disk = MOPED_DISK
        url_mem = MOPED_MEM
        base_disk = MOPED_BASE_DISK
        base_mem = MOPED_BASE_MEM
        base_mem = SPEECH_BASE_MEM
        os_type = 'linux'
    elif machine_name.lower() == "face":
        url_disk = FACE_DISK
        url_mem = FACE_MEM
        base_disk = FACE_BASE_DISK
        base_mem = FACE_BASE_MEM
        os_type = 'window'
    elif machine_name.lower() == "null":
        url_disk = NULL_DISK
        url_mem = NULL_MEM
        base_disk = NULL_BASE_DISK
        base_mem = NULL_BASE_MEM
        base_mem = SPEECH_BASE_MEM
        os_type = 'linux'
    elif machine_name.lower() == "speech":
        url_disk = SPEECH_DISK
        url_mem = SPEECH_MEM
        base_disk = SPEECH_BASE_DISK
        base_mem = SPEECH_BASE_MEM
        os_type = 'window'
    elif machine_name.lower() == "graphics":
        url_disk = SPEECH_DISK
        url_mem = SPEECH_MEM
        base_disk = SPEECH_BASE_DISK
        base_mem = SPEECH_BASE_MEM
        os_type = 'linux'

    return url_disk, url_mem, base_disk, base_mem, os_type


def network_worker(data, out_path, time_queue, chunk_size, data_size=sys.maxint):
    start_time= datetime.now()
    total_read_size = 0
    counter = 0
    output_file = open(out_path, "w+b")
    while total_read_size < data_size:
        read_size = min(data_size-total_read_size, chunk_size)
        counter = counter + 1
        chunk = data.read(read_size)
        total_read_size = total_read_size + len(chunk)
        if chunk:
            output_file.write(chunk)
        else:
            break

    output_file.close()
    end_time = datetime.now()
    time_delta= end_time-start_time
    time_queue.put({'start_time':start_time, 'end_time':end_time})
    try:
        print "[Transfer] : (%s)-(%s)=(%s) (%d loop, %d bytes, %lf Mbps)" % (start_time.strftime('%X'), end_time.strftime('%X'), str(end_time-start_time), counter, total_read_size, total_read_size*8.0/time_delta.seconds/1024/1024)
    except ZeroDivisionError:
        print "[Transfer] : (%s)-(%s)=(%s) (%d, %d)" % (start_time.strftime('%X'), end_time.strftime('%X'), str(end_time-start_time), counter, total_read_size)


def decomp_worker(in_path, out_path, time_queue):
    in_file = open(in_path, "rb")
    out_file = open(out_path, "w+b")
    start_time = datetime.now()
    data_size = 0
    counter = 0
    obj = pylzma.decompressobj()
    while True:
        chunk = in_file.read(CHUNK_SIZE)
        if not chunk:
            break
        data_size = data_size + len(chunk)
        decomp_chunk = obj.decompress(chunk)
        #print "in decomp : %d %d" % (data_size, len(decomp_chunk))

        out_file.write(decomp_chunk)
        counter = counter + 1

    in_file.close()
    out_file.close()
    end_time = datetime.now()
    time_queue.put({'start_time':start_time, 'end_time':end_time})
    print "[Decomp] : (%s)-(%s)=(%s) (%d loop, %d bytes)" % (start_time.strftime('%X'), end_time.strftime('%X'), str(end_time-start_time), counter, data_size)


def delta_worker(in_path, time_queue, base_filename, out_filename):
    in_file = open(in_path, "rb")
    start_time = datetime.now()
    data_size = 0
    counter = 0

    # run xdelta 3 with named pipe
    command_str = "xdelta3 -df -s %s %s %s" % (base_filename, in_path, out_filename)
    xdelta_process = subprocess.Popen(command_str, shell=True)
    xdelta_process.wait()

    in_file.close()
    ret = xdelta_process.wait()
    end_time = datetime.now()
    time_queue.put({'start_time':start_time, 'end_time':end_time})

    if ret == 0:
        print "[Delta] : (%s)-(%s)=(%s) (%d loop, %d bytes)" % (start_time.strftime('%X'), end_time.strftime('%X'), str(end_time-start_time), counter, data_size)
        return True
    else:
        print "Error, xdelta process has not successed"
        return False


def delta_worker_pipe(in_queue, time_queue, base_filename, kvm_pipe):
    start_time = datetime.now()
    data_size = 0
    counter = 0

    # create named pipe for xdelta3
    out_pipename = (base_filename + ".fifo")
    if os.path.exists(out_pipename):
        os.unlink(out_pipename)
    os.mkfifo(out_pipename)

    # run xdelta 3 with named pipe
    command_str = "xdelta3 -df -s %s %s %s" % (base_filename, out_pipename, kvm_pipe)
    xdelta_process = subprocess.Popen(command_str, shell=True)
    out_pipe = open(out_pipename, "w")

    # TODO: If chunk size is too big, XDELTA checksum error occur
    # TODO: It is probably related to the maximum queue buffer size
    while True:
        chunk = in_queue.get()
        if chunk == END_OF_FILE:
            break;

        data_size = data_size + len(chunk)
        #print "in delta : %d, %d, %d %s" %(counter, len(chunk), data_size, out_filename)

        out_pipe.write(chunk)
        in_queue.task_done()
        counter = counter + 1

    out_pipe.close()
    ret = xdelta_process.wait()
    os.unlink(out_pipename)
    end_time = datetime.now()
    time_queue.put({'start_time':start_time, 'end_time':end_time})

    if ret == 0:
        print "[Delta] : (%s)-(%s)=(%s) (%d loop, %d bytes)" % (start_time.strftime('%X'), end_time.strftime('%X'), str(end_time-start_time), counter, data_size)
        return True
    else:
        print "Error, xdelta process has not successed"
        return False

def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog" + " [%s] [option]" % ('|'.join(mode for mode in operation_mode)),
            version="Cloudlet Synthesys(piping) 0.1")
    parser.add_option(
            '-c', '--config', action='store', type='string', dest='config_filename',
            help='[run mode] Set configuration file, which has base VM information, to work as a server mode.')
    parser.add_option(
            '-n', '--name', type='choice', choices=application_names, action='store', dest='vmname',
            help="[test mode] Set VM name among %s" % (str(application_names)))
    settings, args = parser.parse_args(argv)
    if len(args) == 0 or args[0] not in operation_mode:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    mode = args[0]
    if mode == operation_mode[0] and settings.config_filename == None:
        parser.error('program need configuration file for running mode')
    if mode == operation_mode[1] and settings.vmname == None:
        parser.error('program need vmname for mock mode')

    return mode, settings, args


def parse_configfile(filename):
    global BaseVM_list
    if not os.path.exists(filename):
        return None, "configuration file is not exist : " + filename

    try:
        json_data = json.load(open(filename, 'r'), "UTF-8")
    except ValueError:
        return None, "Invlid JSON format : " + open(filename, 'r').read()
    if not json_data.has_key('VM'):
        return None, "JSON Does not have 'VM' Key"


    VM_list = json_data['VM']
    print "-------------------------------"
    print "* VM Configuration Infomation"
    for vm_info in VM_list:
        # check file location
        vm_info['diskimg_path'] = os.path.abspath(vm_info['diskimg_path'])
        vm_info['memorysnapshot_path'] = os.path.abspath(vm_info['memorysnapshot_path'])
        if not os.path.exists(vm_info['diskimg_path']):
            print "Error, disk image (%s) is not exist" % (vm_info['diskimg_path'])
            sys.exit(2)
        if not os.path.exists(vm_info['memorysnapshot_path']):
            print "Error, memory snapshot (%s) is not exist" % (vm_info['memorysnapshot_path'])
            sys.exit(2)

        if vm_info['type'].lower() == 'basevm':
            BaseVM_list.append(vm_info)
            print "%s - (Base Disk %d MB, Base Mem %d MB)" % (vm_info['name'], os.path.getsize(vm_info['diskimg_path'])/1024/1024, os.path.getsize(vm_info['memorysnapshot_path'])/1024/1024)
    print "-------------------------------"

    return json_data, None


class SynthesisTCPHandler(SocketServer.StreamRequestHandler):

    def finish(self):
        pass

    def ret_fail(self, message):
        print "Error, %s" % str(message)
        json_ret = json.dumps({"Error":message})
        json_size = struct.pack("!I", len(json_ret))
        self.request.send(json_size)
        self.wfile.write(json_ret)

    def ret_success(self):
        global LOCAL_IPADDRESS
        json_ret = json.dumps({"command":0x22, "return":"SUCCESS", "LaunchVM-IP":LOCAL_IPADDRESS})
        print "SUCCESS to launch VM"
        json_size = struct.pack("!I", len(json_ret))
        self.request.send(json_size)
        self.wfile.write(json_ret)

    def handle(self):
        # self.request is the TCP socket connected to the clinet
        data = self.request.recv(4)
        json_size = struct.unpack("!I", data)[0]

        # recv JSON header
        json_str = self.request.recv(json_size)
        json_data = json.loads(json_str)
        if 'VM' not in json_data or len(json_data['VM']) == 0:
            self.ret_fail("No VM Key at JSON")
            return

        vm_name = ''
        try:
            vm_name = json_data['VM'][0]['base_name']
            disk_size = int(json_data['VM'][0]['diskimg_size'])
            mem_size = int(json_data['VM'][0]['memory_snapshot_size'])
            #print "received info %s" % (vm_name)
        except KeyError:
            message = 'No key is in JSON'
            print message
            self.ret_fail(message)
            return

        print "[INFO] New client request %s VM (will transfer %d MB, %d MB)" % (vm_name, disk_size/1024/1024, mem_size/1024/1024)

        # check base VM
        base_disk_path = None
        base_mem_path = None
        for base_vm in BaseVM_list:
            if vm_name.lower() == base_vm['name'].lower():
                base_disk_path = base_vm['diskimg_path']
                base_mem_path = base_vm['memorysnapshot_path']
        if base_disk_path == None or base_mem_path == None:
            message = "Failed, No such base VM exist : %s" % (vm_name)
            self.wfile.write(message)            
            print message

        # read overlay files
        tmp_dir = tempfile.mkdtemp()
        time_transfer = Queue()
        time_decomp = Queue()
        time_delta = Queue()

        # check OS type
        # TODO: FIX this
        os_type = ''
        if base_disk_path.find('ubuntu') != -1:
            os_type = 'linux'
        else:
            os_type = 'window'

        start_time = datetime.now()
        # handling disk overlay
        disk_download_file = NamedTemporaryFile(prefix="download-").name
        disk_decomp_file = NamedTemporaryFile(prefix="decomp-").name
        (disk_download_pipe_in, disk_download_pipe_out) = Pipe()
        (disk_decomp_pipe_in, disk_decomp_pipe_out) = Pipe()
        disk_out_filename = os.path.join(tmp_dir, "disk.recover")
        disk_download_process = Process(target=network_worker, args=(self.rfile, disk_download_file, time_transfer, CHUNK_SIZE, disk_size))
        disk_decomp_process = Process(target=decomp_worker, args=(disk_download_file, disk_decomp_file, time_decomp))
        disk_delta_process = Process(target=delta_worker, args=(disk_decomp_file, time_delta, base_disk_path, disk_out_filename))

        # handling memory overlay
        mem_download_file = NamedTemporaryFile(prefix="download-").name
        mem_decomp_file = NamedTemporaryFile(prefix="decomp-").name
        (mem_download_pipe_in, mem_download_pipe_out) = Pipe()
        (mem_decomp_pipe_in, mem_decomp_pipe_out) = Pipe()
        mem_download_process = Process(target=network_worker, args=(self.rfile, mem_download_file, time_transfer, CHUNK_SIZE, mem_size))
        mem_decomp_process = Process(target=decomp_worker, args=(mem_download_file, mem_decomp_file, time_decomp))
        # memory snapshot result will be pipelined to KVM
        mem_out_filename = os.path.join(tmp_dir, "mem.recover")
        mem_delta_process = Process(target=delta_worker, args=(mem_decomp_file, time_delta, base_mem_path, mem_out_filename))
        
        # start processes
        # wait for download disk first
        disk_download_process.start()
        disk_download_process.join()
        disk_decomp_process.start()
        disk_decomp_process.join()
        disk_delta_process.start()
        disk_delta_process.join()

        # Once disk is ready, start KVM
        # Memory snapshot will be completed by pipelining
        mem_download_process.start()
        mem_download_process.join()
        mem_decomp_process.start()
        mem_decomp_process.join()
        mem_delta_process.start()
        mem_delta_process.join()
        telnet_port = 9999
        vnc_port = 2
        exe_time = run_snapshot(disk_out_filename, mem_out_filename, \
                telnet_port, vnc_port, wait_vnc_end=False, \
                terminal_mode=True, os_type=os_type)
        kvm_end_time = datetime.now()


        # Print out Time Measurement
        disk_transfer_time = time_transfer.get()
        mem_transfer_time = time_transfer.get()
        disk_decomp_time = time_decomp.get()
        mem_decomp_time = time_decomp.get()
        disk_delta_time = time_delta.get()
        mem_delta_time = time_delta.get()
        disk_transfer_start_time = disk_transfer_time['start_time']
        disk_transfer_end_time = disk_transfer_time['end_time']
        disk_decomp_start_time = disk_decomp_time['start_time']
        disk_decomp_end_time = disk_decomp_time['end_time']
        disk_delta_start_time = disk_delta_time['start_time']
        disk_delta_end_time = disk_delta_time['end_time']
        mem_transfer_start_time = mem_transfer_time['start_time']
        mem_transfer_end_time = mem_transfer_time['end_time']
        mem_decomp_start_time = mem_decomp_time['start_time']
        mem_decomp_end_time = mem_decomp_time['end_time']
        mem_delta_start_time = mem_delta_time['start_time']
        mem_delta_end_time = mem_delta_time['end_time']

        transfer_diff = (disk_transfer_end_time-disk_transfer_start_time) + (mem_transfer_end_time-mem_transfer_start_time)
        decomp_diff = (disk_decomp_end_time-disk_decomp_start_time) + (mem_decomp_end_time-mem_decomp_start_time)
        delta_diff = (disk_delta_end_time-disk_delta_start_time) + (mem_delta_end_time-mem_delta_start_time)
        kvm_diff = kvm_end_time-mem_delta_end_time
        total_diff = datetime.now()-start_time
        message = "\n"
        message += 'Transfer\tDecomp\tDelta\tBoot\tResume\tTotal\n'
        message += "%04d.%06d\t" % (transfer_diff.seconds, transfer_diff.microseconds)
        message += "%04d.%06d\t" % (decomp_diff.seconds, decomp_diff.microseconds)
        message += "%04d.%06d\t" % (delta_diff.seconds, delta_diff.microseconds)
        message += "%04d.%06d\t" % (kvm_diff.seconds, kvm_diff.microseconds)
        message += "%04d.%06d\t" % (total_diff.seconds, total_diff.microseconds)
        message += "\n"
        print message
        self.ret_success()



def main(argv=None):
    global LOCAL_IPADDRESS
    mode, settings, args = process_command_line(sys.argv[1:])

    if mode == operation_mode[0]: # run mode
        config_file, error_msg = parse_configfile(settings.config_filename)
        if error_msg:
            print error_msg
            sys.exit(2)

        server_address = ("0.0.0.0", SERVER_PORT_NUMBER)
        print "Open TCP Server (%s)\n" % (str(server_address))
        SocketServer.TCPServer.allow_reuse_address = True
        server = SocketServer.TCPServer(server_address, SynthesisTCPHandler)
        server.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        #atexit.register(server.socket.close)
        #atexit.register(server.shutdown)

        try:
            server.serve_forever()
        except KeyboardInterrupt:
            server.socket.close()
            sys.exit(0)

    elif mode == operation_mode[1]: # mock mode
        piping_synthesis(settings.vmname)
    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
