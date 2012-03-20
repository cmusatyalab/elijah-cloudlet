#!/usr/bin/env python
#
# Elijah: Cloudlet Infrastructure for Mobile Computing
# Copyright (C) 2011-2012 Carnegie Mellon University
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of version 2 of the GNU General Public License as published
# by the Free Software Foundation.  A copy of the GNU General Public License
# should have been distributed along with this program in the file
# LICENSE.GPL.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
import os
import sys
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
from cloudlet import run_snapshot
import struct



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
        # self.request is the YCP socket connected to the clinet
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
        recover_file = []
        delta_processes = []
        time_transfer = Queue()
        time_decomp = Queue()
        time_delta = Queue()

        start_time = datetime.now()
        print "[INFO] Chunk size : %d" % (CHUNK_SIZE)
        for overlay_name, file_size, base in (('disk', disk_size, base_disk_path), ('memory', mem_size, base_mem_path)):
            download_queue = JoinableQueue()
            decomp_queue = JoinableQueue()
            (download_pipe_in, download_pipe_out) = Pipe()
            (decomp_pipe_in, decomp_pipe_out) = Pipe()
            out_filename = os.path.join(tmp_dir, overlay_name + ".recover")
            recover_file.append(out_filename)
            
            download_process = Process(target=network_worker, args=(self.rfile, download_queue, time_transfer, CHUNK_SIZE, file_size))
            decomp_process = Process(target=decomp_worker, args=(download_queue, decomp_queue, time_decomp))
            delta_process = Process(target=delta_worker, args=(decomp_queue, time_delta, base, out_filename))
            download_process.start()
            decomp_process.start()
            delta_process.start()
            delta_processes.append(delta_process)

            #print "Waiting for download disk first"
            download_process.join()
            
        for delta_p in delta_processes:
            delta_p.join()

        telnet_port = 9999
        vnc_port = 2
        exe_time = run_snapshot(recover_file[0], recover_file[1], telnet_port, vnc_port, wait_vnc_end=False)

        # Print out Time Measurement
        disk_transfer_time = time_transfer.get()
        mem_transfer_time = time_transfer.get()
        disk_decomp_time = time_decomp.get()
        mem_decomp_time = time_decomp.get()
        disk_delta_time = time_delta.get()
        mem_delta_time = time_delta.get()
        disk_transfer_start_time = disk_transfer_time['start_time']
        #disk_transfer_end_time = disk_transfer_time['end_time']
        #disk_decomp_end_time = disk_decomp_time['end_time']
        #disk_delta_end_time = disk_delta_time['end_time']
        #mem_transfer_start_time = mem_transfer_time['start_time']
        mem_transfer_end_time = mem_transfer_time['end_time']
        mem_decomp_end_time = mem_decomp_time['end_time']
        mem_delta_end_time = mem_delta_time['end_time']

        print '\n'
        print "[Time] Transfer Time      : " + str(mem_transfer_end_time-disk_transfer_start_time).split(":")[-1]
        print "[Time] Decomp (Overlapped): " + str((mem_decomp_end_time-mem_transfer_end_time)).split(":")[-1]
        print "[Time] Delta (Overlapped) : " + str((mem_delta_end_time-mem_decomp_end_time)).split(":")[-1]
        print "[Time] VM Resume          : " + str(exe_time).split(":")[-1]
        print "[Time] Total Time         : " + str(datetime.now()-start_time)
        self.ret_success()


def get_local_ipaddress():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("gmail.com",80))
    ipaddress = (s.getsockname()[0])
    s.close()
    return ipaddress


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog [option]", version="MOPED Desktop Client")
    parser.add_option(
            '-i', '--input', action='store', type='string', dest='input_dir',
            help='Set Input image directory')
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server_address', default="localhost",
            help='Set Input image directory')
    parser.add_option(
            '-p', '--port', action='store', type='int', dest='server_port', default=8888,
            help='Set Input image directory')
    settings, args = parser.parse_args(argv)
    if len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    if not os.path.exists(settings.input_dir):
        parser.error("input directory does no exists at :%s" % (settings.input_dir))
    
    return settings, args


def load_input_images(input_dir):
    
    pass

def send_request(address, port, inputs):
    # connection
    try:
        socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        socket.setblocking(True)
        socket.connect(address, port)
    except socket.error, msg:
        sys.stderr.write("Error, %s\n", msg[1])
        sys.exit(1)

    # send requests
    for each_input in inputs:
        binary = open(each_input, 'r').read();
        length = os.path.getsize(each_input)
        if len(binary) != length:
            sys.stderr.write("Error, input length is wrong");
            sys.exit(1)

        #send
        socket.send(length)
        socket.sendall(binary)
        
        #recv


def main(argv=None):
    global LOCAL_IPADDRESS
    settings, args = process_command_line(sys.argv[1:])

    files = load_input_images(settings.input_dir)
    send_request(settings.server_address, settings.server_port, files)

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
