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
import msgpack
import urllib2

from optparse import OptionParser
from multiprocessing import Process, JoinableQueue, Queue, Manager
from tempfile import NamedTemporaryFile
import json
import tempfile
import struct
import libvirt_cloudlet as cloudlet
from Const import Const
from lzma import LZMADecompressor
import shutil
from datetime import datetime

Log = cloudlet.CloudletLog("log_synthesis-%s" % str(datetime.now()).split(" ")[1])

application = ['moped', 'face']
BaseVM_list = []

class Server_Const(object):
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

def network_worker(overlay_urls, demanding_queue, out_queue, time_queue, chunk_size):
    start_time= time.time()
    total_read_size = 0
    counter = 0
    index = 0 
    finished_url = list()
    out_of_order_count = 0
    total_urls_count = len(overlay_urls)
    while len(overlay_urls) > 0:
        urgent_overlay_url = None
        while not demanding_queue.empty():
            # demanding_queue can have multiple same request
            demanding_url = demanding_queue.get()
            #print "getting from demading queue %s" % demanding_url
            if demanding_url not in finished_url:
                urgent_overlay_url = demanding_url
                break

        if urgent_overlay_url != None:
            # process urgent overlay first
            overlay_url = urgent_overlay_url
            overlay_urls.remove(overlay_url)
            out_of_order_count += 1
            #print "find urgent : %s" % urgent_overlay_url
        else:
            # No urgent request, process as normal
            overlay_url = overlay_urls.pop(0)

        #print "reading %d, %s" % (index, overlay_url)
        finished_url.append(overlay_url)
        stream = urllib2.urlopen(overlay_url)
        while True:
            counter += 1
            chunk = stream.read(chunk_size)
            total_read_size += len(chunk)
            if chunk:
                out_queue.put(chunk)
            else:
                break
        index += 1

    out_queue.put(Server_Const.END_OF_FILE)
    end_time = time.time()
    time_delta= end_time-start_time
    time_queue.put({'start_time':start_time, 'end_time':end_time})
    print "[Transfer] out-of-order fetching : %d / %d == %5.2f %%" % \
            (out_of_order_count, total_urls_count, \
            100.0*out_of_order_count/total_urls_count)
    try:
        print "[Transfer] : (%s)~(%s)=(%s) (%d loop, %d bytes, %lf Mbps)" % \
                (start_time, end_time, (time_delta),\
                counter, total_read_size, \
                total_read_size*8.0/time_delta/1024/1024)
    except ZeroDivisionError:
        print "[Transfer] : (%s)~(%s)=(%s) (%d, %d)" % \
                (start_time, end_time, (time_delta),\
                counter, total_read_size)


