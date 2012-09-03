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

import libvirt
import sys
import os
import subprocess
import KVMMemory
import Disk
import vmnetfs
import vmnetx
import stat
from xml.etree import ElementTree
from uuid import uuid4
from tempfile import NamedTemporaryFile
from time import time
from time import sleep
from optparse import OptionParser

from tool import comp_lzma
from tool import decomp_lzma
from tool import diff_files
from tool import merge_files

class Log(object):
    out = sys.stdout
    mute = open("/dev/null", "w+b")

class Const(object):
    BASE_DISK           = ".base-img"
    BASE_MEM            = ".base-mem"
    BASE_DISK_META      = ".base-img-meta"
    BASE_MEM_META       = ".base-mem-meta"
    BASE_MEM_RAW        = ".base-mem.raw"
    OVERLAY_DISK        = ".overlay-img"
    OVERLAY_DISKMETA    = ".overlay-img-meta"
    OVERLAY_MEM         = ".overlay-mem"

    TEMPLATE_XML = "./config/VM_TEMPLATE.xml"
    VMNETFS_PATH = "/home/krha/cloudlet/src/vmnetx/vmnetfs/vmnetfs"
    CHUNK_SIZE=4096

    @staticmethod
    def get_basepath(base_disk_path, check_exist=False):
        def _check_path(name, path):
            if not os.path.exists(path):
                message = "Cannot find name at %s" % (path)
                raise CloudletGenerationError(message)
        _check_path('base disk', base_disk_path)

        image_name = os.path.splitext(base_disk_path)[0]
        dir_path = os.path.dirname(base_disk_path)
        diskmeta = os.path.join(dir_path, image_name+Const.BASE_DISK_META)
        mempath = os.path.join(dir_path, image_name+Const.BASE_MEM)
        memmeta = os.path.join(dir_path, image_name+Const.BASE_MEM_META)
        memraw = os.path.join(dir_path, image_name+Const.BASE_MEM_RAW)

        #check sanity
        if check_exist==True:
            _check_path('base memory', mempath)
            _check_path('base disk-hash', diskmeta)
            _check_path('base memory-hash', memmeta)
            _check_path('base memory-raw', memraw)

        return diskmeta, mempath, memmeta, memraw


class CloudletGenerationError(Exception):
    pass


def copy_disk(in_path, out_path):
    print "[INFO] Copying disk image to %s" % out_path
    cmd = "cp %s %s" % (in_path, out_path)
    cp_proc = subprocess.Popen(cmd, shell=True)
    cp_proc.wait()
    if cp_proc.returncode != 0:
        raise IOError("Copy failed: from %s to %s " % (in_path, out_path))


def convert_xml(xml, conn, vm_name=None, disk_path=None, uuid=None):
    if vm_name:
        name_element = xml.find('name')
        if not name_element:
            raise CloudletGenerationError("Malfomed XML input: %s", Const.TEMPLATE_XML)
        name_element.text = vm_name

    if uuid:
        uuid_element = xml.find('uuid')
        uuid_element.text = str(uuid)

    # disk path is required
    if not disk_path:
        raise CloudletGenerationError("Need disk_path to run new VM")
    disk_element = xml.find('devices/disk/source')
    if disk_element == None:
        raise CloudletGenerationError("Malfomed XML input: %s", Const.TEMPLATE_XML)

    disk_element.set("file", os.path.abspath(disk_path))
    return ElementTree.tostring(xml)


def get_libvirt_connection():
    conn = libvirt.open("qemu:///session")
    return conn


