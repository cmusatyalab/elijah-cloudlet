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

import xdelta3
import os
import commands
import filecmp
import sys
import subprocess
import getopt
import time
from datetime import datetime
from optparse import OptionParser
import telnetlib
import socket
import pylzma

VM_MEMORY = 1024
VCPU_NUMBER = 4
#VM_MEMORY = 2048
#VCPU_NUMBER = 2
#BALLOON_MEM_SIZE = VM_MEMORY

#KVM = '../kvm-qemu/x86_64-softmmu/qemu-system-x86_64'
OVF_TRANSPORTER = "../../image/ubuntu-11.10-x86_64-server/ovftransport.iso"
PORT_FORWARDING = "-redir tcp:9876::9876 -redir tcp:2222::22 -redir tcp:9092::9092 -redir tcp:9093::9093 -redir tcp:9094::9094 --redir tcp:6789::6789 -redir tcp:10191::10191"


def diff_files(source_file, target_file, output_file):
    if os.path.exists(source_file) == False:
        print '[Error] No such file %s' % (source_file)
        return None
    if os.path.exists(target_file) == False:
        print '[Error] No such file %s' % (target_file)
        return None
    if os.path.exists(output_file):
        os.remove(output_file)

    print '[INFO] %s(base) - %s  =  %s' % (os.path.basename(source_file), os.path.basename(target_file), os.path.basename(output_file))
    command_delta = ['xdelta3', '-f', '-s', source_file, target_file, output_file]
    ret = xdelta3.xd3_main_cmdline(command_delta)
    if ret == 0:
        return output_file
    else:
        return None


def merge_file(source_file, overlay_file, output_file):
    #command_patch = ['xdelta3', '-df', '-s', source_file, overlay_file, output_file]
    # ret = xdelta3.xd3_main_cmdline(command_patch)
    command_patch = "xdelta3 -df -s %s %s %s" % (source_file, overlay_file, output_file)
    proc = subprocess.Popen(command_patch, shell=True)
    proc.wait()

    #print command_patch
    if proc.returncode == 0:
        #print "output : %s (%d)" % (output_file, os.path.getsize(output_file))
        return output_file
    else:
        return None


def compare_same(filename1, filename2):
    print '[INFO] checking validity of generated file'
    compare = filecmp.cmp(filename1, filename2)
    if compare == False:
        print >> sys.stderr, '[ERROR] %s != %s' % (os.path.basename(filename1), os.path.basename(filename2))
        return False
    else:
        print '[INFO] SUCCESS to recover'
        return True


# lzma compression
def comp_lzma(inputname, outputname):
    prev_time = datetime.now()

    in_file = open(inputname, 'rb')
    ret_file = open(outputname, 'wb')
    c_fp = pylzma.compressfile(in_file, eos=1, algorithm=2, dictionary=28)
    while True:
        chunk = c_fp.read(8192)
        if not chunk: break
        ret_file.write(chunk)

    in_file.close()
    ret_file.close()
    time_diff = str(datetime.now()-prev_time)
    return outputname, str(time_diff)


# lzma decompression
def decomp_lzma(inputname, outputname):
    prev_time = datetime.now()
    comp_file = open(inputname, 'rb')
    ret_file = open(outputname, 'wb')
    obj = pylzma.decompressobj()

    while True:
        tmp = comp_file.read(8192)
        if not tmp: break
        ret_file.write(obj.decompress(tmp))
    ret_file.write(obj.flush())

    comp_file.close()
    ret_file.close()
    time_diff = str(datetime.now()-prev_time)
    return outputname, str(time_diff)


