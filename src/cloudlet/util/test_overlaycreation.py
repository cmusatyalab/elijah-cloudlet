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
from optparse import OptionParser
import sys
import subprocess
import shutil
import select
from datetime import datetime
import paramiko
import socket
import time
import telnetlib


def wait_until_finish(channel):
    while not channel.exit_status_ready():
        rl, wl, xl = select.select([channel],[],[],0.0)
        if len(rl) > 0:
            print channel.recv(1024)
        time.sleep(0.1)


def install_program_eclipse(ssh_port):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    # Waiting for boot-up at most 30s
    counter = 0
    while counter < 30:
        try:
            ssh.connect(hostname='localhost', port=ssh_port, username='root', password='cloudlet')
            break
        except (socket.error, paramiko.SSHException):
            counter += 1
            print "[INFO] Connecting to %s(waiting..%d)" % ('localhost', counter)
            time.sleep(1)


    #download install script
    channel = ssh.get_transport().open_session()
    cmd_download = "wget http://dagama.isr.cs.cmu.edu/download/android.tgz"
    #cmd_download = "apt-get install --force-yes -y gimp"
    print "[install] download android sdk file"
    channel.exec_command(cmd_download)
    wait_until_finish(channel)

    cmd_install = "sync"
    channel = ssh.get_transport().open_session()
    print "[install] sync to file system"
    channel.exec_command(cmd_install)
    wait_until_finish(channel)
    ssh.close()


def install_program_MOPED(ssh_port):
    ssh = paramiko.SSHClient()
    ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())

    # Waiting for boot-up at most 30s
    counter = 0
    while counter < 30:
        try:
            ssh.connect(hostname='localhost', port=ssh_port, username='root', password='cloudlet')
            break
        except (socket.error, paramiko.SSHException):
            counter += 1
            print "[INFO] Connecting to %s(waiting..%d)" % ('localhost', counter)
            time.sleep(1)


    #download install script
    channel = ssh.get_transport().open_session()
    cmd_download = "wget http://dagama.isr.cs.cmu.edu/download/MOPED_install_10.04"
    channel.exec_command(cmd_download)
    wait_until_finish(channel)

    #install MOPED
    channel = ssh.get_transport().open_session()
    cmd_install = "source MOPED_install_10.04"
    channel.exec_command(cmd_install)
    wait_until_finish(channel)

    cmd_install = "sync"
    channel = ssh.get_transport().open_session()
    channel.exec_command(cmd_install)
    wait_until_finish(channel)
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

    # Terminate VM
    tn = telnetlib.Telnet('localhost', telnet_port)
    tn.read_until("(qemu)", 10)
    tn.write("quit\n")
    time.sleep(3)

    return comp, time1


def print_result(exec_time, raw_image, copied_image, overlay_image, comp_image):
    print "[Time:%s] Original:%d \t Modified:%d \t xdelta:%d \t overlay:%d" % (exec_time, os.path.getsize(raw_image), \
            os.path.getsize(copied_image), os.path.getsize(overlay_image), os.path.getsize(comp_image))


def raw_and_copy(raw_image, raw_mem):
    copied_image = os.path.abspath(raw_image) + ".copied"
    overlay_image = os.path.abspath(copied_image) + ".overlay"
    shutil.copyfile(raw_image, copied_image)

    telnet_port = 9999
    cloudlet.run_image(copied_image, telnet_port, 1, wait_vnc_end=False, cdrom=None, terminal_mode=True)
    install_program_eclipse(2222)
    comp_image, exec_time = create_overlay(telnet_port, raw_image, copied_image, overlay_image)
    print_result(exec_time, raw_image, copied_image, overlay_image, comp_image)


def raw_and_qcow(raw_image, raw_mem):
    cow_image = os.path.abspath(raw_image) + ".qcow2"
    overlay_image = os.path.abspath(cow_image) + ".overlay"
    create_qcow(raw_image, cow_image)

    telnet_port = 9999
    cloudlet.run_image(cow_image, telnet_port, 1, wait_vnc_end=False, cdrom=None, terminal_mode=True)
    install_program_eclipse(2222)
    comp_image, exec_time = create_overlay(telnet_port, raw_image, cow_image, overlay_image)
    print_result(exec_time, raw_image, cow_image, overlay_image, comp_image)


def qcow_and_copy(qcow_base, raw_mem):
    copied_image = os.path.abspath(qcow_base) + ".copied"
    overlay_image = os.path.abspath(copied_image) + ".overlay"
    shutil.copyfile(qcow_base, copied_image)

    telnet_port = 9999
    cloudlet.run_image(copied_image, telnet_port, 1, wait_vnc_end=False, cdrom=None, terminal_mode=True)
    install_program_eclipse(2222)
    comp_image, exec_time = create_overlay(telnet_port, qcow_base, copied_image, overlay_image)
    print_result(exec_time, qcow_base, copied_image, overlay_image, comp_image)


def qcow_and_qcow(qcow_base, raw_mem):
    cow_image = os.path.abspath(qcow_base) + ".qcow2"
    overlay_image = os.path.abspath(cow_image) + ".overlay"
    create_qcow(qcow_base, cow_image)

    telnet_port = 9999
    cloudlet.run_image(cow_image, telnet_port, 1, wait_vnc_end=False, cdrom=None, terminal_mode=True)
    install_program_eclipse(2222)
    comp_image, exec_time = create_overlay(telnet_port, qcow_base, cow_image, overlay_image)
    print_result(exec_time, qcow_base, cow_image, overlay_image, comp_image)


def convert_to_qcow(raw_image, cow_image):
    cmd = "qemu-img convert -f raw %s -O qcow2 %s" % (raw_image, cow_image)
    proc = subprocess.Popen(cmd, shell=True)
    proc.wait()
    if proc.returncode != 0:
        print >> sys.stderr, "Error, Failed to make qcow2 image"
        sys.exit(2)

    return cow_image


def create_qcow(raw_image, cow_image):
    cmd = 'qemu-img create -f qcow2 -b ' + raw_image + ' ' + cow_image
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

    return settings, args


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    cow_image = settings.raw_image[0:settings.raw_image.rindex(".")] + ".qcow2"
    convert_to_qcow(settings.raw_image, cow_image)

    raw_and_copy(settings.raw_image, None)
    raw_and_qcow(settings.raw_image, None)
    qcow_and_copy(cow_image, None)
    qcow_and_qcow(cow_image, None)


if __name__ == "__main__":
    CLOUDLET_DIR = '/home/krha/cloudlet/src/server/'
    import_dir = os.path.abspath(CLOUDLET_DIR)
    if import_dir not in sys.path:
        sys.path.insert(0, import_dir)
        import cloudlet

    status = main(sys.argv)
    sys.exit(status)
