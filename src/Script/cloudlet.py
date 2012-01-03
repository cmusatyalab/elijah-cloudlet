#!/usr/bin/env python
import xdelta3
import os
import commands
import filecmp
import sys
import subprocess
import getopt
import time
from datetime import datetime
import telnetlib
import socket
import pylzma

VM_MEMORY = "2048"
VNC_VIEWER = "/home/krha/Cloudlet/src/Script/vnc_viewer.py"
PORT_FORWARDING = "-redir tcp:9876::9876 -redir tcp:2222::22 -redir tcp:19092::9092 -redir tcp:6789::6789"


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
    command_patch = ['xdelta3', '-df', '-s', source_file, overlay_file, output_file]
    #print command_patch
    ret = xdelta3.xd3_main_cmdline(command_patch)
    if ret == 0:
        #print "output : %s (%d)" % (output_file, os.path.getsize(output_file))
        return output_file
    else:
        return None

def compare_same(filename1, filename2):
    print '[INFO] checking validity of generated file'
    compare = filecmp.cmp(filename1, filename2)
    if compare == False:
        print '[ERROR] %s != %s' % (os.path.basename(filename1), os.path.basename(filename2))
        return False
    else:
        print '[INFO] SUCCESS to recover'
        return True

# compression
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

#decompression
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

def create_overlay(base_image, base_mem):
    # generate overlay VM(disk + memory) from Base VM
    vm_name = os.path.basename(base_image).split('.')[0]
    vm_path = os.path.dirname(base_mem)
    overlay_disk = os.path.join(os.getcwd(), vm_name) + '_overlay.qcow2'
    overlay_mem = os.path.join(os.getcwd(), vm_name) + '_overlay.mem'
    tmp_disk = os.path.join(vm_path, vm_name) + '_tmp.qcow2'
    tmp_mem = os.path.join(vm_path, vm_name) + '_tmp.mem'
    command_str = 'cp ' + base_image + ' ' + tmp_disk
    ret = commands.getoutput(command_str)

    print '[INFO] run Base Image to generate memory snapshot'
    telnet_port = 19823; vnc_port = 2
    run_snapshot(tmp_disk, base_mem, telnet_port, vnc_port, wait_vnc_end=True)
    # stop and migrate
    run_migration(telnet_port, vnc_port, tmp_mem)

    if os.path.exists(tmp_mem) == False:
        print '[ERROR] new memory snapshot (%s) is not exit' % tmp_mem
        if os.path.exists(tmp_mem):
            os.remove(tmp_mem)
        if os.path.exists(tmp_disk):
            os.remove(tmp_disk)
        return None, None, None, None

    prev_time = datetime.now()
    ret = diff_files(base_image, tmp_disk, overlay_disk)
    print '[TIME] time for creating overlay disk : ', str(datetime.now()-prev_time)
    if ret == None:
        print '[ERROR] cannot create overlay disk'
        if os.path.exists(tmp_mem):
            os.remove(tmp_mem)
        if os.path.exists(tmp_disk):
            os.remove(tmp_disk)
        return None, None, None, None
    
    prev_time = datetime.now()
    ret = diff_files(base_mem, tmp_mem, overlay_mem)
    print '[TIME] time for creating overlay memory : ', str(datetime.now()-prev_time)
    if ret == None:
        print '[ERROR] cannot create overlay_mem'
        if os.path.exists(tmp_mem):
            os.remove(tmp_mem)
        if os.path.exists(tmp_disk):
            os.remove(tmp_disk)
        return None, None, None, None

    # compression
    comp_disk = overlay_disk + '.lzma'
    comp_mem = overlay_mem + '.lzma'
    comp_disk, time1 = comp_lzma(overlay_disk, comp_disk)
    comp_mem, time2 = comp_lzma(overlay_mem, comp_mem)

    os.remove(tmp_mem)
    os.remove(tmp_disk)
    #os.remove(overlay_disk)
    #os.remove(overlay_mem)
    return comp_disk, comp_mem

