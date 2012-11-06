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
import time
import SocketServer
import socket
import subprocess
import tool
import bson

from optparse import OptionParser
from multiprocessing import Process, JoinableQueue, Queue
from tempfile import NamedTemporaryFile
import json
import tempfile
import struct
import libvirt_cloudlet as cloudlet
from lzma import LZMADecompressor
import Memory
import shutil
import delta


application = ['moped', 'face']
BaseVM_list = []

class Const(object):
    # Delta type
    DELTA_MEMORY    = 1
    DELTA_DISK      = 2

    # PIPLINING
    TRANSFER_SIZE = 1024*16
    END_OF_FILE = "!!Overlay Transfer End Marker"

    # Web server for Andorid Client
    LOCAL_IPADDRESS = 'localhost'
    SERVER_PORT_NUMBER = 8021


class RapidSynthesisError(Exception):
    pass


def recv_all(request, size):
    data = ''
    while len(data) < size:
        data += request.recv(size - len(data))
    return data

def network_worker(data, queue, time_queue, chunk_size, data_size=sys.maxint):
    start_time= time.time()
    total_read_size = 0
    counter = 0
    while total_read_size < data_size:
        read_size = min(data_size-total_read_size, chunk_size)
        counter = counter + 1
        chunk = data.read(read_size)
        total_read_size = total_read_size + len(chunk)
        if chunk:
            queue.put(chunk)
        else:
            break

    queue.put(Const.END_OF_FILE)
    end_time = time.time()
    time_delta= end_time-start_time
    time_queue.put({'start_time':start_time, 'end_time':end_time})
    try:
        print "[Transfer] : (%s)~(%s)=(%s) (%d loop, %d bytes, %lf Mbps)" % \
                (start_time, end_time, (time_delta),\
                counter, total_read_size, \
                total_read_size*8.0/time_delta/1024/1024)
    except ZeroDivisionError:
        print "[Transfer] : (%s)~(%s)=(%s) (%d, %d)" % \
                (start_time, end_time, (time_delta),\
                counter, total_read_size)