# create overlay VM using base VM
def create_overlay(base_image, base_mem, os_type):
    # generate overlay VM(disk + memory) from Base VM
    vm_name = os.path.basename(base_image).split('.')[0]
    vm_path = os.path.dirname(base_image)
    info_tag = '.overlay.' + str(VCPU_NUMBER) + 'cpu.' + str(VM_MEMORY) + "mem"
    overlay_disk = os.path.join(os.getcwd(), vm_name) + info_tag +  '.qcow2'
    overlay_mem = os.path.join(os.getcwd(), vm_name) + info_tag + '.mem'
    tmp_disk = os.path.join(vm_path, vm_name) + '_tmp.qcow2'
    tmp_mem = os.path.join(vm_path, vm_name) + '_tmp.mem'
    #command_str = 'cp ' + base_image + ' ' + tmp_disk
    command_str = 'qemu-img create -f qcow2 -b ' + base_image + ' ' + tmp_disk
    ret = commands.getoutput(command_str)

    telnet_port = 19823; vnc_port = 2

    # Run VM
    argument = []
    if base_mem == None:
        # create overlay only for disk image
        if os.path.exists(OVF_TRANSPORTER):
            run_image(tmp_disk, telnet_port, vnc_port, wait_vnc_end=True, cdrom=OVF_TRANSPORTER, os_type=os_type)
        else:
            print >> sys.stderr, "Warning, Make sure that you have OVF transport at %s\n" % (OVF_TRANSPORTER)
            run_image(tmp_disk, telnet_port, vnc_port, wait_vnc_end=True, cdrom=None, os_type=os_type)
        # terminal VM
        terminate_vm(telnet_port)
        argument.append((base_image, tmp_disk, overlay_disk))
    else:
        # create overlay only for disk and memory
        run_snapshot(tmp_disk, base_mem, telnet_port, vnc_port, wait_vnc_end=True, os_type=os_type)
        message = "----------\nBase Disk: %s\nBase Memory: %s\nOverlay Disk: %s\nOverlay Memory: %s\n\n" % \
                (base_image, base_mem, tmp_disk, tmp_mem)
        message += "You can review your VM by\ngvncviewer localhost:%d\n----------\n" % \
                (vnc_port)
        raw_input("%s\nEnter if you're ready to create overlay\n" % message)

        # stop and migrate to disk
        print "[INFO] migrating memory snapshot to disk"
        run_migration(telnet_port, vnc_port, tmp_mem)
        if os.path.exists(tmp_mem) == False:
            print >> sys.stderr, '[ERROR] new memory snapshot (%s) is not exit' % tmp_mem
            if os.path.exists(tmp_mem):
                os.remove(tmp_mem)
            if os.path.exists(tmp_disk):
                os.remove(tmp_disk)
            return []
        argument.append((base_image, tmp_disk, overlay_disk))
        argument.append((base_mem, tmp_mem, overlay_mem))

    ret_files = run_delta_compress(argument)
    return ret_files


def run_delta_compress(overlay_list):
    # xdelta and compression
    ret_files = []
    for (base, tmp, overlay) in overlay_list:
        prev_time = datetime.now()

        # xdelta
        ret = diff_files(base, tmp, overlay)
        print '[TIME] time for creating overlay : ', str(datetime.now()-prev_time)
        print '[INFO] (%d)-(%d)=(%d): ' % (os.path.getsize(base), os.path.getsize(tmp), os.path.getsize(overlay))
        if ret == None:
            print >> sys.stderr, '[ERROR] cannot create overlay ' + str(overlay)
            if os.path.exists(tmp):
                os.remove(tmp)
            return []
        
        # compression
        comp= overlay + '.lzma'
        comp, time1 = comp_lzma(overlay, comp)
        ret_files.append(comp)

        # remove temporary files
        #os.remove(tmp)
        #os.remove(overlay)

    return ret_files


# generate launch VM from compressed overlay VM
def recover_snapshot(base_img, base_mem, comp_img, comp_mem):
    recover=[]
    if base_mem == None:
        recover.append((base_img, comp_img))
    else:
        recover.append((base_img, comp_img))
        recover.append((base_mem, comp_mem))

    recover_files = []
    for (base, comp) in recover:
        # decompress
        overlay = comp + '.decomp'
        prev_time = datetime.now()
        decomp_lzma(comp, overlay)
        print '[Time] Decompression(%s) - %s' % (comp, str(datetime.now()-prev_time))

        # merge with base image
        recover = os.path.join(os.path.dirname(base), os.path.basename(comp) + '.recover'); 
        if os.path.exists(recover):
            os.remove(recover)

        prev_time = datetime.now()
        merge_file(base, overlay, recover)
        print '[Time] Recover(xdelta) image(%s) - %s' %(recover, str(datetime.now()-prev_time))

        os.remove(overlay)
        recover_files.append(recover)

    return recover_files


