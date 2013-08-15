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
import urllib2
from multiprocessing import Process, Queue, Pipe, JoinableQueue
from optparse import OptionParser
from datetime import datetime
import subprocess
import sys
import tempfile
from cloudlet.server import network_worker, decomp_worker, delta_worker

CHUNK_SIZE = 1024*16

def piping_synthesis(overlay_url, base_path):
    prev = datetime.now()
    delta_processes = []
    tmp_dir = tempfile.mkdtemp()
    time_transfer = Queue()
    time_decomp = Queue()
    time_delta = Queue()

    download_queue = JoinableQueue()
    decomp_queue = JoinableQueue()
    (download_pipe_in, download_pipe_out) = Pipe()
    (decomp_pipe_in, decomp_pipe_out) = Pipe()
    recover_file = os.path.join(tmp_dir, overlay_url.split("/")[-1] + ".recover")
    
    # synthesis
    print "[INFO] Start Downloading at %s" % (overlay_url)
    url = urllib2.urlopen(overlay_url)
    download_process = Process(target=network_worker, args=(url, download_queue, time_transfer, CHUNK_SIZE))
    decomp_process = Process(target=decomp_worker, args=(download_queue, decomp_queue, time_decomp))
    delta_process = Process(target=delta_worker, args=(decomp_queue, time_delta, base_path, recover_file))
    delta_processes.append(delta_process)
    download_process.start()
    decomp_process.start()
    delta_process.start()

    # wait until end
    delta_process.join()
    print "\n[Time] Total Time for synthesis(including download) : " + str(datetime.now()-prev)
    return recover_file


def mount_launchVM(launch_disk_path, base_vm_path):
    mount_dir = tempfile.mkdtemp()
    raw_vm = launch_disk_path + ".raw"

    # rebase overlay img
    start_time = datetime.now()
    cmd_rebase = "qemu-img rebase -f qcow2 -u -b %s -F qcow2 %s" % (base_vm_path, launch_disk_path)
    proc = subprocess.Popen(cmd_rebase, shell=True, stdin=sys.stdin, stdout=sys.stdout)
    proc.wait()
    if proc.returncode != 0:
        print >> sys.stderr, "Error, Failed to QEMU-IMG Rebasing"
        print >> sys.stderr, "CMD: %s" % (cmd_rebase)
        sys.exit(2)
    convert_time = datetime.now()-start_time


    # qemu-img convert
    start_time = datetime.now()
    cmd_convert = "qemu-img convert -f qcow2 %s -O raw %s" % (launch_disk_path, raw_vm)
    proc = subprocess.Popen(cmd_convert, shell=True, stdin=sys.stdin, stdout=sys.stdout)
    proc.wait()
    if proc.returncode != 0 or os.path.exists(raw_vm) == False:
        print >> sys.stderr, "Error, Failed to QEMU-IMG Converting"
        print >> sys.stderr, "CMD: %s" % (cmd_convert)
        sys.exit(2)
    convert_time = datetime.now()-start_time

    # mount
    start_time = datetime.now()
    cmd_mapping = "sudo kpartx -av %s" % (raw_vm)
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

    print "[TIME] QCOW2 to Raw converting time : %s" % (str(convert_time))
    print "[TIME] Raw Mouting time : %s" % (str(mount_time))
    return mount_dir, raw_vm


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

def clean_up(raw_vm, launch_vm):
    print "[INFO] clean up files : %s, %s" % (os.path.abspath(raw_vm), os.path.abspath(launch_vm))
    os.remove(raw_vm)
    os.remove(launch_vm)


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
    if settings.overlay_download_url == None or settings.base_path == None or settings.output_mount == None:
        parser.error('Read usage')

    if not os.path.exists(settings.base_path):
        print >> sys.stderr, "[Error] Base VM does not exist at %s" % (settings.base_path)
        sys.exit(2)

    if not os.path.exists(settings.output_mount):
        print >> sys.stderr, "[Error] Mount directory does not exist at %s" % ( settings.output_mount)
        sys.exit(2)

    if not os.path.ismount(settings.output_mount):
        print >> sys.stderr, "[Error] It is not valid mount point at %s" % (settings.output_mount)
        sys.exit(2)

    return settings, args



def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])

    # Synthesis overlay
    qcow_launch_image = piping_synthesis(settings.overlay_download_url, settings.base_path)

    # Mount VM File system
    launchVM_dir, VM_path =  mount_launchVM(qcow_launch_image, settings.base_path)

    # rsync VM to origianl disk
    rsync_overlayVM(launchVM_dir, settings.output_mount)

    # clean up
    clean_up(VM_path, qcow_launch_image)

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
