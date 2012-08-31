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
import vmnetfs
import vmnetx
import delta
from xml.etree import ElementTree
from uuid import uuid4
from tempfile import mkstemp 
from tempfile import NamedTemporaryFile
from time import time
from time import sleep
from optparse import OptionParser
from json import dumps
from json import loads

from tool import comp_lzma
from tool import decomp_lzma
from tool import diff_files
from tool import merge_files
from tool import sha1_fromfile


class Const(object):
    BASE_DISK   = ".base-img"
    BASE_MEM    = ".base-mem"
    OVERLAY_DISK    = ".overlay-img"
    OVERLAY_MEM     = ".overlay-mem"
    OVERLAY_META    = ".overlay-meta"

    TEMPLATE_XML = "./config/VM_TEMPLATE.xml"
    VMNETFS_PATH = "/home/krha/cloudlet/src/vmnetx/vmnetfs/vmnetfs"
    CHUNK_SIZE=4096


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

    image_name = os.path.splitext(disk_image_path)[0]
    dir_path = os.path.dirname(disk_image_path)
    base_mempath = os.path.join(dir_path, image_name+Const.BASE_MEM)

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

    ret = raw_input("Your disk image will be modified. Proceed? (y/N) ")
    if str(ret).lower() != 'y':
        sys.exit(1)

    # edit default XML to use give disk image
    conn = get_libvirt_connection()
    xml = ElementTree.fromstring(open(Const.TEMPLATE_XML, "r").read())
    new_xml_string = convert_xml(xml, conn, disk_path=disk_image_path, uuid=str(uuid4()))

    # launch VM & wait for end of vnc
    machine = run_vm(conn, new_xml_string, wait_vnc=True)

    # make memory snapshot
    save_mem_snapshot(machine, base_mempath)

    # TODO: Get hashing meta

    return disk_image_path, base_mempath


def create_overlay(base_image, base_mem):
    image_name = os.path.splitext(base_image)[0]
    dir_path = os.path.dirname(base_mem)

    #check sanity
    if not os.path.exists(base_image):
        message = "Cannot find base image at %s" % (base_image)
        raise CloudletGenerationError(message)
    if not os.path.exists(base_mem):
        message = "Cannot find base memory at %s" % (base_mem)
        raise CloudletGenerationError(message)
    
    # filename for overlay VM
    image_name = os.path.basename(base_image).split(".")[0]
    dir_path = os.path.dirname(base_mem)
    metafile_path = os.path.join(dir_path, image_name+Const.OVERLAY_META)
    overlay_diskpath = os.path.join(dir_path, image_name+Const.OVERLAY_DISK)
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
    save_mem_snapshot(machine, modified_mem.name)

    # 2-1. get disk overlay
    m_chunk_list = monitor.chunk_list
    m_chunk_list.sort()
    delta.create_disk_overlay(overlay_diskpath, metafile_path, \
            modified_disk, m_chunk_list, Const.CHUNK_SIZE)

    # 2-2. get memory overlay
    monitor.terminate()
    monitor.join()
    return (metafile_path, overlay_diskpath, overlay_mempath)
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


def recover_launchVM_from_URL(base_disk_path, base_mem_path, overlay_disk_url, overlay_mem_url, **kwargs):
    # kwargs
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    log = kwargs.get('log', None)
    nova_util = kwargs.get('nova_util', None)

    def download(url, out_path):
        import urllib2
        url = urllib2.urlopen(url)
        out_file = open(out_path, "wb")
        while True:
            data = url.read(1024*1024)
            if not data:
                break
            out_file.write(data)
        out_file.flush()
        out_file.close()

    # download overlay
    basedir = os.path.dirname(base_mem_path)
    #overlay_disk = os.path.join("/tmp", "overlay.disk")
    #overlay_mem = os.path.join("/tmp", "overlay.mem")
    overlay_disk = os.path.join(basedir, "overlay.disk")
    overlay_mem = os.path.join(basedir, "overlay.mem")
    download(overlay_disk_url, overlay_disk)
    download(overlay_mem_url, overlay_mem)

    if log:
        log.debug("Download overlay disk : %d" % os.path.getsize(overlay_disk))
        log.debug("Download overlay mem: %d" % os.path.getsize(overlay_mem))
    else:
        print "Download overlay disk : %d" % os.path.getsize(overlay_disk)
        print "Download overlay mem: %d" % os.path.getsize(overlay_mem)

    # prepare meta data
    meta = os.path.join(basedir, "overlay_meta")
    metafile = open(meta, "w")
    metafile.write(dumps({"base_disk_path":os.path.abspath(base_disk_path),"base_mem_path":os.path.abspath(base_mem_path)}))
    metafile.close()

    # recover launch VM
    launch_disk, launch_mem = recover_launchVM(meta, overlay_disk, overlay_mem, \
            skip_validation=True, log=log, nova_util=nova_util)

    os.remove(overlay_disk)
    os.remove(overlay_mem)
    return launch_disk, launch_mem


