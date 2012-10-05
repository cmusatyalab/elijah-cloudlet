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
import Memory
import Disk
import vmnetfs
import vmnetx
import stat
import delta
from delta import DeltaList
from delta import DeltaItem
from xml.etree import ElementTree
from xml.etree.ElementTree import Element
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

        #check sanity
        if check_exist==True:
            _check_path('base memory', mempath)
            _check_path('base disk-hash', diskmeta)
            _check_path('base memory-hash', memmeta)

        return diskmeta, mempath, memmeta


class CloudletGenerationError(Exception):
    pass


def copy_disk(in_path, out_path):
    print "[INFO] Copying disk image to %s" % out_path
    cmd = "cp %s %s" % (in_path, out_path)
    cp_proc = subprocess.Popen(cmd, shell=True)
    cp_proc.wait()
    if cp_proc.returncode != 0:
        raise IOError("Copy failed: from %s to %s " % (in_path, out_path))


def convert_xml(xml, conn, vm_name=None, disk_path=None, uuid=None, logfile=None):
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

    # append QEMU-argument
    if logfile:
        qemu_xmlns="http://libvirt.org/schemas/domain/qemu/1.0"
        qemu_element = xml.find("{%s}commandline" % qemu_xmlns)
        if qemu_element == None:
            qemu_element = Element("{%s}commandline" % qemu_xmlns)
            xml.append(qemu_element)
            #msg = "qemu_xmlns is NULL, Malfomed XML input: %s\n%s" % \
            #        (Const.TEMPLATE_XML, ElementTree.tostring(xml))
            #raise CloudletGenerationError(msg)
        qemu_element.append(Element("{%s}arg" % qemu_xmlns, {'value':'-cloudlet'}))
        qemu_element.append(Element("{%s}arg" % qemu_xmlns, {'value':"logfile=%s" % logfile}))

    return ElementTree.tostring(xml)


def get_libvirt_connection():
    conn = libvirt.open("qemu:///session")
    return conn


def create_baseVM(disk_image_path):
    # Create Base VM(disk, memory) snapshot using given VM disk image
    # :param disk_image_path : file path of the VM disk image
    # :returns: (generated base VM disk path, generated base VM memory path)

    (base_diskmeta, base_mempath, base_memmeta) = \
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

    # edit default XML to have new disk path
    conn = get_libvirt_connection()
    xml = ElementTree.fromstring(open(Const.TEMPLATE_XML, "r").read())
    new_xml_string = convert_xml(xml, conn, disk_path=disk_image_path, uuid=str(uuid4()))

    # launch VM & wait for end of vnc
    machine = None
    try:
        machine = run_vm(conn, new_xml_string, wait_vnc=True)
        # make memory snapshot
        # VM has to be paused first to perform stable disk hashing
        save_mem_snapshot(machine, base_mempath)
        base_mem = Memory.hashing(base_mempath)
        base_mem.export_to_file(base_memmeta)

        # generate disk hashing
        # TODO: need more efficient implementation, e.g. bisect
        Disk.hashing(disk_image_path, base_diskmeta, print_out=Log.out)
    except Exception as e:
        sys.stderr.write(str(e)+"\n")
        if machine:
            machine.destroy()
        sys.exit(1)

    # write protection
    os.chmod(disk_image_path, stat.S_IRUSR)
    os.chmod(base_diskmeta, stat.S_IRUSR)
    os.chmod(base_mempath, stat.S_IRUSR)
    os.chmod(base_memmeta, stat.S_IRUSR)
    return disk_image_path, base_mempath