def decomp_worker(in_queue, pipe_filepath, time_queue):
    start_time = time.time()
    data_size = 0
    counter = 0
    decompressor = LZMADecompressor()
    pipe = open(pipe_filepath, "w")

    while True:
        chunk = in_queue.get()
        if chunk == Const.END_OF_FILE:
            break
        data_size = data_size + len(chunk)
        decomp_chunk = decompressor.decompress(chunk)

        in_queue.task_done()
        pipe.write(decomp_chunk)
        counter = counter + 1

    decomp_chunk = decompressor.flush()
    pipe.write(decomp_chunk)
    pipe.close()

    end_time = time.time()
    time_queue.put({'start_time':start_time, 'end_time':end_time})
    print "[Decomp] : (%s)-(%s)=(%s) (%d loop, %d bytes)" % \
            (start_time, end_time, (end_time-start_time), 
            counter, data_size)


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog" + " [option]",
            version="Rapid VM Synthesis(piping) 0.1")
    parser.add_option(
            '-c', '--config', action='store', type='string', dest='config_filename',
            help='Set configuration file, which has base VM information, to work as a server mode.')
    parser.add_option(
            '-s', '--sequential', action='store_true', dest='sequential',
            help='test no-piplining for speedup measurement')
    parser.add_option(
            '-a', '--app', action='store', type='string', dest='application',
            help="Set testing application name")
    settings, args = parser.parse_args(argv)
    if (settings.config_filename == None) and (not settings.sequential):
        parser.error('program need configuration file for running mode')
    if settings.sequential and (not settings.application):
        parser.error("Need application among [%s]" % ('|'.join(application)))

    return settings, args


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
        base_path = os.path.abspath(vm_info['path'])
        (base_diskmeta, base_mempath, base_memmeta) = \
                cloudlet.Const.get_basepath(base_path)
        vm_info['path'] = os.path.abspath(vm_info['path'])
        if not os.path.exists(base_path):
            print "Error, disk image (%s) is not exist" % (vm_info['path'])
            sys.exit(2)
        if not os.path.exists(base_mempath):
            print "Error, memory snapshot (%s) is not exist" % (base_mempath)
            sys.exit(2)

        if vm_info['type'].lower() == 'basevm':
            BaseVM_list.append(vm_info)
            print "%s - (Base Disk %d MB, Base Mem %d MB)" % \
                    (vm_info['name'], os.path.getsize(vm_info['path'])/1024/1024, \
                    os.path.getsize(base_mempath)/1024/1024)
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
        json_ret = json.dumps({"command":0x22, "return":"SUCCESS", "LaunchVM-IP":Const.LOCAL_IPADDRESS})
        print "SUCCESS to launch VM"
        json_size = struct.pack("!I", len(json_ret))
        self.request.send(json_size)
        self.wfile.write(json_ret)

    def _check_validity(self, request):
        # self.request is the TCP socket connected to the clinet
        data = request.recv(4)
        bson_size = struct.unpack("!I", data)[0]

        # recv JSON header
        bson_data = request.recv(bson_size)
        while len(bson_data) < bson_size:
            bson_data += request.recv(bson_size - len(bson_data))
        open("./bson_transfer", "wrb").write(bson_data)
        bson_header = bson.loads(bson_data)

        try:
            base_hashvalue = bson_header.get(cloudlet.Const.META_BASE_VM_SHA256, None)
            disk_size = bson_header[cloudlet.Const.META_OVERLAY_DISK_SIZE]
            mem_size = bson_header[cloudlet.Const.META_OVERLAY_MEMORY_SIZE]
        except KeyError:
            message = 'No key is in JSON'
            print message
            self.ret_fail(message)
            return
        # check base VM
        for base_vm in BaseVM_list:
            if base_hashvalue == base_vm.get('sha256', None):
                base_path = base_vm['path']
                print "[INFO] New client request %s VM (will transfer %d MB, %d MB)" \
                        % (base_path, disk_size/1024/1024, mem_size/1024/1024)
                return [base_path, bson_header, disk_size, mem_size]

        message = "Cannot find matching Base VM\nsha256: %s" % (base_hashvalue)
        print message
        self.ret_fail(message)
        return None

    def handle(self):
        # check_base VM
        start_time = time.time()
        header_start_time = time.time()
        ret_info = self._check_validity(self.request)
        header_end_time = time.time()
        if ret_info == None:
            message = "Failed, No such base VM exist : %s" % (ret_info[0])
            print message
            self.wfile.write(message)            
            self.ret_fail()
            return
        base_path, meta_info, disk_size, mem_size = ret_info
        (base_diskmeta, base_mem, base_memmeta) = \
                cloudlet.Const.get_basepath(base_path, check_exist=True)

        # read overlay files
        # create named pipe to convert queue to stream
        time_transfer_mem = Queue(); time_decomp_mem = Queue();
        time_delta_mem = Queue(); time_fuse_mem = Queue();
        tmp_dir = tempfile.mkdtemp()
        memory_pipe = os.path.join(tmp_dir, 'memory_pipe')
        os.mkfifo(memory_pipe)

        # Memory overlay
        mem_download_queue = JoinableQueue()
        mem_download_process = Process(target=network_worker, 
                args=(
                    self.rfile, mem_download_queue, time_transfer_mem, Const.TRANSFER_SIZE, mem_size
                    )
                )
        mem_decomp_process = Process(target=decomp_worker,
                args=(
                    mem_download_queue, memory_pipe, time_decomp_mem
                    )
                )

        # Disk overlay
        time_transfer_disk = Queue(); time_decomp_disk = Queue(); 
        time_delta_disk = Queue(); time_fuse_disk = Queue();
        disk_download_queue = JoinableQueue()

        disk_pipe = os.path.join(tmp_dir, 'disk_pipe')
        os.mkfifo(disk_pipe)
        disk_download_process = Process(target=network_worker, \
                args=(
                    self.rfile, disk_download_queue, time_transfer_disk, Const.TRANSFER_SIZE, disk_size
                    )
                )
        disk_decomp_process = Process(target=decomp_worker, \
                args=(
                    disk_download_queue, disk_pipe, time_decomp_disk
                    )
                )
        modified_img, modified_mem, fuse, delta_memory, memory_fuse, delta_disk, disk_fuse =\
                cloudlet.recover_launchVM(base_path, meta_info, disk_pipe, memory_pipe)
        delta_memory.time_queue = time_delta_mem
        delta_disk.time_queue = time_delta_disk
        memory_fuse.time_queue = time_fuse_mem
        disk_fuse.time_queue = time_fuse_disk

        # resume VM
        resumed_VM = cloudlet.ResumedVM(modified_img, modified_mem, fuse)
        resumed_VM.start()

        # early return to have application request
        # but need to wait until VM port opens
        self.ret_success()

        # start processes
        mem_download_process.start()
        mem_decomp_process.start()
        delta_memory.start()
        memory_fuse.start()
        memory_fuse.join()

        # Once memory is ready, start disk download
        # disk thread cannot start before finish memory
        disk_download_process.start()
        disk_decomp_process.start()
        delta_disk.start()
        disk_fuse.start()
        disk_fuse.join()

        end_time = time.time()
        total_time = (end_time-start_time)

        # printout result
        SynthesisTCPHandler.print_statistics(start_time, end_time, \
                time_transfer_mem, time_decomp_mem, time_delta_mem, time_fuse_mem,
                time_transfer_disk, time_decomp_disk, time_delta_disk, time_fuse_disk)

        # terminate
        while True:
            user_input = raw_input("type q to quit : ")
            if user_input == 'q':
                break

        close_start_time = time.time()
        delta_disk.join()
        delta_memory.finish()
        delta_disk.finish()
        resumed_VM.terminate()
        fuse.terminate()

        if os.path.exists(memory_pipe):
            os.unlink(memory_pipe)
        if os.path.exists(disk_pipe):
            os.unlink(disk_pipe)
        shutil.rmtree(tmp_dir)
        close_end_time = time.time()
        print "Time for finishing(close all fd) : %f" % (close_end_time-close_start_time)


    @staticmethod
    def print_statistics(start_time, end_time, \
            time_transfer_mem, time_decomp_mem, time_delta_mem, time_fuse_mem,\
            time_transfer_disk, time_decomp_disk, time_delta_disk, time_fuse_disk):
        # Print out Time Measurement
        disk_transfer_time = time_transfer_disk.get()
        disk_decomp_time = time_decomp_disk.get()
        disk_delta_time = time_delta_disk.get()
        disk_fuse_time = time_fuse_disk.get()
        mem_transfer_time = time_transfer_mem.get()
        mem_decomp_time = time_decomp_mem.get()
        mem_delta_time = time_delta_mem.get()
        mem_fuse_time = time_fuse_mem.get()
        disk_transfer_start_time = disk_transfer_time['start_time']
        disk_transfer_end_time = disk_transfer_time['end_time']
        disk_decomp_start_time = disk_decomp_time['start_time']
        disk_decomp_end_time = disk_decomp_time['end_time']
        disk_delta_start_time = disk_delta_time['start_time']
        disk_delta_end_time = disk_delta_time['end_time']
        disk_fuse_start_time = disk_fuse_time['start_time']
        disk_fuse_end_time = disk_fuse_time['end_time']

        mem_transfer_start_time = mem_transfer_time['start_time']
        mem_transfer_end_time = mem_transfer_time['end_time']
        mem_decomp_start_time = mem_decomp_time['start_time']
        mem_decomp_end_time = mem_decomp_time['end_time']
        mem_delta_start_time = mem_delta_time['start_time']
        mem_delta_end_time = mem_delta_time['end_time']
        mem_fuse_start_time = mem_fuse_time['start_time']
        mem_fuse_end_time = mem_fuse_time['end_time']

        transfer_diff = (disk_transfer_end_time-disk_transfer_start_time) + \
                (mem_transfer_end_time-mem_transfer_start_time)
        decomp_diff = (disk_decomp_end_time-disk_transfer_end_time) + \
                (mem_decomp_end_time-mem_transfer_end_time)
        delta_diff = (disk_fuse_end_time-disk_decomp_end_time) + \
                (mem_fuse_end_time-mem_decomp_end_time)

        message = "\n"
        message += "Pipelined measurement\n"
        message += 'Transfer\tDecomp\t\tDelta(Fuse)\t\tTotal\n'
        message += "%011.06f\t" % (transfer_diff)
        message += "%011.06f\t" % (decomp_diff)
        message += "%011.06f\t" % (delta_diff)
        message += "%011.06f\t" % (end_time-start_time)
        message += "\n"
        print message


