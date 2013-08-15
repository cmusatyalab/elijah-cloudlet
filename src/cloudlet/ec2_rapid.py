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
import time
import urllib2
from multiprocessing import Process, Queue, Pipe, JoinableQueue
from optparse import OptionParser
from datetime import datetime
import subprocess
import shutil
import sys
import tempfile
from multiprocessing import JoinableQueue, Queue, Manager

from cloudlet import synthesis as synthesis
from cloudlet import server as synthesis_server
from cloudlet.Configuration import Const
from cloudlet import msgpack

CHUNK_SIZE = 1024*16

def read_all(url_stream):
    data = ''
    while True:
        chunk = url_stream.read(CHUNK_SIZE)
        if chunk:
            data += chunk
        else:
            break
    return data
        

def piping_synthesis(overlay_url, base_path):
    # check_base VM
    start_time = time.time()
    meta_stream = urllib2.urlopen(overlay_url)
    meta_raw = read_all(meta_stream)
    meta_info = msgpack.unpackb(meta_raw)
    url_manager = Manager()
    overlay_urls = url_manager.list()
    url_prefix = os.path.dirname(overlay_url)
    for blob in meta_info[Const.META_OVERLAY_FILES]:
        blob_filename = os.path.basename(blob[Const.META_OVERLAY_FILE_NAME])
        url = os.path.join(url_prefix, blob_filename)
        overlay_urls.append(url)
    (base_diskmeta, base_mem, base_memmeta) = \
            Const.get_basepath(base_path, check_exist=True)

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
    download_process = Process(target=synthesis_server.network_worker, 
            args=(
                overlay_urls, demanding_queue, download_queue, time_transfer, CHUNK_SIZE,
                )
            )
    decomp_process = Process(target=synthesis_server.decomp_worker,
            args=(
                download_queue, overlay_pipe, time_decomp, temp_overlay_file,
                )
            )
    modified_img, modified_mem, fuse, delta_proc, fuse_thread = \
            synthesis.recover_launchVM(base_path, meta_info, overlay_pipe, 
                    log=sys.stdout, demanding_queue=demanding_queue)
    delta_proc.time_queue = time_delta
    fuse_thread.time_queue = time_fuse

    # start processes
    download_process.start()
    decomp_process.start()
    delta_proc.start()
    fuse_thread.start()

    # wait for end
    delta_proc.join()
    fuse_thread.join()

    # printout result
    end_time = time.time()
    total_time = (end_time-start_time)
    synthesis_server.SynthesisTCPHandler.print_statistics(start_time, end_time, \
            time_transfer, time_decomp, time_delta, time_fuse, \
            print_out=sys.stdout)

    delta_proc.finish()

    if os.path.exists(overlay_pipe):
        os.unlink(overlay_pipe)
    shutil.rmtree(tmp_dir)

    print "\n[Time] Total Time for synthesis(including download) : %f" % (total_time)
    return fuse


def mount_launchVM(fuse):
    start_time = datetime.now()
    fuse_image = os.path.join(fuse.mountpoint, 'disk', 'image')
    mount_dir = tempfile.mkdtemp()
    #temp_raw_vm = NamedTemporaryFile(prefix="cloudlet-image-", delete=False)
    #raw_vm = temp_raw_vm.name
    #print "[INFO] copyting file from %s to %s" % (fuse_image, raw_vm)
    #shutil.copyfile(fuse_image, raw_vm)

    # mount
    cmd_mapping = "sudo kpartx -av %s" % (fuse_image)
    proc = subprocess.Popen(cmd_mapping, shell=True, stdin=sys.stdin, stdout=subprocess.PIPE)
    proc.wait()
    if proc.returncode != 0:
        print >> sys.stderr, "Error, Failed to kpartx"
        sys.exit(2)
    output = proc.stdout.readline()
    output = output[output.find("loop"):]
    mapper_dev = output.split(" ")[0].strip()
    cmd_mount = "sudo mount /dev/mapper/%s %s" % (mapper_dev, mount_dir)
    proc = subprocess.Popen(cmd_mount, shell=True, stdin=sys.stdin, stdout=sys.stdout)
    proc.wait()
    if proc.returncode != 0:
        print >> sys.stderr, "Error, Failed to mount, ret code : " + str(proc.returncode)
        sys.exit(2)
    mount_time = datetime.now()-start_time

    print "[TIME] Raw Mouting time : %s" % (str(mount_time))
    return mount_dir, fuse_image