def recover_launchVM(meta, overlay_disk, overlay_mem, **kwargs):
    # kwargs
    # skip_validation   :   skipp sha1 validation
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    log = kwargs.get('log', None)
    nova_util = kwargs.get('nova_util', None)

    # find base VM using meta
    if not os.path.exists(meta):
        message = "cannot find meta file at : %s" % os.path.abspath(meta)
        raise IOError(message)
    meta_info = loads(open(meta, "r").read()) # load json formatted meta info
    base_disk_path = meta_info['base_disk_path']
    base_mem_path = meta_info['base_mem_path']

    # base-vm validation with sha1 finger-print
    if not kwargs.get('skip_validation'):
        base_disk_sha1 = sha1_fromfile(base_disk_path)
        base_mem_sha1 = sha1_fromfile(base_mem_path)
        if base_disk_sha1 != meta_info['base_disk_sha1'] or base_mem_sha1 != meta_info['base_mem_sha1']:
            message = "sha1 does not match (%s!=%s) or (%s!=%s)" % \
                    (base_disk_sha1, meta_info['base_disk_sha1'], \
                    base_mem_sha1, meta_info['base_mem_sha1'])
            raise CloudletGenerationError(message)

    # add recover files
    recover_inputs= []
    recover_outputs = []
    recover_inputs.append((base_disk_path, overlay_disk))
    recover_inputs.append((base_mem_path, overlay_mem))

    for (base, comp) in recover_inputs:
        # decompress
        overlay = comp + '.decomp'
        prev_time = time()
        decomp_lzma(comp, overlay, nova_util=nova_util)
        msg = '[Time] Decompression(%s) - %s' % (comp, str(time()-prev_time))
        if log:
            log.debug(msg)
        else:
            print msg

        # merge with base image
        from random import randint
        #recover = os.path.join(os.path.dirname(base), 'recover_%04d.qcow2' % randint(0, 9999)); 
        recover = comp + '.recover.qcow2'
        prev_time = time()
        merge_files(base, overlay, recover, log=log, nova_util=nova_util)
        msg = '[Time] Recover(xdelta) image(%s) - %s' %(recover, str(time()-prev_time))
        if log:
            log.debug("base: %s, overlay: %s, recover: %s", base, overlay, recover)
            log.debug(msg)
        else:
            print msg

        #delete intermeidate files
        os.remove(overlay)
        recover_outputs.append(recover)
        
    return recover_outputs


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

    vnc_process = subprocess.Popen("gvncviewer localhost:%d" % vnc_port, shell=True)
    if kwargs.get('wait_vnc'):
        print "[INFO] waiting for finishing VNC interaction"
        vnc_process.wait()
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

    # Get memory metadata
    base = KVMMemory.Memory.load_from_kvm(fout_path, out_path=fout_path+KVMMemory.EXT_RAW)
    base.export_to_file(fout_path+KVMMemory.EXT_META)


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

    # Run VNC
    vnc_process = subprocess.Popen("gvncviewer localhost:%d" % vnc_port, shell=True)
    if kwargs.get('wait_vnc'):
        print "[INFO] waiting for finishing VNC interaction"
        vnc_process.wait()
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
        mem_path = os.path.splitext(disk_path)[0] + Const.BASE_MEM
        overlay_files = create_overlay(disk_path, mem_path)
        print "[INFO] meta file: %s" % overlay_files[0]
        print "[INFO] disk overlay : %s" % overlay_files[1]
        print "[INFO] mem overlay : %s" % overlay_files[2]
    elif mode == MODE[2]:   #synthesis
        if len(args) != 4:
            parser.error("Synthesis requires 3 arguments\n1)meta-info path\n2)overlay disk path\n3)overlay memory path")
            sys.exit(1)
        meta = args[1]
        overlay_disk = args[2] 
        overlay_mem = args[3]
        launch_disk, launch_mem = recover_launchVM(meta, overlay_disk, overlay_mem, skip_validation=True)
        conn = get_libvirt_connection()
        run_snapshot(conn, launch_disk, launch_mem, wait_vnc=True)
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


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