def get_local_ipaddress():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("gmail.com",80))
    ipaddress = (s.getsockname()[0])
    s.close()
    return ipaddress


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    # Open port for both Internet and private network
    if settings.sequential:
        start_time = time.time()
        base_disk, download_disk, download_mem = download_app(settings.application)
        meta = None

        overlay_disk = NamedTemporaryFile(prefix="cloudlet-synthesis-disk-")
        overlay_mem = NamedTemporaryFile(prefix="cloudlet-synthesis-mem-")
        outpath, decomp_time = tool.decomp_lzma(download_disk, overlay_disk.name)
        sys.stdout.write("[Debug] Overlay-disk decomp time: %s\n" % (decomp_time))
        outpath, decomp_time = tool.decomp_lzma(download_mem, overlay_mem.name)
        sys.stdout.write("[Debug] Overlay-mem decomp time: %s\n" % (decomp_time))

        # recover VM
        modified_img, modified_mem, fuse = cloudlet.recover_launchVM(base_disk, meta, 
                overlay_disk.name, overlay_mem.name, log=sys.stdout)

        # resume VM
        end_time = time.time()
        resumed_VM = cloudlet.ResumedVM(modified_img, modified_mem, fuse)
        resume_time = resumed_VM.resume()
        resumed_VM.terminate()
        total_time = (end_time-start_time) + resume_time

        os.unlink(download_disk)
        os.unlink(download_mem)

        print "Total Synthesis time : %011.06f" % (total_time)
    else:
        config_file, error_msg = parse_configfile(settings.config_filename)
        if error_msg:
            print error_msg
            sys.exit(2)

        Const.LOCAL_IPADDRESS = "0.0.0.0" # get_local_ipaddress()
        server_address = (Const.LOCAL_IPADDRESS, Const.SERVER_PORT_NUMBER)
        print "Open TCP Server (%s)\n" % (str(server_address))
        SocketServer.TCPServer.allow_reuse_address = True
        server = SocketServer.TCPServer(server_address, SynthesisTCPHandler)
        server.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        try:
            server.serve_forever()
        except KeyboardInterrupt:
            server.socket.close()
            sys.exit(0)

    return 0