def create_overlay(base_image):
    # create user customized overlay.
    # First resume VM, then let user edit its VM
    # Finally, return disk/memory binary as an overlay
    # base_image: path to base disk

    (base_diskmeta, base_mem, base_memmeta) = \
            Const.get_basepath(base_image, check_exist=True)
    
    # filename for overlay VM
    qemu_logfile = NamedTemporaryFile(prefix="cloudlet-qemu-log-", delete=False)
    image_name = os.path.basename(base_image).split(".")[0]
    dir_path = os.path.dirname(base_mem)
    overlay_diskpath = os.path.join(dir_path, image_name+Const.OVERLAY_DISK)
    overlay_mempath = os.path.join(dir_path, image_name+Const.OVERLAY_MEM)
    
    # make FUSE disk & memory
    fuse = run_fuse(Const.VMNETFS_PATH, Const.CHUNK_SIZE, base_image, base_mem)
    modified_disk = os.path.join(fuse.mountpoint, 'disk', 'image')
    base_mem_fuse = os.path.join(fuse.mountpoint, 'memory', 'image')
    modified_mem = NamedTemporaryFile(prefix="cloudlet-mem-", delete=False)
    # monitor modified chunks
    stream_modified = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_modified')
    stream_access = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_accessed')
    memory_access = os.path.join(fuse.mountpoint, 'memory', 'streams', 'chunks_accessed')
    monitor = vmnetfs.StreamMonitor()
    monitor.add_path(stream_modified, vmnetfs.StreamMonitor.DISK_MODIFY)
    monitor.add_path(stream_access, vmnetfs.StreamMonitor.DISK_ACCESS)
    monitor.add_path(memory_access, vmnetfs.StreamMonitor.MEMORY_ACCESS)
    monitor.start()
    qemu_monitor = vmnetfs.FileMonitor(qemu_logfile.name, vmnetfs.FileMonitor.QEMU_LOG)
    qemu_monitor.start()

    # 1-1. resume & get modified disk
    conn = get_libvirt_connection()
    machine = run_snapshot(conn, modified_disk, base_mem_fuse, 
            wait_vnc=True, qemu_logfile=qemu_logfile.name)
    # 1-2. get modified memory
    # TODO: support stream of modified memory rather than tmp file
    save_mem_snapshot(machine, modified_mem.name)
    # 1-3. get hashlist of base memory and disk
    basemem_hashlist = Memory.base_hashlist(base_memmeta)
    basedisk_hashlist = Disk.base_hashlist(base_diskmeta)

    # 2-1. get memory overlay
    mem_footer, mem_deltalist= Memory.create_memory_overlay(modified_mem.name, 
            basemem_meta=base_memmeta, basemem_path=base_mem,
            basedisk_hashlist=basedisk_hashlist, basedisk_path=base_image,
            print_out=Log.out)
    Log.out.write("[Debug] Statistics for Memory overlay\n")
    DeltaList.statistics(mem_deltalist, print_out=Log.out)
    DeltaList.tofile_with_footer(mem_footer, mem_deltalist, overlay_mempath)

    # 2-2. get disk overlay
    m_chunk_list = monitor.chunk_list
    m_chunk_list.sort()
    packed_chunk_list = dict((x,x) for x in m_chunk_list).values()
    disk_deltalist = Disk.create_disk_overlay(modified_disk,
            packed_chunk_list, Const.CHUNK_SIZE,
            basedisk_hashlist=basedisk_hashlist, basedisk_path=base_image,
            basemem_hashlist=basemem_hashlist, basemem_path=base_mem,
            qemu_logfile=qemu_logfile.name,
            print_out=Log.out)

    # 2-3. disk-memory de-duplication
    # update disk delta list using memory delta list
    delta.diff_with_deltalist(disk_deltalist, mem_deltalist, DeltaItem.REF_OVERLAY_MEM)
    Log.out.write("[Debug] Statistics for Disk overlay\n")
    DeltaList.statistics(disk_deltalist, print_out=Log.out)
    DeltaList.tofile(disk_deltalist, overlay_diskpath)

    # 3. terminting
    monitor.terminate()
    qemu_monitor.terminate()
    monitor.join()
    qemu_monitor.join()
    os.unlink(modified_mem.name)
    #if os.path.exists(qemu_logfile.name):
    #    os.unlink(qemu_logfile.name)

    return (overlay_diskpath, overlay_mempath)
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
    log = kwargs.get('log', open("/dev/null", "w+b"))
    nova_util = kwargs.get('nova_util', None)

    (base_diskmeta, base_mem, base_memmeta) = \
            Const.get_basepath(base_image, check_exist=True)
    modified_mem = NamedTemporaryFile(prefix="cloudlet-recoverd-mem-", delete=False)
    modified_img = NamedTemporaryFile(prefix="cloudlet-recoverd-img-", delete=False)

    # Recover Modified Memory
    memory_overlay_map = Memory.recover_memory(base_image, base_mem, overlay_mem, \
            base_memmeta, modified_mem.name)

    # Recover Modified Disk
    disk_overlay_map = Disk.recover_disk(base_image, base_mem, modified_mem.name,
            overlay_disk, modified_img.name, Const.CHUNK_SIZE)


    print "[INFO] VM Disk is recovered at %s" % modified_img.name
    print "[INFO] VM Memory is recoverd at %s" % modified_mem.name
    # make FUSE disk & memory
    fuse = run_fuse(Const.VMNETFS_PATH, Const.CHUNK_SIZE, base_image, base_mem, 
            resumed_disk=modified_img.name, disk_overlay_map=disk_overlay_map,
            resumed_memory=modified_mem.name, memory_overlay_map=memory_overlay_map)

    return [modified_img.name, modified_mem.name, fuse]


