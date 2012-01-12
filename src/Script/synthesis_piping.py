#!/usr/bin/env python
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
import atexit

# PIPLINING
CHUNK_SIZE = 1024*8
END_OF_FILE = "Overlay Transfer End Marker"
operation_mode = ('run', 'mock')
application_names = ("moped", "face", "speech", "null")

# Web server for Andorid Client
SERVER_PORT_NUMBER = 8022
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


def network_worker(data, queue, chunk_size, data_size=sys.maxint):
    start_time= datetime.now()
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

    queue.put(END_OF_FILE)
    end_time = datetime.now()
    time_delta= end_time-start_time
    try:
        print "[Download] time : (%s)-(%s)=(%s) (%d, %d, %d)" % (start_time.strftime('%X'), end_time.strftime('%X'), str(end_time-start_time), counter, total_read_size, total_read_size*8.0/time_delta.seconds/1000/1000)
    except ZeroDivisionError:
        print "[Download] time : (%s)-(%s)=(%s) (%d, %d)" % (start_time.strftime('%X'), end_time.strftime('%X'), str(end_time-start_time), counter, total_read_size)


def decomp_worker(in_queue, out_queue):
    start_time = datetime.now()
    data_size = 0
    counter = 0
    obj = pylzma.decompressobj()
    while True:
        counter = counter + 1
        chunk = in_queue.get()
        if chunk == END_OF_FILE:
            break
        data_size = data_size + len(chunk)
        decomp_chunk = obj.decompress(chunk)
        #print "in decomp : %d %d" % (data_size, len(decomp_chunk))

        in_queue.task_done()
        out_queue.put(decomp_chunk)

    out_queue.put(END_OF_FILE)
    end_time = datetime.now()
    print "[Decomp] time : (%s)-(%s)=(%s) (%d, %d)" % (start_time.strftime('%X'), end_time.strftime('%X'), str(end_time-start_time), counter, data_size)


def delta_worker(in_queue, base_filename, out_filename):
    start_time = datetime.now()
    data_size = 0
    counter = 0

    # create named pipe for xdelta3
    # out_file = open(out_filename, 'wb')
    out_pipename = (out_filename + ".fifo")
    if os.path.exists(out_pipename):
        os.unlink(out_pipename)
    if os.path.exists(out_filename):
        os.unlink(out_filename)
    os.mkfifo(out_pipename)

    # run xdelta 3 with named pipe
    command_str = "xdelta3 -df -s %s %s %s" % (base_filename, out_pipename, out_filename)
    xdelta_process = subprocess.Popen(command_str, shell=True)
    out_pipe = open(out_pipename, "w")

    while True:
        counter = counter + 1
        chunk = in_queue.get()
        if chunk == END_OF_FILE:
            break;

        data_size = data_size + len(chunk)
        #print "in delta : %d, %d, %d" %(counter, len(chunk), data_size)

        out_pipe.write(chunk)
        in_queue.task_done()

    out_pipe.close()
    ret = xdelta_process.wait()
    os.unlink(out_pipename)
    end_time = datetime.now()

    if ret == 0:
        print "[delta] time : (%s)-(%s)=(%s) (%d, %d)" % (start_time.strftime('%X'), end_time.strftime('%X'), str(end_time-start_time), counter, data_size)
        return True
    else:
        print "Error, xdelta process has not successed"
        return False


def piping_synthesis(vm_name):
    disk_url, mem_url, base_disk, base_mem = get_download_url(vm_name)
    prev = datetime.now()
    recover_file = []
    delta_processes = []
    tmp_dir = tempfile.mkdtemp()
    for (overlay_url, base_name) in ((disk_url, base_disk), (mem_url, base_mem)):
        download_queue = JoinableQueue()
        decomp_queue = JoinableQueue()
        (download_pipe_in, download_pipe_out) = Pipe()
        (decomp_pipe_in, decomp_pipe_out) = Pipe()
        out_filename = os.path.join(tmp_dir, overlay_url.split("/")[-1] + ".recover")
        recover_file.append(out_filename)
        
        url = urllib2.urlopen(overlay_url)
        download_process = Process(target=network_worker, args=(url, download_queue, CHUNK_SIZE))
        decomp_process = Process(target=decomp_worker, args=(download_queue, decomp_queue))
        delta_process = Process(target=delta_worker, args=(decomp_queue, base_name, out_filename))
        delta_processes.append(delta_process)
        
        download_process.start()
        decomp_process.start()
        delta_process.start()

    for delta_p in delta_processes:
        delta_p.join()

    telnet_port = 9999
    vnc_port = 2
    exe_time = run_snapshot(recover_file[0], recover_file[1], telnet_port, vnc_port, wait_vnc_end=False)
    print "[Time] VM Resume : " + exe_time
    print "\n[Time] Total Time except VM Resume : " + str(datetime.now()-prev)


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
    print "* Configuration List"
    for vm_info in VM_list:
        if vm_info['type'].lower() == 'basevm':
            BaseVM_list.append(vm_info)
            print "%s : (%s, %s)" % (vm_info['name'], vm_info['diskimg_path'], vm_info['memorysnapshot_path'])
    print ""

    return json_data, None