def download_app(application):
    WEB_SERVER_URL = 'http://192.168.2.4'
    overlay_disk = NamedTemporaryFile(prefix="cloudlet-download-disk-", delete=False)
    overlay_mem = NamedTemporaryFile(prefix="cloudlet-download-mem-", delete=False)
    base_path = ''
    if application == 'moped':
        disk_url = WEB_SERVER_URL + '/overlay/ubuntu/moped/precise.overlay-img.lzma'
        memory_url = WEB_SERVER_URL + '/overlay/ubuntu/moped/precise.overlay-mem.lzma'
        base_path = "/home/krha/cloudlet/image/ubuntu-12.04.1-server-i386/precise.raw"
    elif application == 'face':
        disk_url = WEB_SERVER_URL + '/overlay/window/face/window7.overlay-img.lzma'
        memory_url = WEB_SERVER_URL + '/overlay/window/face/window7.overlay-mem.lzma'
        base_path = "/home/krha/cloudlet/image/window7-enterprise-x86/window7.raw"
    else:
        raise RapidSynthesisError("No such application : %s" % (application))

    #memory download
    cmd = "wget %s -O %s" % (memory_url, overlay_mem.name)
    print cmd
    memory_start_time = time.time()
    proc = subprocess.Popen(cmd, shell=True)
    proc.wait()
    memory_end_time = time.time()

    #disk download
    cmd = "wget %s -O %s" % (disk_url, overlay_disk.name)
    print cmd
    disk_start_time = time.time()
    proc = subprocess.Popen(cmd, shell=True)
    proc.wait()
    disk_end_time = time.time()

    print "Memory download time : %010.04f" % (memory_end_time-memory_start_time)
    print "Disk download time : %010.04f" % (disk_end_time-disk_start_time)

    return base_path, overlay_disk.name, overlay_mem.name

if __name__ == "__main__":
    status = main()
    sys.exit(status)