def run_fuse(bin_path, chunk_size, original_disk, original_memory,
        resumed_disk=None, disk_overlay_map=None, 
        resumed_memory=None, memory_overlay_map=None):
    # run fuse file system

    resumed_disk = os.path.abspath(resumed_disk) if resumed_disk else ""
    resumed_memory = os.path.abspath(resumed_memory) if resumed_memory else ""
    disk_overlay_map = str(disk_overlay_map) if disk_overlay_map else ""
    memory_overlay_map = str(memory_overlay_map) if memory_overlay_map else ""

    # launch fuse
    execute_args = ['', '', \
            # disk parameter
            'http://dummy.url/', 
            "%s" % os.path.abspath(original_disk),  # base path
            "%s" % resumed_disk,                    # overlay path
            "%s" % disk_overlay_map,                # overlay map
            '%d' % os.path.getsize(original_disk),  # size of base
            '0',                                    # segment size
            "%d" % chunk_size]
    if original_memory:
        for parameter in [
                # memory parameter
                'http://dummy.url/', 
                "%s" % os.path.abspath(original_memory), 
                "%s" % resumed_memory, 
                "%s" % memory_overlay_map, 
                '%d' % os.path.getsize(original_memory), 
                '0',\
                "%d" % chunk_size
                ]:
            execute_args.append(parameter)

    #print "Fuse argument %s" % ",".join(execute_args)

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
            shell=True, stdin=_PIPE, stdout=_PIPE, stderr=_PIPE)
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
    log = kwargs.get('log', )
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
    # qemu_logfile      :   log file for QEMU-KVM

    # read embedded XML at memory snapshot to change disk path
    hdr = vmnetx._QemuMemoryHeader(open(mem_snapshot))
    xml = ElementTree.fromstring(hdr.xml)
    logfile = kwargs.get('qemu_logfile', None)
    new_xml_string = convert_xml(xml, conn, disk_path=disk_image, 
            uuid=uuid4(), logfile=logfile)
    overwrite_xml(mem_snapshot, new_xml_string)
    #temp_mem = NamedTemporaryFile(prefix="cloudlet-mem-")
    #copy_with_xml(mem_snapshot, temp_mem.name, new_xml_string)

    # resume
    restore_with_config(conn, mem_snapshot, new_xml_string)

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


def overwrite_xml(in_path, new_xml):
    fin = open(in_path, "rb")
    hdr = vmnetx._QemuMemoryHeader(fin)
    fin.close()

    # Write header
    fin = open(in_path, "r+b")
    hdr.overwrite(fin, new_xml)
    fin.close()


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


def synthesis(base_disk, meta, overlay_disk, overlay_mem):
    # VM Synthesis and run recoverd VM
    # param base_disk : path to base disk
    # param meta : path to meta file for overlay
    # param overlay_disk : path to overlay disk file
    # param overlay_mem : path to overlay memory file
    print_out = sys.stdout

    # recover VM
    qemu_logfile = NamedTemporaryFile(prefix="cloudlet-qemu-log-", delete=False)
    print_out.write("1. recover launch VM")
    modified_img, launch_mem, fuse = recover_launchVM(base_disk, meta, 
            overlay_disk, overlay_mem, log=print_out)

    # monitor modified chunks
    residue_img = os.path.join(fuse.mountpoint, 'disk', 'image')
    stream_modified = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_modified')
    stream_disk_access = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_accessed')
    stream_memory_access = os.path.join(fuse.mountpoint, 'memory', 'streams', 'chunks_accessed')
    monitor = vmnetfs.StreamMonitor()
    monitor.add_path(stream_modified, vmnetfs.StreamMonitor.DISK_MODIFY)
    monitor.add_path(stream_disk_access, vmnetfs.StreamMonitor.DISK_ACCESS)
    monitor.add_path(stream_memory_access, vmnetfs.StreamMonitor.MEMORY_ACCESS)
    monitor.start() 
    qemu_monitor = vmnetfs.FileMonitor(qemu_logfile.name, vmnetfs.FileMonitor.QEMU_LOG)
    qemu_monitor.start()

    #resume VM
    conn = get_libvirt_connection()
    machine = None
    try:
        machine=run_snapshot(conn, residue_img, launch_mem, wait_vnc=True, 
                qemu_logfile=qemu_logfile.name)
    except Exception as e:
        sys.stdout.write(str(e)+"\n")
    if machine:
        machine.destroy()

    # terminate
    fuse.terminate()
    monitor.terminate()
    qemu_monitor.terminate()
    monitor.join()
    qemu_monitor.join()
    
    # delete all temporary file
    if os.path.exists(modified_img):
        os.unlink(modified_img)
    if os.path.exists(launch_mem):
        os.unlink(launch_mem)
    if os.path.exists(qemu_logfile.name):
        os.unlink(qemu_logfile.name)


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
        print "[INFO] disk overlay : %s" % overlay_files[0]
        print "[INFO] mem overlay : %s" % overlay_files[1]
    elif mode == MODE[2]:   #synthesis
        if len(args) != 4:
            parser.error("Synthesis requires 4 arguments\n \
                    1)base-disk path\n \
                    2)overlay disk path\n \
                    3)overlay memory path")
            sys.exit(1)
        base_disk_path = args[1]
        meta = None
        overlay_disk = args[2] 
        overlay_mem = args[3]

        synthesis(base_disk_path, meta, overlay_disk, overlay_mem)
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
