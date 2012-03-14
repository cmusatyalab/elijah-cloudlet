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
from optparse import OptionParser
import sys
import subprocess
import shutil
from datetime import datetime
import paramiko
import socket
import time
import telnetlib


def wait_until_finish(stdout, stderr, log=True, max_time=20):
    global LOG_FILE
    for x in xrange(max_time):
        ret1 = stdout.readline()
        ret2 = stderr.readline()
        if log:
            sys.stdout.write(ret1)
            sys.stdout.write(ret2)
            sys.stdout.flush()

        if len(ret1) == 0:
            break
        time.sleep(0.01)


def install_program(ssh_port):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    # Waiting for boot-up at most 30s
    counter = 0
    while counter < 30:
        try:
            ssh.connect(hostname='localhost', port=ssh_port, username='krha-cloudlet', password='cloudlet')
            break
        except socket.error:
            counter += 1
            print "[INFO] Connecting to %s(waiting..%d)" % ('localhost', counter)
            time.sleep(1)

    #download install script
    cmd_download = "wget http://dagama.isr.cs.cmu.edu/download/MOPED_install_10.04"
    stdin, stdout, stderr = ssh.exec_command(cmd_download)
    wait_until_finish(stdout, stderr)

    #install MOPED
    cmd_install = "sudo source MOPED_install_10.04"
    stdin, stdout, stderr = ssh.exec_command(cmd_install)
    wait_until_finish(stdout, stderr)
    ssh.close()


def create_overlay(telnet_port, base, tmp, overlay):
    # Stop VM
    tn = telnetlib.Telnet('localhost', telnet_port)
    tn.read_until("(qemu)", 10)
    tn.write("stop\n")
    for i in xrange(20):
        try:
            ret = tn.read_until("(qemu)", 10)
            if ret.find("(qemu)") != -1:
                break;
        except socket.timeout:
            pass
        time.sleep(1)


    prev_time = datetime.now()
    # xdelta
    ret = cloudlet.diff_files(base, tmp, overlay)
    print '[TIME] time for creating overlay : ', str(datetime.now()-prev_time)
    print '[INFO] (%d)-(%d)=(%d): ' % (os.path.getsize(base), os.path.getsize(tmp), os.path.getsize(overlay))
    if ret == None:
        print >> sys.stderr, '[ERROR] cannot create overlay ' + str(overlay)
        if os.path.exists(tmp):
            os.remove(tmp)
        return []
    
    # compression
    comp= overlay + '.lzma'
    comp, time1 = cloudlet.comp_lzma(overlay, comp)
    return comp, time1


def print_result(exec_time, raw_image, copied_image, overlay_image, comp_image):
    print "[Time:%s] Original:%d \t Modified:%d \t xdelta:%d \t overlay:%d" % (exec_time, os.path.getsize(raw_image), \
            os.path.getsize(copied_image), os.path.getsize(overlay_image), os.path.getsize(comp_image))


def raw_and_copy(raw_image, raw_mem):
    copied_image = os.path.abspath(raw_image) + ".copied"
    overlay_image = os.path.abspath(copied_image) + ".overlay"
    #shutil.copyfile(raw_image, copied_image)

    telnet_port = 9999
    cloudlet.run_snapshot(copied_image, raw_mem, telnet_port, 1, wait_vnc_end=True)
    install_program(2222)
    comp_image, exec_time = create_overlay(telnet_port, raw_image, copied_image, overlay_image)
    print_result(exec_time, raw_image, copied_image, overlay_image, comp_image)


def raw_and_qcow(raw_image, raw_mem):
    cow_image = os.path.abspath(raw_image) + ".qcow2"
    overlay_image = os.path.abspath(cow_image) + ".overlay"
    convert_to_qcow(raw_image, cow_image)

    telnet_port = 9999
    cloudlet.run_snapshot(cow_image, raw_mem, telnet_port, 1, wait_vnc_end=True)
    install_program(2222)
    comp_image, exec_time = create_overlay(telnet_port, raw_image, cow_image, overlay_image)
    print_result(exec_time, raw_image, cow_image, overlay_image, comp_image)

def qcow_and_copy(qcow_image):
    pass

def qcow_and_qcow(qcow_image):
    pass

def convert_to_qcow(raw_image, cow_image):
    cmd = "qemu-img convert -f raw %s -O qcow2 %s" % (raw_image, cow_image)
    proc = subprocess.Popen(cmd, shell=True)
    proc.wait()
    if proc.returncode != 0:
        print >> sys.stderr, "Error, Failed to make qcow2 image"
        sys.exit(2)

    return cow_image


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog [option]",
            version="Cloudlet Overlay Creation Efficiency Test")
    parser.add_option(
            '-i', '--image', action='store', type='string', dest='raw_image',
            help='Raw image file')
    parser.add_option(
            '-m', '--memory', action='store', type='string', dest='memory_snapshot',
            help='Memory Snapshot')
    settings, args = parser.parse_args(argv)

    if not os.path.exists(settings.raw_image):
        parser.error('Cannot file input raw image : %s' + settings.raw_image)
    if not os.path.exists(settings.memory_snapshot):
        parser.error('Cannot file input memory snapshot : %s' + settings.memory_snapshot)

    return settings, args


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    raw_and_copy(settings.raw_image, settings.memory_snapshot)
    raw_and_qcow(settings.raw_image, settings.memory_snapshot)
    '''
    qcow_image = convert_to_qcow(settings.raw_image)
    qcow_and_copy(qcow_image, settings.memory_snapshot)
    qcow_and_qcow(qcow_image, settings.memory_snapshot)
    '''


if __name__ == "__main__":
    CLOUDLET_DIR = '/home/krha/cloudlet/src/server/'
    import_dir = os.path.abspath(CLOUDLET_DIR)
    if import_dir not in sys.path:
        print "import cloudlet path(%s)" % import_dir
        sys.path.insert(0, import_dir)
        import cloudlet

    status = main(sys.argv)
    sys.exit(status)