def create_baseVM(disk_image_path):
    # Create Base VM(disk, memory) snapshot using given VM disk image
    # :param disk_image_path : file path of the VM disk image
    # :returns: (generated base VM disk path, generated base VM memory path)

    (base_diskmeta, base_mempath, base_memmeta, base_memraw) = \
            Const.get_basepath(disk_image_path)

    # check sanity
    if not os.path.exists(Const.TEMPLATE_XML):
        raise CloudletGenerationError("Cannot find Base VM default XML at %s\n" \
                % Const.TEMPLATE_XML)
    if os.path.exists(base_mempath):
        warning_msg = "Warning: (%s) exist.\nAre you sure to overwrite? (y/N) " \
                % (base_mempath)
        ret = raw_input(warning_msg)
        if str(ret).lower() != 'y':
            sys.exit(1)

    # allow write permission to base disk and delete all previous files
    os.chmod(disk_image_path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)
    if os.path.exists(base_diskmeta):
        os.unlink(base_diskmeta)
    if os.path.exists(base_mempath):
        os.unlink(base_mempath)
    if os.path.exists(base_memmeta):
        os.unlink(base_memmeta)
    if os.path.exists(base_memraw):
        os.unlink(base_memraw)

    # edit default XML to use give disk image
    conn = get_libvirt_connection()
    xml = ElementTree.fromstring(open(Const.TEMPLATE_XML, "r").read())
    new_xml_string = convert_xml(xml, conn, disk_path=disk_image_path, uuid=str(uuid4()))

    # launch VM & wait for end of vnc
    try:
        machine = run_vm(conn, new_xml_string, wait_vnc=True)

        # make memory snapshot
        # VM has to be paused first to perform stable disk hashing
        save_mem_snapshot(machine, base_mempath)
        base_mem = KVMMemory.hashing(base_mempath, base_memraw)
        base_mem.export_to_file(base_memmeta)

        # generate disk hashing
        # TODO: need more efficient implementation, e.g. bisect
        Disk.hashing(disk_image_path, base_diskmeta, print_out=Log.out)
    except Exception as e:
        sys.stderr.write(str(e))
        if machine:
            machine.destroy()

    # write protection
    os.chmod(disk_image_path, stat.S_IRUSR)
    os.chmod(base_diskmeta, stat.S_IRUSR)
    os.chmod(base_mempath, stat.S_IRUSR)
    os.chmod(base_memmeta, stat.S_IRUSR)
    os.chmod(base_memraw, stat.S_IRUSR)
    return disk_image_path, base_mempath


def create_overlay(base_image):
    # create user customized overlay.
    # First resume VM, then let user edit its VM
    # Finally, return disk/memory binary as an overlay
    # base_image: path to base disk

    (base_diskmeta, base_mem, base_memmeta, base_memraw) = \
            Const.get_basepath(base_image, check_exist=True)
    
    # filename for overlay VM
    image_name = os.path.basename(base_image).split(".")[0]
    dir_path = os.path.dirname(base_mem)
    overlay_diskpath = os.path.join(dir_path, image_name+Const.OVERLAY_DISK)
    overlay_diskmeta = os.path.join(dir_path, image_name+Const.OVERLAY_DISKMETA)
    overlay_mempath = os.path.join(dir_path, image_name+Const.OVERLAY_MEM)
    
    # make FUSE disk & memory
    fuse = run_fuse(Const.VMNETFS_PATH, base_image, Const.CHUNK_SIZE,
            resumed_disk=None, overlay_map=None)
    modified_disk = os.path.join(fuse.mountpoint, 'disk', 'image')
    modified_mem = NamedTemporaryFile(prefix="cloudlet-mem-")
    # monitor modified chunks
    stream_modified = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_modified')
    stream_access = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_accessed')
    monitor = vmnetfs.StreamMonitor()
    monitor.add_path(stream_modified)
    monitor.add_path(stream_access)
    monitor.start()

    # 1-1. resume & get modified disk
    conn = get_libvirt_connection()
    machine = run_snapshot(conn, modified_disk, base_mem, wait_vnc=True)
    # 1-2. get modified memory
    # TODO: support stream of modified memory rather than tmp file
    save_mem_snapshot(machine, modified_mem.name)

    # 2-1. get memory overlay
    KVMMemory.create_memory_overlay(base_memmeta, base_memraw, \
            modified_mem.name, overlay_mempath, print_out=Log.out)

    # 2-2. get disk overlay
    m_chunk_list = monitor.chunk_list
    m_chunk_list.sort()
    packed_chunk_list = dict((x,x) for x in m_chunk_list).values()
    Disk.create_disk_overlay(overlay_diskpath, overlay_diskmeta, \
            modified_disk, packed_chunk_list, Const.CHUNK_SIZE, print_out=Log.out)

    # 3. terminting
    monitor.terminate()
    monitor.join()
    return (overlay_diskmeta, overlay_diskpath, overlay_mempath)
    '''
    output_list = []
    output_list.append((base_image, modified_disk, overlay_diskpath))
    output_list.append((base_mem, modified_mem, overlay_mempath))

    ret_files = run_delta_compression(output_list)
    overlay_files = []
    overlay_files.append(meta_file_path)
    overlay_files.append(ret_files[0])
    overlay_files.append(ret_files[1])
    return overlay_files
    '''

