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
from optparse import OptionParser
from multiprocessing import Process, Queue, Pipe, JoinableQueue
from tempfile import NamedTemporaryFile
import json
import tempfile
import struct
import libvirt_cloudlet as cloudlet
from lzma import LZMADecompressor
import Memory
import Disk
import shutil
import threading



BaseVM_list = []

class Const(object):
    # No pipelining test
    NO_PIPELINING   = False
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


def delta_worker(time_queue, output_queue, options):
    start_time = time.time()
    delta_type = options["type"]
    base_image = options["base_path"]
    piping_file = options["input_file"]
    output_path = options["output_file"]

    (base_diskmeta, base_mem, base_memmeta) = \
            cloudlet.Const.get_basepath(base_image, check_exist=True)
    if delta_type == Const.DELTA_MEMORY:
        overlay_map = Memory.recover_memory(base_image, \
                base_mem, piping_file, \
                base_memmeta, output_path)
    elif delta_type == Const.DELTA_DISK:
        overlay_memory = options["overlay_memory"]
        overlay_map = Disk.recover_disk(base_image, base_mem, \
                overlay_memory, piping_file, output_path, \
                cloudlet.Const.CHUNK_SIZE)
    else:
        raise RapidSynthesisError("Invalid delta type : %d" % delta_type)

    output_queue.put(overlay_map)
    end_time = time.time()
    time_queue.put({'start_time':start_time, 'end_time':end_time})
    print "[Delta] : (%s)-(%s)=(%s)" % \
            (start_time, end_time, (end_time-start_time))