# wait until qemu telnet connection is established
def telnet_connection_waiting(telnet_port):
    # waiting for valid connection
    is_connected = False
    start_time = datetime.now()
    for i in xrange(200):
        try:
            tn = telnetlib.Telnet('localhost', telnet_port, 0.1)
            ret = tn.read_until("(qemu)", 0.1)
            if ret.find("(qemu)") != -1:
                is_connected = True
                tn.close()
                break;
        except EOFError:
            pass
        except socket.timeout:
            pass
        tn.close()

    if is_connected:
        count = 0
        while True:
            try:
                tn = telnetlib.Telnet('localhost', telnet_port, 0.1)
                ret = tn.read_until("(qemu)", 0.1)
                if ret.find("(qemu)") != -1:
                    tn.write('info status\n')
                    ret = tn.read_until("(qemu)", 1)
                    #print "request ret : %s, %s" % (ret, datetime.now())
                    if ret.find("running") != -1:
                        #print "info status time: ", str(datetime.now()-start_time)
                        tn.close()
                        return True
            except socket.timeout:
                #print "Connection timeout error"
                pass
            tn.close()
            count += 1
            if count%100 == 0:
                print "Waiting for connection to KVM controller"

    print "Error, No connection to KVM" 
    return False


def run_snapshot(disk_image, memory_image, telnet_port, vnc_port, wait_vnc_end=True, terminal_mode=False, os_type='window'):
    vm_path = os.path.dirname(memory_image)
    vnc_file = os.path.join(vm_path, 'kvm.vnc')
    
    # run kvm
    command_str = "kvm -hda "
    command_str += disk_image

    # virtio
    if os_type == 'linux':
        command_str += " -net nic,model=virtio"
    else:
        command_str += " -net nic"

    if telnet_port != 0 and vnc_port != -1:
        command_str += " -m " + str(VM_MEMORY) + " -monitor telnet:localhost:" + str(telnet_port) + ",server,nowait -enable-kvm -net user -serial none -parallel none -usb -usbdevice tablet " + PORT_FORWARDING
        command_str += " -vnc :" + str(vnc_port)
        command_str += " -smp " + str(VCPU_NUMBER)
        command_str += " -balloon virtio"
    else:
        command_str += " -m " + str(VM_MEMORY) + " -enable-kvm -net user -serial none -parallel none -usb -usbdevice tablet -redir tcp:2222::22"
    command_str += " -incoming \"exec:cat " + memory_image + "\""


    # parameter for AMI Image
    ovftransporter = os.path.join(os.path.dirname(disk_image), "ami.iso")
    if os.path.exists(ovftransporter):
        command_str += " -cdrom " + str(os.path.abspath(ovftransporter))

    print '[INFO] Run snapshot..'
    print command_str
    subprocess.Popen(command_str, shell=True)
    start_time = datetime.now()
    
    # waiting for TCP socket open
    loop_counter = 0
    while True:
        command_net = "netstat -an | grep 127.0.0.1:" + str(telnet_port)
        proc = subprocess.Popen(command_net, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        output = proc.stdout.readline()
        if output.find("LISTEN") != -1:
            break;
        time.sleep(0.1)
        loop_counter += 1
        if loop_counter%100 == 0:
            print "Waiting for KVM Start"

    # Getting VM Status information through Telnet
    ret = telnet_connection_waiting(telnet_port)
    end_time = datetime.now()

    if ret:
        if terminal_mode:
            print "[Terminal Mode] Do not show Guest OS"
            return str(end_time-start_time)

        # Run VNC
        # vnc_process = subprocess.Popen(VNC_VIEWER + " " + vnc_file, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        vnc_process = subprocess.Popen("gvncviewer localhost:" + str(vnc_port), shell=True, stdin=None, stdout=None, stderr=None)
        if wait_vnc_end:
            ret = vnc_process.wait()

        return str(end_time-start_time)
    else:
        return 0


def terminate_vm(telnet_port):
    try:
        tn = telnetlib.Telnet('localhost', telnet_port)
        tn.read_until("(qemu)", 10)
        # Stop running VM
        tn.write("stop\n")
        tn.read_until("(qemu)", 10)
        tn.write("quit\n")
        time.sleep(1)
    except EOFError:
        print "VM is already terminated"
    except socket.error:
        print "VM is already terminated"


# execute file migration command using telnet qemu command
def run_migration(telnet_port, vnc_port, mig_path):
    # save Memory State
    migration_cmd = "migrate \"exec:dd bs=1M 2> /dev/null | dd bs=1M of=" + mig_path +" 2> /dev/null\"\n"

    tn = telnetlib.Telnet('localhost', telnet_port)
    tn.read_until("(qemu)", 10)

    # Stop running VM
    tn.write("stop\n")
    for i in xrange(20):
        try:
            ret = tn.read_until("(qemu)", 10)
            if ret.find("(qemu)") != -1:
                break;
        except socket.timeout:
            pass
        time.sleep(1)

    # Set unlimited bandwidth for local migration
    tn.write("migrate_set_speed 10g\n")
    for i in xrange(20):
        try:
            ret = tn.read_until("(qemu)", 10)
            if ret.find("(qemu)") != -1:
                break;
        except socket.timeout:
            pass
        time.sleep(1)
    # Do migration to the disk file
    tn.write(migration_cmd)
    for i in xrange(20):
        try:
            ret = tn.read_until("(qemu)", 10)
            if ret.find("(qemu)") != -1:
                break;
        except socket.timeout:
            pass
        time.sleep(1)

    tn.write("quit\n")
    tn.close()


def run_ballooning(telnet_port, target_mem_size):
    # original mem size
    tn = telnetlib.Telnet('localhost', telnet_port)
    tn.read_until("(qemu)", 10)
    mem_info_cmd = "info balloon\n"
    tn.write(mem_info_cmd)
    ret = tn.read_until("(qemu)", 10)
    ret = ret.split('\n')[1]
    if not len(ret.split("actual=")) == 2:
        return False;

    original_mem_size = int(ret.split("actual=")[1].strip())
    tn.close()

    ret = set_balloon_size(telnet_port, target_mem_size)
    if ret:
        ret = set_balloon_size(telnet_port, original_mem_size)
        return ret
    else:
        return False


def set_balloon_size(telnet_port, target_mem_size):
    start_time = datetime.now()
    tn = telnetlib.Telnet('localhost', telnet_port)
    tn.read_until("(qemu)", 10)

    # ballooning to target size
    balloon_cmd = "balloon " + str(target_mem_size) + "\n"
    tn.write(balloon_cmd)
    # print "writing ballon command : " + str(datetime.now())
    tn.read_until("(qemu)", 20)
    # print "returned : " + str(datetime.now())

    for i in xrange(300):
        try:
            print "waiting for balloon memory size to %s" % (target_mem_size)
            tn.write('info balloon\n')
            ret = tn.read_until("(qemu)", 1)
            # print "request ret : %s, %s\n" % (ret, datetime.now())
            if ret.find(str(target_mem_size)) != -1:
                print "success to balloon %s(MB) at %s" % (target_mem_size, str(datetime.now()-start_time))
                tn.close()
                return True
        except socket.timeout:
            pass
        time.sleep(1)

    tn.close()
    return False


#stop VM using telnet qemu port
def stop_vm(telnet_port):
    tn = telnetlib.Telnet('localhost', telnet_port)
    tn.write("stop\n")
    ret = tn.read_until("(qemu)", 10)
    tn.write("quit\n")
    tn.read_until("(qemu)", 10)
    tn.close()


def create_base(imagefile, os_type=None, **kwargs):
    if os.path.exists(imagefile) == False:
        print >> sys.stderr, '[ERROR] %s is not exist' % imagefile
        return None

    vm_name = os.path.basename(imagefile).split('.')[0]
    vm_path = os.path.dirname(imagefile)
    base_image = os.path.join(vm_path, vm_name) + '.base.img'

    # check existing file first
    if os.path.exists(base_image):
        message = "(%s) is exist. Are you sure to overwrite?(y/N) " % (base_image)
        ret = raw_input(message)
        if str(ret).lower() != 'y':
            sys.exit(1)

    #command_str = 'qemu-img create -f qcow2 -b ' + imagefile + ' ' + base_image
    command_str = 'cp ' + imagefile + ' ' + base_image
    ret = commands.getoutput(command_str)
    print '[INFO] run Base Image to generate memory snapshot'
    telnet_port = 12123; vnc_port = 3

    run_image(base_image, telnet_port, vnc_port, os_type=os_type, terminal_mode=kwargs.get("terminal_mode", False))
    base_mem = os.path.join(vm_path, vm_name) + '.base.mem'

    # stop and migrate
    run_migration(telnet_port, vnc_port, base_mem)
    if os.path.exists(base_mem) == False:
        print >> sys.stderr, '[ERROR] base memory snapshot (%s) is not exit' % base_mem
        return None, None

    return base_image, base_mem


def run_image(disk_image, telnet_port, vnc_port, wait_vnc_end=True, cdrom=None, terminal_mode=False, os_type=None):
    command_str = "kvm -hda "
    command_str += disk_image

    # Virtio Support
    if os_type and os_type.lower() == 'linux': 
        command_str += " -net nic,model=virtio"
    else:
        command_str += " -net nic"

    if telnet_port != 0 and vnc_port != -1:
        command_str += " -m " + str(VM_MEMORY) + " -monitor telnet:localhost:" + str(telnet_port) + ",server,nowait -enable-kvm -net user -serial none -parallel none -usb -usbdevice tablet " + PORT_FORWARDING
        command_str += " -vnc :" + str(vnc_port)
        command_str += " -smp " + str(VCPU_NUMBER)
        command_str += " -balloon virtio"
    else:
        command_str += " -m " + str(VM_MEMORY) + " -enable-kvm -net user -serial none -parallel none -usb -usbdevice tablet -redir tcp:2222::22"

    # parameter for AMI Image
    if cdrom != None:
        command_str += " -cdrom " + str(os.path.abspath(cdrom))

    print '[DEBUG] command : ' + command_str
    subprocess.Popen(command_str, shell=True)
    time.sleep(3)

    if terminal_mode:
        return

    # Run VNC and wait until user finishes working
    vnc_process = subprocess.Popen("gvncviewer localhost:" + str(vnc_port), shell=True)
    if wait_vnc_end:
        vnc_process.wait()


def print_usage(program_name):
    print 'usage: %s [option] [arg1[,arg2[, ...]]]..  ' % program_name
    print ' -h, --help  print help'
    print ' -b, --base [window|linux] [disk image]' + '\tcreate Base VM (image and memory)'
    print ' -o, --overlay [window|linux] [base image [,base mem]]' + '\n\tcreate overlay from base image. Overlay for EC2 will only generate disk image, so you do not need base memory'
    print ' -r, --run [window|linux] [base image[,base memory]] [overlay image[,overlay memory]] [telnet_port] [vnc_port]' + '\n\trun overlay image. Ovelay for EC2 start from booting, so you do not need memory information'
    print ' -s, --stop [command_port]' + '\tstop VM using qemu telnet monitor'


def validate_congifuration():
    cmd = "kvm --version"
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    out, err = proc.communicate()
    if len(err) > 0:
        print "KVM validation Error: %s" % (err)
        return False
    if out.find("Cloudlet") > 0:
        print "KVM validation Error, Incorrect Version:\n%s" % (out)
        return False
    if out.find('1.1.1') < 0:
        print "Invalid KVM version. Use 1.1.1:\n%s" % (out)
        return False
    return True

def main(argv):
    if not validate_congifuration():
        sys.stderr.write("failed to validate configuration\n")
        sys.exit(1)

    if len(argv) < 2:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)
    try:
        optlist, args = getopt.getopt(argv[1:], 'hbors', ["help", "base", "overlay", "run", "stop"])
    except getopt.GetoptError, err:
        print str(err)
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    # parse argument
    o = optlist[0][0]
    if o in ("-h", "--help"):
        print_usage(os.path.basename(argv[0]))
    elif o in ("-b", "--base"):
        if len(args) != 2:
            print_usage(os.path.basename(argv[0]))
            print 'invalid argument'
            return;
        os_type = args[0]
        input_image_path = os.path.abspath(args[1])
        base_image, base_mem = create_base(input_image_path, os_type=os_type)
        print '[INFO] Base (%s, %s) is created from %s' % (base_image, base_mem, args[0])
    elif o in ("-o", "--overlay"):
        if len(args) == 2:
            # create overlay for EC2 (disk only)
            os_type = args[0]
            base_image = os.path.abspath(args[1])
            ret_files = create_overlay(base_image, None, os_type=os_type)
            print '[INFO] Overlay (%s) is created from %s' % (str(ret_files[0]), os.path.basename(base_image))
        elif len(args) == 3:
            # create overlay for mobile (disk and memory)
            os_type = args[0]
            base_image = os.path.abspath(args[1])
            base_mem = os.path.abspath(args[2])
            ret_files = create_overlay(base_image, base_mem, os_type=os_type)
            print '[INFO] Overlay (%s, %s) is created from %s' % (str(ret_files[0]), str(ret_files[1]), os.path.basename(base_image))
        else:
            print_usage(os.path.basename(argv[0]))
            print 'invalid argument'
            return;
    elif o in ("-r", "--run"):
        if len(args) == 5:
            # running VM from booting, (e.g. EC2 case)
            os_type = args[0]
            base_img = os.path.abspath(args[1]); 
            comp_img = os.path.abspath(args[2]); 
            telnet_port = int(args[3]); vnc_port = int(args[4])

            # recover image from overlay
            ret_files = recover_snapshot(base_img, None, comp_img, None)

            if os.path.exists(OVF_TRANSPORTER):
                run_image(ret_files[0], telnet_port, vnc_port, wait_vnc_end=False, cdrom=OVF_TRANSPORTER, os_type=os_type)
            else:
                run_image(ret_files[0], telnet_port, vnc_port, wait_vnc_end=False, os_type=os_type)
                print "Warning, you may require OVF transport at %s" % (OVF_TRANSPORTER)


            # run snapshot non-blocking mode
            print '[INFO] Launch overlay Disk image'
            return;
        if len(args) == 7:
            os_type = args[0]
            # running VM base on Memory Snapshot, cloudlet case
            base_img = os.path.abspath(args[1]); base_mem = os.path.abspath(args[2])
            comp_img = os.path.abspath(args[3]); comp_mem = os.path.abspath(args[4])
            telnet_port = int(args[5]); vnc_port = int(args[6])
            # recover image from overlay
            recover_img, recover_mem = recover_snapshot(base_img, base_mem, comp_img, comp_mem)
            # run snapshot non-blocking mode
            execution_time = run_snapshot(recover_img, recover_mem, telnet_port, vnc_port, wait_vnc_end=False, os_type=os_type)
            print '[Time] Run Snapshot - ', execution_time
            return;
        else:
            print_usage(os.path.basename(argv[0]))
            print 'invalid argument'
            return;
    elif o in ("-s", "--stop"):
        if len(args) != 1:
            print_usage(os.path.basename(argv[0]))
            print 'invalid argument'
            return;
        telnet_port = int(args[0])
        # stop and quit
        stop_vm(telnet_port)
    else:
        assert False, "unhandled option"

    if len(optlist) == 0:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