def run_delta_compression(output_list, **kwargs):
    # kwargs
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    # custom_delta
    log = kwargs.get('log', None)
    nova_util = kwargs.get('nova_util', None)
    custom_delta = kwargs.get('custom_delta', False)

    # xdelta and compression
    ret_files = []
    for (base, modified, overlay) in output_list:
        start_time = time()

        # xdelta
        if custom_delta:
            diff_files(base, modified, overlay, nova_util=nova_util)
        else:
            diff_files(base, modified, overlay, nova_util=nova_util)
        print '[TIME] time for creating overlay : ', str(time()-start_time)
        print '[INFO] (%d)-(%d)=(%d): ' % (os.path.getsize(base), os.path.getsize(modified), os.path.getsize(overlay))
        
        # compression
        comp = overlay + '.lzma'
        comp, time1 = comp_lzma(overlay, comp, nova_util=nova_util)
        ret_files.append(comp)

        # remove temporary files
        os.remove(modified)
        os.remove(overlay)

    return ret_files


def recover_launchVM(base_image, overlay_meta, overlay_disk, overlay_mem, **kwargs):
    # kwargs
    # skip_validation   :   skip sha1 validation
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    log = kwargs.get('log', None)
    nova_util = kwargs.get('nova_util', None)

    (base_diskmeta, base_mem, base_memmeta, base_memraw) = \
            Const.get_basepath(base_image, check_exist=True)
    modified_mem = NamedTemporaryFile(prefix="cloudlet-recoverd-mem-", delete=False)
    modified_img = NamedTemporaryFile(prefix="cloudlet-recoverd-img-", delete=False)
    print modified_mem.name
    print modified_img.name

    # Recover Modified Memory
    KVMMemory.recover_memory(base_mem, overlay_mem, base_memmeta, 
            base_memraw, modified_mem.name)

    # Recover Modified Disk
    overlay_map = Disk.recover_disk(overlay_disk, overlay_meta, 
            modified_img.name, Const.CHUNK_SIZE)

    # make FUSE disk & memory
    fuse = run_fuse(Const.VMNETFS_PATH, base_image, Const.CHUNK_SIZE,
            resumed_disk=modified_img.name, overlay_map=overlay_map)

    return [modified_img.name, modified_mem.name, fuse]


def run_fuse(bin_path, original_disk, chunk_size, resumed_disk=None, overlay_map=None):
    # launch fuse
    execute_args = ['', '', 'http://cloudlet.krha.kr/ubuntu-11/', \
            "%s" % (os.path.abspath(original_disk)), \
            ("%s" % os.path.abspath(resumed_disk)) if resumed_disk else "", \
            ("%s" % overlay_map) if overlay_map else "", \
            '%d' % os.path.getsize(original_disk), \
            '0',\
            "%d" % chunk_size]
    fuse_process = vmnetfs.VMNetFS(bin_path, execute_args)
    fuse_process.start()
    return fuse_process


def run_vm(conn, domain_xml, **kwargs):
    # kwargs
    # vnc_disable       :   do not show vnc console
    # wait_vnc          :   wait until vnc finishes if vnc_enabled
    machine = conn.createXML(domain_xml, 0)

    # Run VNC and wait until user finishes working
    if kwargs.get('vnc_disable'):
        return machine

    # Get VNC port
    vnc_port = 5900
    try:
        running_xml_string = machine.XMLDesc(libvirt.VIR_DOMAIN_XML_SECURE)
        running_xml = ElementTree.fromstring(running_xml_string)
        vnc_port = running_xml.find("devices/graphics").get("port")
        vnc_port = int(vnc_port)-5900
    except AttributeError as e:
        sys.stderr.write("Warning, Possible VNC port error:%s\n" % str(e))

    _PIPE = subprocess.PIPE
    vnc_process = subprocess.Popen("gvncviewer localhost:%d" % vnc_port, 
            shell=True, stdin=_PIPE, stdout=_PIPE)
    if kwargs.get('wait_vnc'):
        try:
            vnc_process.wait()
        except KeyboardInterrupt as e:
            print "[INFO] keyboard interrupt while waiting VNC"
            if machine:
                machine.destroy()
    return machine


def save_mem_snapshot(machine, fout_path, **kwargs):
    #kwargs
    #LOG = log object for nova
    #nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    log = kwargs.get('log', None)
    nova_util = kwargs.get('nova_util', None)

    #Set migration speed
    ret = machine.migrateSetMaxSpeed(1000000, 0)   # 1000 Gbps, unlimited
    if ret != 0:
        raise CloudletGenerationError("Cannot set migration speed : %s", machine.name())

    #Pause VM
    ret = machine.suspend()
    if ret != 0:
        raise CloudletGenerationError("Cannot pause VM : %s", machine.name())

    #Save memory state
    print "[INFO] save VM memory state at %s" % fout_path
    try:
        ret = machine.save(fout_path)
    except libvirt.libvirtError, e:
        raise CloudletGenerationError(str(e))
    if ret != 0:
        raise CloudletGenerationError("libvirt: Cannot save memory state")