def decomp_worker(in_queue, pipe_filepath, time_queue, temp_overlay_file=None):
    start_time = time.time()
    data_size = 0
    counter = 0
    decompressor = LZMADecompressor()
    pipe = open(pipe_filepath, "w")

    while True:
        chunk = in_queue.get()
        if chunk == Server_Const.END_OF_FILE:
            break
        data_size = data_size + len(chunk)
        decomp_chunk = decompressor.decompress(chunk)

        in_queue.task_done()
        pipe.write(decomp_chunk)
        if temp_overlay_file:
            temp_overlay_file.write(decomp_chunk)
        counter = counter + 1

    decomp_chunk = decompressor.flush()
    pipe.write(decomp_chunk)
    pipe.close()
    if temp_overlay_file:
        temp_overlay_file.write(decomp_chunk)
        temp_overlay_file.close()

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
                Const.get_basepath(base_path)
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
        json_ret = json.dumps({"command":0x22, "return":"SUCCESS", "LaunchVM-IP":Server_Const.LOCAL_IPADDRESS})
        print "SUCCESS to launch VM"
        json_size = struct.pack("!I", len(json_ret))
        self.request.send(json_size)
        self.wfile.write(json_ret)

    def _check_validity(self, request):
        # self.request is the TCP socket connected to the clinet
        data = request.recv(4)
        msgpack_size = struct.unpack("!I", data)[0]

        # recv JSON header
        msgpack_data = request.recv(msgpack_size)
        while len(msgpack_data) < msgpack_size:
            msgpack_data += request.recv(msgpack_size- len(msgpack_data))
        header = msgpack.unpackb(msgpack_data)

        try:
            base_hashvalue = header.get(Const.META_BASE_VM_SHA256, None)
        except KeyError:
            message = 'No key is in JSON'
            print message
            self.ret_fail(message)
            return
        # check base VM
        for base_vm in BaseVM_list:
            if base_hashvalue == base_vm.get('sha256', None):
                base_path = base_vm['path']
                print "[INFO] New client request %s VM" \
                        % (base_path)
                return [base_path, header]
        message = "Cannot find matching Base VM\nsha256: %s" % (base_hashvalue)
        print message
        self.ret_fail(message)
        return None

    def handle(self):
        # check_base VM
        Log.write("----------------------- New Connection --------------\n")
        start_time = time.time()
        header_start_time = time.time()
        base_path, meta_info = self._check_validity(self.request)
        url_manager = Manager()
        overlay_urls = url_manager.list()
        for blob in meta_info[Const.META_OVERLAY_FILES]:
            url = blob[Const.META_OVERLAY_FILE_NAME]
            overlay_urls.append(url)
        Log.write("Base VM     : %s\n" % base_path)
        Log.write("Application : %s\n" % str(overlay_urls[0]))
        Log.write("Blob count  : %d\n" % len(overlay_urls[0]))
        if base_path == None or meta_info == None or overlay_urls == None:
            message = "Failed, Invalid header information"
            print message
            self.wfile.write(message)            
            self.ret_fail()
            return
        (base_diskmeta, base_mem, base_memmeta) = \
                Const.get_basepath(base_path, check_exist=True)
        header_end_time = time.time()

        # read overlay files
        # create named pipe to convert queue to stream
        time_transfer = Queue(); time_decomp = Queue();
        time_delta = Queue(); time_fuse = Queue();
        tmp_dir = tempfile.mkdtemp()
        temp_overlay_filepath = os.path.join(tmp_dir, "overlay_file")
        temp_overlay_file = open(temp_overlay_filepath, "w+b")
        overlay_pipe = os.path.join(tmp_dir, 'overlay_pipe')
        os.mkfifo(overlay_pipe)

        # overlay
        demanding_queue = Queue()
        download_queue = JoinableQueue()
        download_process = Process(target=network_worker, 
                args=(
                    overlay_urls, demanding_queue, download_queue, time_transfer, Server_Const.TRANSFER_SIZE, 
                    )
                )
        decomp_process = Process(target=decomp_worker,
                args=(
                    download_queue, overlay_pipe, time_decomp, temp_overlay_file,
                    )
                )
        modified_img, modified_mem, fuse, delta_proc, fuse_thread = \
                cloudlet.recover_launchVM(base_path, meta_info, overlay_pipe, 
                        log=sys.stdout, demanding_queue=demanding_queue)
        delta_proc.time_queue = time_delta
        fuse_thread.time_queue = time_fuse

        # resume VM
        resumed_VM = cloudlet.ResumedVM(modified_img, modified_mem, fuse)
        time_start_resume = time.time()
        resumed_VM.start()
        time_end_resume = time.time()
        self.ret_success()

        # start processes
        download_process.start()
        decomp_process.start()
        delta_proc.start()
        fuse_thread.start()
        fuse_thread.join()

        end_time = time.time()
        total_time = (end_time-start_time)

        # printout result
        SynthesisTCPHandler.print_statistics(start_time, end_time, \
                time_transfer, time_decomp, time_delta, time_fuse, \
                print_out=Log, resume_time=(time_end_resume-time_start_resume))

        # terminate
        resumed_VM.join()
        #cloudlet.connect_vnc(resumed_VM.machine)
        while True:
            user_input = raw_input("q to quit: ")
            if user_input == 'q':
                break

        # printout synthesis statistics
        mem_access_list = resumed_VM.monitor.mem_access_chunk_list
        disk_access_list = resumed_VM.monitor.disk_access_chunk_list
        cloudlet.synthesis_statistics(meta_info, temp_overlay_filepath, \
                mem_access_list, disk_access_list, \
                print_out=Log)

        delta_proc.join()
        delta_proc.finish()
        resumed_VM.terminate()
        fuse.terminate()

        if os.path.exists(overlay_pipe):
            os.unlink(overlay_pipe)
        shutil.rmtree(tmp_dir)

    @staticmethod
    def print_statistics(start_time, end_time, \
            time_transfer, time_decomp, time_delta, time_fuse,
            print_out=sys.stdout, resume_time=0):
        # Print out Time Measurement
        transfer_time = time_transfer.get()
        decomp_time = time_decomp.get()
        delta_time = time_delta.get()
        fuse_time = time_fuse.get()
        transfer_start_time = transfer_time['start_time']
        transfer_end_time = transfer_time['end_time']
        decomp_start_time = decomp_time['start_time']
        decomp_end_time = decomp_time['end_time']
        delta_start_time = delta_time['start_time']
        delta_end_time = delta_time['end_time']
        fuse_start_time = fuse_time['start_time']
        fuse_end_time = fuse_time['end_time']

        transfer_diff = (transfer_end_time-transfer_start_time)
        decomp_diff = (decomp_end_time-transfer_end_time)
        delta_diff = (fuse_end_time-decomp_end_time)

        message = "\n"
        message += "Pipelined measurement\n"
        message += 'Transfer\tDecomp\t\tDelta(Fuse)\tResume\t\tTotal\n'
        message += "%011.06f\t" % (transfer_diff)
        message += "%011.06f\t" % (decomp_diff)
        message += "%011.06f\t" % (delta_diff)
        message += "%011.06f\t" % (resume_time)
        message += "%011.06f\t" % (end_time-start_time)
        message += "\n"
        print_out.write(message)


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

        Server_Const.LOCAL_IPADDRESS = "0.0.0.0" # get_local_ipaddress()
        server_address = (Server_Const.LOCAL_IPADDRESS, Server_Const.SERVER_PORT_NUMBER)
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
    if not cloudlet.validate_congifuration():
        sys.stderr.write("failed to validate configuration\n")
        sys.exit(1)
    status = main()
    sys.exit(status)