class SynthesisTCPHandler(SocketServer.StreamRequestHandler):

    def finish(self):
        pass

    def ret_fail(self, message):
        print "Error, %s" % str(message)
        json_str = {"Error":message}
        self.wfile.write(json.dumps(json_str))

    def ret_success(self):
        print "SUCCESS to launch VM"
        json_str = {"SUCCESS":"SUCCESS"}
        self.wfile.write(json.dumps(json_str))

    def handle(self):
        # self.request is the YCP socket connected to the clinet
        data = self.request.recv(4)
        json_size = struct.unpack("I", data)[0]

        # recv JSON header
        json_str = self.request.recv(json_size)
        json_data = json.loads(json_str)
        if 'VM' not in json_data or len(json_data['VM']) == 0:
            self.ret_fail("No VM Key at JSON")
            return

        vm_name = ''
        try:
            vm_name = json_data['VM'][0]['base_name']
            disk_size = int(json_data['VM'][0]['overlay_disk_size'])
            mem_size = int(json_data['VM'][0]['overlay_memory_size'])
            print "received info %s" % (vm_name)
        except KeyError:
            message = 'No key is in JSON'
            print message
            self.ret_fail(message)
            return

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
        prev_time = datetime.now()
        tmp_dir = tempfile.mkdtemp()
        recover_file = []
        delta_processes = []
        for overlay_name, file_size, base in (('disk', disk_size, base_disk_path), ('memory', mem_size, base_mem_path)):
            download_queue = JoinableQueue()
            decomp_queue = JoinableQueue()
            (download_pipe_in, download_pipe_out) = Pipe()
            (decomp_pipe_in, decomp_pipe_out) = Pipe()
            out_filename = os.path.join(tmp_dir, overlay_name + ".recover")
            recover_file.append(out_filename)
            
            download_process = Process(target=network_worker, args=(self.rfile, download_queue, CHUNK_SIZE, file_size))
            decomp_process = Process(target=decomp_worker, args=(download_queue, decomp_queue))
            delta_process = Process(target=delta_worker, args=(decomp_queue, base, out_filename))
            download_process.start()
            decomp_process.start()
            delta_process.start()
            delta_processes.append(delta_process)

            print "Waiting for download disk first"
            download_process.join()
            
        for delta_p in delta_processes:
            delta_p.join()

        telnet_port = 9999
        vnc_port = 2
        exe_time = run_snapshot(recover_file[0], recover_file[1], telnet_port, vnc_port, wait_vnc_end=False)
        print "[Time] VM Resume : " + exe_time
        print "\n[Time] Total Time except VM Resume : " + str(datetime.now()-prev_time)
        self.ret_success()


def main(argv=None):
    mode, settings, args = process_command_line(sys.argv[1:])
    if mode == operation_mode[0]: # run mode
        config_file, error_msg = parse_configfile(settings.config_filename)
        if error_msg:
            print error_msg
            sys.exit(2)

        server_address = ("server.krha.kr", SERVER_PORT_NUMBER)
        print "Open TCP Server (%s)" % (str(server_address))
        server = SocketServer.TCPServer(server_address, SynthesisTCPHandler)
        server.allow_reuse_address = True
        server.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

        #atexit.register(server.socket.close)
        #atexit.register(server.shutdown)

        try:
            server.serve_forever()
        except KeyboardInterrupt:
            sys.exit(0)


    elif mode == operation_mode[1]: # mock mode
        piping_synthesis(settings.vmname)
    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