def run_snapshot(conn, disk_image, mem_snapshot, **kwargs):
    # kwargs
    # vnc_disable       :   show vnc console
    # wait_vnc          :   wait until vnc finishes if vnc_enabled

    # read embedded XML at memory snapshot to change disk path
    hdr = vmnetx._QemuMemoryHeader(open(mem_snapshot))
    xml = ElementTree.fromstring(hdr.xml)
    new_xml_string = convert_xml(xml, conn, disk_path=disk_image, uuid=uuid4())
    temp_mem = NamedTemporaryFile(prefix="cloudlet-mem-")
    copy_with_xml(mem_snapshot, temp_mem.name, new_xml_string)

    # resume
    restore_with_config(conn, temp_mem.name, new_xml_string)

    # get machine
    domxml = ElementTree.fromstring(new_xml_string)
    uuid_element = domxml.find('uuid')
    uuid = str(uuid_element.text)
    machine = conn.lookupByUUIDString(uuid)
    if kwargs.get('vnc_disable'):
        return machine

    # Get VNC port
    vnc_port = 5900
    try:
        running_xml_string = machine.XMLDesc(libvirt.VIR_DOMAIN_XML_SECURE)
        running_xml = ElementTree.fromstring(running_xml_string)
        vnc_port = running_xml.find("devices/graphics").get("port")
        vnc_port = int(vnc_port)-5900
    except AttributeError as e:
        sys.stderr.write("Warning, Possible VNC port error:%s\n" % str(e))

    # Run VNC
    vnc_process = subprocess.Popen("gvncviewer localhost:%d" % vnc_port, shell=True)
    if kwargs.get('wait_vnc'):
        print "[INFO] waiting for finishing VNC interaction"
        try:
            vnc_process.wait()
        except KeyboardInterrupt as e:
            print "keyboard interrupt while waiting VNC"
            vnc_process.terminate()
    return machine


def rettach_nic(conn, xml, **kwargs):
    #kwargs
    #LOG = log object for nova
    #nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    log = kwargs.get('log', None)

    # get machine
    domxml = ElementTree.fromstring(xml)
    uuid = domxml.find('uuid').text
    machine = conn.lookupByUUIDString(uuid)

    # get xml info of running xml
    running_xml = machine.XMLDesc(libvirt.VIR_DOMAIN_XML_SECURE)
    machinexml = ElementTree.fromstring(running_xml)
    nic = machinexml.find('devices/interface')
    nic_xml = ElementTree.tostring(nic)
    
    if log:
        log.debug(_("Rettaching device : %s" % str(nic_xml)))
        log.debug(_("memory xml"))
        log.debug(_("%s" % xml))
        log.debug(_("running xml"))
        log.debug(_("%s" % running_xml))
    else:
        print "[Debug] Rettaching device : %s" % str(nic_xml)

    #detach
    machine.detachDevice(nic_xml)
    sleep(3)
    if log:
        log.debug(_("dettached xml"))
        dettached_xml = machine.XMLDesc(libvirt.VIR_DOMAIN_XML_SECURE)
        log.debug(_("%s" % str(dettached_xml)))

    #attach
    machine.attachDevice(nic_xml)
    if log:
        log.debug(_("rettached xml"))
        rettached_xml = machine.XMLDesc(libvirt.VIR_DOMAIN_XML_SECURE)
        log.debug(_("%s" % str(rettached_xml)))


def restore_with_config(conn, mem_snapshot, xml):
    print "[INFO] restoring VM..."
    try:
        conn.restoreFlags(mem_snapshot, xml, libvirt.VIR_DOMAIN_SAVE_RUNNING)
    except libvirt.libvirtError, e:
        message = "%s\nXML: %s" % (str(e), xml)
        raise CloudletGenerationError(message)

def copy_with_xml(in_path, out_path, xml):
    fin = open(in_path)
    fout = open(out_path, 'w+b')
    hdr = vmnetx._QemuMemoryHeader(fin)

    # Write header
    hdr.xml = xml
    hdr.write(fout)
    fout.flush()

    # move to the content
    hdr.seek_body(fin)
    fout.write(fin.read())