class RecoverThread(threading.Thread):
    def __init__(self, recover_type, base_image, piping_file, output_path, 
            overlay_memory=None):
        self.base_image = base_image
        self.piping_file = piping_file
        self.output_path = output_path
        self.stop = threading.Event()
        self._running = True
        if recover_type == Const.DELTA_MEMORY:
            threading.Thread.__init__(self, target=self.recover_memory)
        elif recover_type == Const.DELTA_DISK:
            self.overlay_memory = overlay_memory
            threading.Thread.__init__(self, target=self.recover_disk)

    def recover_memory(self):
        (base_diskmeta, base_mem, base_memmeta) = \
                cloudlet.Const.get_basepath(self.base_image, check_exist=True)
        self.overlay_map = Memory.recover_memory(self.base_image, \
                base_mem, self.piping_file, \
                base_memmeta, self.output_path)
        self._running = False
        print "[INFO] close memory recover thread"

    def recover_disk(self):
        (base_diskmeta, base_mem, base_memmeta) = \
                cloudlet.Const.get_basepath(self.base_image, check_exist=True)
        self.overlay_map = Disk.recover_disk(self.base_image, base_mem, \
                self.overlay_memory, self.piping_file, self.output_path, \
                cloudlet.Const.CHUNK_SIZE)
        self._running = False
        print "[INFO] close disk recover thread"

    def terminate(self):
        self.stop.set()


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog" + " [option]",
            version="Rapid VM Synthesis(piping) 0.1")
    parser.add_option(
            '-c', '--config', action='store', type='string', dest='config_filename',
            help='[run mode] Set configuration file, which has base VM information, to work as a server mode.')
    settings, args = parser.parse_args(argv)
    if settings.config_filename == None:
        parser.error('program need configuration file for running mode')

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
        json_size = struct.unpack("!I", data)[0]

        # recv JSON header
        json_str = request.recv(json_size)
        json_data = json.loads(json_str)
        if 'VM' not in json_data or len(json_data['VM']) == 0:
            self.ret_fail("No VM Key at JSON")
            return

        vm_name = ''
        try:
            vm_name = json_data['VM'][0]['base_name']
            disk_size = int(json_data['VM'][0]['diskimg_size'])
            mem_size = int(json_data['VM'][0]['memory_snapshot_size'])
        except KeyError:
            message = 'No key is in JSON'
            print message
            self.ret_fail(message)
            return
        print "[INFO] New client request %s VM (will transfer %d MB, %d MB)" \
                % (vm_name, disk_size/1024/1024, mem_size/1024/1024)

        # check base VM
        for base_vm in BaseVM_list:
            if vm_name.lower() == base_vm['name'].lower():
                base_path = base_vm['path']
                return [base_path, disk_size, mem_size]
                (base_diskmeta, base_mem, base_memmeta) = \
                        cloudlet.Const.get_basepath(base_path, check_exist=True)

        return None

    def handle(self):
        # check_base VM
        start_time = time.time()
        ret_info = self._check_validity(self.request)
        if ret_info == None:
            message = "Failed, No such base VM exist : %s" % (ret_info[0])
            print message
            self.wfile.write(message)            
            self.ret_fail()
            return
        base_path, disk_size, mem_size = ret_info
        modified_mem = NamedTemporaryFile(prefix="cloudlet-recoverd-mem-", 
                delete=False)
        modified_img = NamedTemporaryFile(prefix="cloudlet-recoverd-img-", 
                delete=False)

        # read overlay files
        # create named pipe to convert queue to stream
        time_transfer_mem = Queue(); time_decomp_mem = Queue(); time_delta_mem = Queue()
        tmp_dir = tempfile.mkdtemp()
        memory_pipe = os.path.join(tmp_dir, 'memory_pipe')
        os.mkfifo(memory_pipe)

        #import pdb; pdb.set_trace()
        # Memory overlay
        mem_download_queue = JoinableQueue()
        mem_output_queue = JoinableQueue()
        memory_overlay_map = list()
        (mem_download_pipe_in, mem_download_pipe_out) = Pipe()
        (mem_decomp_pipe_in, mem_decomp_pipe_out) = Pipe()
        mem_options = {"type":Const.DELTA_MEMORY, "base_path":base_path,
                "input_file":memory_pipe, "output_file":modified_mem.name}
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
        mem_delta_process = Process(target=delta_worker, \
                args=(
                    time_delta_mem, mem_output_queue, mem_options
                    )
                )

        # Disk overlay
        time_transfer_disk = Queue(); time_decomp_disk = Queue(); time_delta_disk = Queue()
        disk_download_queue = JoinableQueue()
        disk_output_queue = JoinableQueue()
        (disk_download_pipe_in, disk_download_pipe_out) = Pipe()
        (disk_decomp_pipe_in, disk_decomp_pipe_out) = Pipe()
        disk_overlay_map = list()

        disk_pipe = os.path.join(tmp_dir, 'disk_pipe')
        os.mkfifo(disk_pipe)
        disk_options = {"type":Const.DELTA_DISK, "base_path":base_path,
                "input_file":disk_pipe, "output_file":modified_img.name,
                "overlay_memory":modified_mem.name}

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
        disk_delta_process = Process(target=delta_worker, \
                args=(
                    time_delta_disk, disk_output_queue, disk_options
                    )
                )

        # start processes
        # Memory snapshot will be completed by pipelining
        if Const.NO_PIPELINING:
            mem_download_process.start()
            mem_download_process.join()
            mem_decomp_process.start()
            mem_decomp_process.join()
            mem_delta_process.start()
            memory_overlay_map = mem_output_queue.get()
            mem_output_queue.task_done()
            mem_delta_process.join()

            disk_download_process.start()
            disk_download_process.join()
            disk_decomp_process.start()
            disk_decomp_process.join()
            disk_delta_process.start()
            disk_overlay_map = disk_output_queue.get()
            disk_output_queue.task_done()
            disk_delta_process.join()
        else:
            mem_download_process.start()
            mem_decomp_process.start()
            mem_delta_process.start()
            memory_overlay_map = mem_output_queue.get()
            mem_output_queue.task_done()
            mem_delta_process.join()

            # Once memory is ready, start disk download
            # disk thread cannot start before finish memory
            disk_download_process.start()
            disk_decomp_process.start()
            disk_delta_process.start()
            disk_overlay_map = disk_output_queue.get()
            disk_output_queue.task_done()
            disk_delta_process.join()

        # make FUSE disk & memory
        (base_diskmeta, base_mem, base_memmeta) = \
                cloudlet.Const.get_basepath(base_path, check_exist=True)
        fuse = cloudlet.run_fuse(cloudlet.Const.VMNETFS_PATH, 
                cloudlet.Const.CHUNK_SIZE, 
                base_path, base_mem, resumed_disk=modified_img.name, 
                disk_overlay_map=disk_overlay_map,
                resumed_memory=modified_mem.name, 
                memory_overlay_map=memory_overlay_map)
        end_time = time.time()

        # resume VM
        resumed_VM = cloudlet.ResumedVM(modified_img.name, modified_mem.name, fuse)
        resume_time = resumed_VM.resume()
        resumed_VM.terminate()
        total_time = (end_time-start_time) + resume_time

        # printout result
        SynthesisTCPHandler.print_statistics(total_time, resume_time, \
                time_transfer_mem, time_decomp_mem, time_delta_mem, \
                time_transfer_disk, time_decomp_disk, time_delta_disk)

        # terminate
        if os.path.exists(memory_pipe):
            os.unlink(memory_pipe)
        if os.path.exists(disk_pipe):
            os.unlink(disk_pipe)
        shutil.rmtree(tmp_dir)

        self.ret_success()


    @staticmethod
    def print_statistics(total_time, resume_time, \
            time_transfer_mem, time_decomp_mem, time_delta_mem, \
            time_transfer_disk, time_decomp_disk, time_delta_disk):
        # Print out Time Measurement
        disk_transfer_time = time_transfer_disk.get()
        disk_decomp_time = time_decomp_disk.get()
        disk_delta_time = time_delta_disk.get()
        mem_transfer_time = time_transfer_mem.get()
        mem_decomp_time = time_decomp_mem.get()
        mem_delta_time = time_delta_mem.get()
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

        transfer_diff = (disk_transfer_end_time-disk_transfer_start_time) + \
                (mem_transfer_end_time-mem_transfer_start_time)
        decomp_diff = (disk_decomp_end_time-disk_transfer_end_time) + \
                (mem_decomp_end_time-mem_transfer_end_time)
        delta_diff = (disk_delta_end_time-disk_decomp_end_time) + \
                (mem_delta_end_time-mem_decomp_end_time)
        message = "\n"
        message += 'Transfer\tDecomp\tDelta\tBoot\tResume\tTotal\n'
        message += "%011.06f\t" % (transfer_diff)
        message += "%011.06f\t" % (decomp_diff)
        message += "%011.06f\t" % (delta_diff)
        message += "%011.06f\t" % (resume_time)
        message += "%011.06f\t" % (total_time)
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
    config_file, error_msg = parse_configfile(settings.config_filename)
    if error_msg:
        print error_msg
        sys.exit(2)

    Const.LOCAL_IPADDRESS = get_local_ipaddress()
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


if __name__ == "__main__":
    status = main()
    sys.exit(status)
