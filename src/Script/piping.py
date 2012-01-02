#!/usr/bin/env python
import os
import sys
import urllib2
from optparse import OptionParser
import time
from multiprocessing import Process, Queue, Pipe, JoinableQueue
import pylzma

# PIPLINING
CHUCK_SIZE = 1024*8
END_OF_FILE = "Transfer End"
download_queue = JoinableQueue()
decomp_queue = JoinableQueue()

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

def network_worker(overlay_url, bandwidth, queue):
    prev = time.time()
    data_size = 0
    url = urllib2.urlopen(overlay_url)
    counter = 0
    while True:
        counter = counter + 1
        chuck = url.read(CHUCK_SIZE)
        data_size = data_size + len(chuck)
        if chuck:
            queue.put(chuck)
            #conn.send(chuck)
        else:
            break

    queue.put(END_OF_FILE)
    time_delta = time.time()-prev
    print "[Time] transfer : %s (loop: %d)" % (str(time_delta), counter)
    print "Bandwidth: %d Mbps(%d/%d)" % (data_size*8.0/time_delta/1000/1000, data_size, time_delta)


def decomp_worker(in_queue, out_queue):
    data_size = 0
    counter = 0
    tmp_file = open(os.path.join(".", "test.tmp"), "wb")
    obj = pylzma.decompressobj()
    while True:
        chuck = in_queue.get()
        if chuck == END_OF_FILE:
            break;

        counter = counter + 1
        data_size = data_size + len(chuck)
        decomp_chuck = obj.decompress(chuck)

        tmp_file.write(decomp_chuck)
        in_queue.task_done()
        out_queue.put(decomp_chuck)

    out_queue.put(END_OF_FILE)
    print "Total looping : %d" % (counter)


def delta_worker(in_queue, out_file):
    data_size = 0
    counter = 0
    while True:
        chuck = in_queue.get()
        if chuck == END_OF_FILE:
            break;

        counter = counter + 1
        data_size = data_size + len(chuck)
        print "in delta : %d, %d" %(counter, data_size)
        out_file.write(chuck)
        in_queue.task_done()


def piping_synthesis(vm_name, bandwidth):
    global download_queue
    global decomp_queue
    global CHUCK_SIZE
    disk_url, mem_url, base_disk, base_mem = get_download_url(vm_name)
    tmp_disk_file = open(os.path.join(".", disk_url.split("/")[-1] + ".tmp"), "wb")

    prev = time.time()
    download_process = Process(target=network_worker, args=(disk_url, bandwidth, download_queue))
    decomp_process = Process(target=decomp_worker, args=(download_queue, decomp_queue))
    delta_process = Process(target=delta_worker, args=(decomp_queue, tmp_disk_file))

    download_process.start()
    decomp_process.start()
    delta_process.start()

    delta_process.join()
    print "[Time] Total Time : " + str(time.time()-prev)


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