def main(argv):
    MODE = ('base', 'overlay', 'synthesis', "test")
    USAGE = 'Usage: %prog ' + ("[%s]" % "|".join(MODE)) + " [paths..]"
    VERSION = '%prog 0.1'
    DESCRIPTION = 'Cloudlet Overlay Generation & Synthesis'

    parser = OptionParser(usage=USAGE, version=VERSION, description=DESCRIPTION)
    opts, args = parser.parse_args()
    if len(args) == 0:
        parser.error("Incorrect mode among %s" % "|".join(MODE))
    mode = str(args[0]).lower()

    if mode == MODE[0]: #base VM generation
        if len(args) != 2:
            parser.error("Generating base VM requires 1 arguements\n1) VM disk path")
            sys.exit(1)
        # creat base VM
        disk_image_path = args[1]
        disk_path, mem_path = create_baseVM(disk_image_path)
        print "Base VM is created from %s" % disk_image_path
        print "Disk: %s" % disk_path
        print "Mem: %s" % mem_path
    elif mode == MODE[1]:   #overlay VM creation
        if len(args) != 2:
            parser.error("Overlay Creation requires 2 arguments\n1) Base disk path")
            sys.exit(1)
        # create overlay
        disk_path = args[1]
        overlay_files = create_overlay(disk_path)
        print "[INFO] meta file: %s" % overlay_files[0]
        print "[INFO] disk overlay : %s" % overlay_files[1]
        print "[INFO] mem overlay : %s" % overlay_files[2]
    elif mode == MODE[2]:   #synthesis
        if len(args) != 5:
            parser.error("Synthesis requires 4 arguments\n \
                    1)base-disk path\n \
                    2)overlay meta path\n \
                    3)overlay disk path\n \
                    4)overlay memory path")
            sys.exit(1)
        base_disk_path = args[1]
        meta = args[2]
        overlay_disk = args[3] 
        overlay_mem = args[4]
        modified_img, launch_mem, fuse = recover_launchVM(base_disk_path, meta, 
                overlay_disk, overlay_mem)

        # monitor modified chunks
        residue_img = os.path.join(fuse.mountpoint, 'disk', 'image')
        stream_modified = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_modified')
        stream_access = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_accessed')
        monitor = vmnetfs.StreamMonitor()
        monitor.add_path(stream_modified)
        monitor.add_path(stream_access)
        monitor.start()

        #resume VM
        conn = get_libvirt_connection()
        machine = None
        try:
            machine=run_snapshot(conn, residue_img, launch_mem, wait_vnc=True)
        except Exception as e:
            sys.stdout.write(str(e)+"\n")
        if machine:
            machine.destroy()
        # terminate
        fuse.terminate()
        monitor.terminate()
        monitor.join()
        if os.path.exists(modified_img):
            os.unlink(modified_img)
        if os.path.exists(launch_mem):
            os.unlink(launch_mem)
            
    elif mode == 'test_overlay_download':    # To be delete
        base_disk_path = "/home/krha/cloudlet/image/nova/base_disk"
        base_mem_path = "/home/krha/cloudlet/image/nova/base_memory"
        overlay_disk_url = "http://dagama.isr.cs.cmu.edu/overlay/nova_overlay_disk.lzma"
        overlay_mem_url = "http://dagama.isr.cs.cmu.edu/overlay/nova_overlay_mem.lzma"
        launch_disk, launch_mem = recover_launchVM_from_URL(base_disk_path, base_mem_path, overlay_disk_url, overlay_mem_url)
        conn = get_libvirt_connection()
        run_snapshot(conn, launch_disk, launch_mem, wait_vnc=True)
    elif mode == 'test_new_xml':    # To be delete
        in_path = args[1]
        out_path = in_path + ".new"
        hdr = vmnetx._QemuMemoryHeader(open(in_path))
        domxml = ElementTree.fromstring(hdr.xml)
        domxml.find('uuid').text = "new-uuid"
        new_xml = ElementTree.tostring(domxml)
        copy_with_xml(in_path, out_path, new_xml)

        hdr = vmnetx._QemuMemoryHeader(open(out_path))
        domxml = ElementTree.fromstring(hdr.xml)
        print "new xml is changed uuid to " + domxml.find('uuid').text
    elif mode == 'nic':
        mem_path = args[1]
        conn = get_libvirt_connection()
        hdr = vmnetx._QemuMemoryHeader(open(mem_path))
        rettach_nic(conn, hdr.xml)
    else:
        print "Invalid command"
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