def recover_snapshot(base_img, base_mem, comp_img, comp_mem):
    # decompress
    overlay_img = comp_img + '.decomp'
    overlay_mem = comp_mem + '.decomp'
    prev_time = datetime.now()
    decomp_lzma(comp_img, overlay_img)
    decomp_lzma(comp_mem, overlay_mem)
    print '[Time] Decompression - ', str(datetime.now()-prev_time)

    # merge with base image
    recover_img = os.path.join(os.path.dirname(base_img), 'recover.qcow2'); 
    recover_mem = os.path.join(os.path.dirname(base_mem), 'recover.mem');
    for recover_file in (recover_img, recover_mem):
        if os.path.exists(recover_file):
            os.remove(recover_file)

    prev_time = datetime.now()
    merge_file(base_img, overlay_img, recover_img)
    merge_file(base_mem, overlay_mem, recover_mem)
    print '[Time] Recover(xdelta) image - ', str(datetime.now()-prev_time)

    os.remove(overlay_img)
    os.remove(overlay_mem)
    return recover_img, recover_mem


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

    #print "Socket connection time : ", str(datetime.now()-start_time)

    start_time = datetime.now()
    if is_connected:
        for i in xrange(200):
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
    print "No connection to KVM" 
    return False


def run_snapshot(disk_image, memory_image, telnet_port, vnc_port, wait_vnc_end):
    vm_path = os.path.dirname(memory_image)
    vnc_file = os.path.join(vm_path, 'kvm.vnc')

    # run kvm
    command_str = "kvm -hda "
    command_str += disk_image
    if telnet_port != 0 and vnc_port != -1:
        command_str += " -m " + VM_MEMORY + " -monitor telnet:localhost:" + str(telnet_port) + ",server,nowait -enable-kvm -net nic -net user -serial none -parallel none -usb -usbdevice tablet " + PORT_FORWARDING
        #command_str += " -vnc :" + str(vnc_port) + " "
        command_str += " -vnc unix:" + vnc_file
    else:
        command_str += " -m " + VM_MEMORY + " -enable-kvm -net nic -net user -serial none -parallel none -usb -usbdevice tablet -redir tcp:2222::22"
    command_str += " -incoming \"exec:cat " + memory_image + "\""
    # print '[INFO] Run snapshot..'
    print command_str
    subprocess.Popen(command_str, shell=True)
    start_time = datetime.now()
    
    # waiting for TCP socket open
    for i in xrange(200):
        command_str = "netstat -an | grep 127.0.0.1:" + str(telnet_port)
        proc = subprocess.Popen(command_str, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        output = proc.stdout.readline()
        if output.find("LISTEN") != -1:
            break;
        time.sleep(0.1)

    # Getting VM Status information through Telnet
    ret = telnet_connection_waiting(telnet_port)
    end_time = datetime.now()

    if ret:
        # Run VNC
        vnc_process = subprocess.Popen(VNC_VIEWER + " " + vnc_file, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        # vnc_process = subprocess.Popen("gvncviewer localhost:" + str(vnc_port), shell=True)
        if wait_vnc_end:
            ret = vnc_process.wait()

        return str(end_time-start_time)
    else:
        return 0

    '''
    vnc = VncViewer(vnc_file, fullscreen=False)
    vnc.main()
    print "VM Resume End : ", vnc.launch_time()
    print "[Time] KVM Resume Time : ", str(vnc.launch_time() - start_time)
    '''


def run_migration(telnet_port, vnc_port, mig_path):
    # save Memory State
    migration_cmd = "migrate \"exec:dd bs=1M 2> /dev/null | dd bs=1M of=" + mig_path +" 2> /dev/null\"\n"
    print 'debug ', migration_cmd

    tn = telnetlib.Telnet('localhost', telnet_port)
    tn.write("stop\n")
    ret = tn.read_until("(qemu)", 10)
    print 'waiting telnet ', ret
    tn.write(migration_cmd)
    while (1):
        time.sleep(1)
        tn_read = tn.read_some()
        if tn_read.find("(qemu)"):
            break;
    tn.write("quit\n")
    ret = tn.read_until("(qemu)", 10)
    tn.close()


#stop VM using telnet qemu port
def stop_vm(telnet_port):
    tn = telnetlib.Telnet('localhost', telnet_port)
    tn.write("stop\n")
    ret = tn.read_until("(qemu)", 10)
    tn.write("quit\n")
    ret = tn.read_until("(qemu)", 2)
    tn.close()


def create_base(imagefile):
    if os.path.exists(imagefile) == False:
        print '[ERROR] %s is not exist' % imagefile
        return None

    vm_name = os.path.basename(imagefile).split('.')[0]
    vm_path = os.path.dirname(imagefile)
    base_image = os.path.join(vm_path, vm_name) + '_base.qcow2'
    command_str = 'qemu-img create -f qcow2 -b ' + imagefile + ' ' + base_image
    ret = commands.getoutput(command_str)
    print '[INFO] run Base Image to generate memory snapshot'
    telnet_port = 12123; vnc_port = 3
    run_image(base_image, telnet_port, vnc_port)

    # stop and migrate
    base_mem = os.path.join(vm_path, vm_name) + '_base.mem'
    run_migration(telnet_port, vnc_port, base_mem)
    if os.path.exists(base_mem) == False:
        print '[ERROR] base memory snapshot (%s) is not exit' % base_mem
        return None, None

    return base_image, base_mem

def run_image(disk_image, telnet_port, vnc_port):
    command_str = "kvm -hda "
    command_str += disk_image
    if telnet_port != 0 and vnc_port != -1:
        command_str += " -m " + VM_MEMORY + " -monitor telnet:localhost:" + str(telnet_port) + ",server,nowait -enable-kvm -net nic -net user -serial none -parallel none -usb -usbdevice tablet -redir tcp:9876::9876"
        command_str += " -vnc :" + str(vnc_port) + " "
    else:
        command_str += " -m " + VM_MEMORY + " -enable-kvm -net nic -net user -serial none -parallel none -usb -usbdevice tablet -redir tcp:2222::22"
    print '[DEBUG] command : ' + command_str
    process = subprocess.Popen(command_str, shell=True)

    # Run VNC and wait until user finishes working
    time.sleep(3)
    vnc_process = subprocess.Popen("gvncviewer localhost:" + str(vnc_port), shell=True)
    #vnc_process = subprocess.Popen(VNC_VIEWER + " " + vnc_file, shell=True, stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    ret = vnc_process.wait()


def print_usage(program_name):
    print 'usage: %s [option] [file]..  ' % program_name
    print ' -h, --help  print help'
    print ' -b, --base [disk image]' + '\tcreate Base VM (image and memory)'
    print ' -o, --overlay [base image] [base mem]' + '\tcreate overlay from base image'
    print ' -r, --run [base image] [base memory] [overlay image] [overlay memory] [telnet_port] [vnc_port]' + '\trun overlay image'
    print ' -s, --stop [command_port]' + '\tstop VM using qemu telnet monitor'


def main(argv):
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
        if len(args) != 1:
            print_usage(os.path.basename(argv[0]))
            print 'invalid argument'
            return;
        base_image, base_mem = create_base(args[0])
        print '[INFO] Base (%s, %s) is created from %s' % (base_image, base_mem, args[0])
    elif o in ("-o", "--overlay"):
        if len(args) != 2:
            print_usage(os.path.basename(argv[0]))
            print 'invalid argument'
            return;
        base_image = args[0]
        base_mem = args[1]
        # create overlay
        overlay_disk, overlay_mem = create_overlay(base_image, base_mem)
        print '[INFO] Overlay (%s, %s) is created from %s' % (overlay_disk, overlay_mem, os.path.basename(base_image))
    elif o in ("-r", "--run"):
        if len(args) != 6:
            print_usage(os.path.basename(argv[0]))
            print 'invalid argument'
            return;
        base_img = args[0]; base_mem = args[1]; comp_img = args[2]; comp_mem = args[3]
        telnet_port = int(args[4]); vnc_port = int(args[5])
        # recover image from overlay
        recover_img, recover_mem = recover_snapshot(base_img, base_mem, comp_img, comp_mem)
        # run snapshot non-blocking mode
        execution_time = run_snapshot(recover_img, recover_mem, telnet_port, vnc_port, wait_vnc_end=False)
        print '[Time] Run Snapshot - ', execution_time
        sys.exit(0)
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
    main(sys.argv)