def rsync_overlayVM(vm_dir, instance_dir):
    instance_dir = os.path.join(instance_dir, ".")

    # restart init process because it might inidicate original init process
    print "[INFO] Restart Init processs"
    subprocess.Popen("sudo telinit u", shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()

    # possible mount residue at origianl block
    new_mount_dir = os.path.join(instance_dir, "mnt")
    print "[INFO] Remove possible mountings at : " + new_mount_dir
    subprocess.Popen("sudo umount %s" % (new_mount_dir), shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()

    # erase instance dir
    '''
    if os.path.exists(instance_dir):
        message = "Are you sure to delete all files at %s?(y/N) " % (instance_dir)
        ret = raw_input(message)
        if str(ret) != 'y':
            sys.exit(1)
    else:
        print >> sys.strerr, "Instance directory does not exists, " + str(instance_dir)
        sys.exit(1)
    
    cmd_erase = "sudo rm -rf %s" % os.path.join(instance_dir, "*")
    print "[INFO] Erase instance dir: %s" % (cmd_erase)
    subprocess.Popen(cmd_erase, shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()
    '''

    # rsync
    print "[INFO] rsync from %s to %s" % (vm_dir, instance_dir)
    start_time = datetime.now()
    cmd_rsync = "sudo rsync -aHx --delete %s/ %s/" % (vm_dir, instance_dir)
    subprocess.Popen(cmd_rsync, shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()
    subprocess.Popen("sudo sync", shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()
    rsync_time = datetime.now()-start_time
    print "[TIME] rsync time : %s" % (str(rsync_time))

    # umount
    print "[INFO] umount instance dir, %s" % (instance_dir)
    subprocess.Popen("sudo umount %s" % (instance_dir), shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()
    print "[INFO] umount VM dir, %s" % (vm_dir)
    subprocess.Popen("sudo umount %s" % (vm_dir), shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()
    subprocess.Popen("sudo sync", shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()


def clean_up(fuse, mount_dir, raw_vm):
    '''
    print "[INFO] clean up files : %s, size: %d" % (os.path.abspath(raw_vm), os.path.getsize(raw_vm))
    if os.path.exists(raw_vm):
        os.remove(raw_vm)
    '''
    print "[INFO] clean up temp dir : %s" % (os.path.abspath(mount_dir))
    if os.path.exists(mount_dir):
        print "[INFO] umount VM dir, %s" % (mount_dir)
        subprocess.Popen("sudo umount %s" % (mount_dir), shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()
    if os.path.exists(fuse.mountpoint):
        print "[INFO] umount FUSE dir, %s" % (mount_dir)
        subprocess.Popen("sudo umount %s" % (fuse.mountpoint), shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()


def process_command_line(argv):
    help_message = "\nEC2 synthesis is designed for rapid launching of customizing instance at Amazon EC2"

    parser = OptionParser(usage="usage: %prog -o [Overlay Download URL] -b [Base VM Path] -m [Instance Mount Path]" + help_message,
            version="EC2 Synthesys v0.1.1")
    parser.add_option(
            '-o', '--overlay', action='store', type='string', dest='overlay_download_url',
            help='Set overlay disk download URL.')
    parser.add_option(
            '-b', '--base', action='store', type='string', dest='base_path',
            help='Set Base disk path.')
    parser.add_option(
            '-m', '--mount', action='store', type='string', dest='output_mount',
            help='Set output Mount point. This Mount point is EC2 inital disk.')
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    if settings.overlay_download_url == None: # or settings.base_path == None or settings.output_mount == None:
        parser.error("Read usage\n \
                Example: ./ec2_rapid.py -o http://cloudlet.krha.kr/overlay/ec2/oneiric.overlay-meta -b ~/cloudlet/image/ami-oneiric-server-amd64/oneiric.raw")

    if not os.path.exists(settings.base_path):
        print >> sys.stderr, "[Error] Base VM does not exist at %s" % (settings.base_path)
        sys.exit(2)

    '''
    if not os.path.exists(settings.output_mount):
        print >> sys.stderr, "[Error] Mount directory does not exist at %s" % ( settings.output_mount)
        sys.exit(2)

    if not os.path.ismount(settings.output_mount):
        print >> sys.stderr, "[Error] It is not valid mount point at %s" % (settings.output_mount)
        sys.exit(2)
    '''

    return settings, args



def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])

    # Synthesis overlay
    fuse = piping_synthesis(settings.overlay_download_url, settings.base_path)

    # Mount VM File system
    mount_dir, raw_image = mount_launchVM(fuse)

    # rsync VM to origianl disk
    rsync_overlayVM(mount_dir, settings.output_mount)

    # clean up
    clean_up(fuse, mount_dir, raw_image)
    fuse.terminate()

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
