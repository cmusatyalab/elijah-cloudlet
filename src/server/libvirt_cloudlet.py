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
from xml.etree import ElementTree
from uuid import uuid4
from tempfile import mkstemp 
from time import time
from time import sleep
from optparse import OptionParser
from json import dumps
from json import loads

from vmnetx import _QemuMemoryHeader
from tool import comp_lzma
from tool import decomp_lzma
from tool import diff_files
from tool import merge_files
from tool import sha1_fromfile

# default XML for Base VM
BaseVM_xml = "./config/cloudlet_base.xml"

class CloudletGenerationError(Exception):
    pass

def copy_disk(in_path, out_path):
    print "[INFO] Copying disk image to %s" % out_path
    cmd = "cp %s %s" % (in_path, out_path)
    cp_proc = subprocess.Popen(cmd, shell=True)
    cp_proc.wait()
    if cp_proc.returncode != 0:
        raise IOError("Copy failed: from %s to %s " % (in_path, out_path))


def get_libvirt_connection():
    conn = libvirt.open("qemu:///session")
    return conn

def create_baseVM(vm_name, disk_image_path):
    # Create Base VM(disk, memory) snapshot using given VM disk image
    # :param vm_name : Name of the Base VM
    # :param disk_image_path : file path of the VM disk image
    # :returns: (generated base VM disk path, generated base VM memory path)
    global BaseVM_xml

    # check sanity
    image_name = os.path.basename(disk_image_path).split(".")[0]
    base_diskpath = os.path.join(os.path.dirname(disk_image_path), image_name+".base.disk")
    base_mempath = os.path.join(os.path.dirname(disk_image_path), image_name+".base.mem")
    if not os.path.exists(os.path.abspath(BaseVM_xml)):
        sys.stderr.write("Cannot find Base VM default XML at %s\n" % BaseVM_xml)
        sys.exit(1)
    if os.path.exists(base_diskpath) or os.path.exists(base_mempath):
        key_in = "Warning: (%s) exist.\nAre you sure to overwrite? (y/N) " % (base_diskpath)
        ret = raw_input(key_in)
        if str(ret).lower() != 'y':
            sys.exit(1)

    # make new disk not to modify input disk
    copy_disk(disk_image_path, base_diskpath)

    # edit default XML to use give disk image
    domxml = ElementTree.fromstring(BaseVM_xml)
    name_element = domxml.find('name')
    disk_element = domxml.find('devices/disk/source')
    uuid_element = domxml.find('uuid')
    if name_element == None or disk_element == None or uuid_element == None:
        raise Exception("Malfomed XML input: %s", os.path.abspath(BaseVM_xml))
    name_element.text = vm_name
    uuid_element.text = str(uuid4())
    disk_element.set("file", os.path.abspath(base_diskpath))
    #print "XML Converted"
    #print ElementTree.tostring(domxml)

    # launch VM & vnc console
    conn = get_libvirt_connection()
    machine = run_vm(conn, ElementTree.tostring(domxml), wait_vnc=True)
    # make a snapshot
    save_mem_snapshot(machine, base_mempath)

    return base_diskpath, base_mempath


def create_overlay(base_image, base_mem):
    #check sanity
    base_image = os.path.abspath(base_image)
    base_mem = os.path.abspath(base_mem)
    if not os.path.exists(base_image) or not os.path.exists(base_mem):
        message = "Cannot find base path at %s, %s" % (base_image, base_mem)
        raise CloudletGenerationError(message)
    
    #filename for overlay VM
    image_name = os.path.basename(base_image).split(".")[0]
    meta_file_path = os.path.join(os.path.dirname(base_image), image_name+".overlay.meta")
    overlay_diskpath = os.path.join(os.path.dirname(base_image), image_name+".overlay.disk")
    overlay_mempath = os.path.join(os.path.dirname(base_mem), image_name+".overlay.mem")

    #create meta file that has base VM information
    meta_file = open(meta_file_path, "w")
    base_image_sha1 = sha1_fromfile(base_image)
    base_mem_sha1 = sha1_fromfile(base_mem)
    meta_info = {"base_disk_path":base_image, "base_disk_sha1":base_image_sha1,
            "base_mem_path":base_mem, "base_mem_sha1":base_mem_sha1}
    meta_file.write(dumps(meta_info)) # save it as json format
    meta_file.close()
    
    #make modified disk
    fd1, modified_disk = mkstemp(prefix="cloudlet-disk-")
    fd2, modified_mem = mkstemp(prefix="cloudlet-mem-")
    copy_disk(base_image, modified_disk)

    #resume with modified disk
    conn = get_libvirt_connection()
    machine = run_snapshot(conn, modified_disk, base_mem, wait_vnc=True)
    #generate modified memory snapshot
    save_mem_snapshot(machine, modified_mem)

    output_list = []
    output_list.append((base_image, modified_disk, overlay_diskpath))
    output_list.append((base_mem, modified_mem, overlay_mempath))

    ret_files = run_delta_compression(output_list)
    overlay_files = []
    overlay_files.append(meta_file_path)
    overlay_files.append(ret_files)
    return overlay_files


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


def run_vm(conn, libvirt_xml, **kwargs):
    # kwargs
    # vnc_disable       :   do not show vnc console
    # wait_vnc          :   wait until vnc finishes if vnc_enabled

    # TODO: get right parameter for second argument
    machine = conn.createXML(libvirt_xml, 0)

    # Run VNC and wait until user finishes working
    if kwargs.get('vnc_disable'):
        return machine

    vnc_process = subprocess.Popen("gvncviewer localhost:0", shell=True)
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
    hdr = _QemuMemoryHeader(open(mem_snapshot))
    domxml = ElementTree.fromstring(hdr.xml)
    disk_element = domxml.find('devices/disk/source')
    if not disk_element.get("file"):
        raise CloudletGenerationError("Malfomed XML embedded: %s" % os.path.abspath(mem_snapshot))
    disk_element.set("file", os.path.abspath(disk_image))

    # resume
    restore_with_config(conn, mem_snapshot, ElementTree.tostring(domxml))

    # get machine
    uuid_element = domxml.find('uuid')
    uuid = str(uuid_element.text)
    machine = conn.lookupByUUIDString(uuid)

    # Run VNC and wait until user finishes working
    if kwargs.get('vnc_disable'):
        return machine
    vnc_process = subprocess.Popen("gvncviewer localhost:0", shell=True)
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


def copy_with_uuid(in_path, out_path, uuid):
    fin = open(in_path)
    fout = open(out_path, 'w')
    hdr = _QemuMemoryHeader(fin)

    # Write header with new uuid
    domxml = ElementTree.fromstring(hdr.xml)
    domxml.find('uuid').text = uuid
    hdr.xml = ElementTree.tostring(domxml)
    hdr.write(fout)
    fout.flush()

    # move to the content
    hdr.seek_body(fin)
    fout.write(fin.read())


def copy_with_xml(in_path, out_path, xml):
    fin = open(in_path)
    fout = open(out_path, 'w')
    hdr = _QemuMemoryHeader(fin)

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
        if len(args) != 3:
            parser.error("Generating base VM requires 2 arguements\n1) vm name\n2) disk path")
            sys.exit(1)
        # creat base VM
        vm_name = args[1]
        disk_image_path = args[2]
        disk_path, mem_path = create_baseVM(vm_name, disk_image_path)
        print "Base VM is created from %s" % disk_image_path
        print "Disk: %s" % disk_path
        print "Mem: %s" % mem_path
    elif mode == MODE[1]:   #overlay VM creation
        if len(args) != 3:
            parser.error("Overlay Creation requires 2 arguments\n1) Base disk path\n2) Base mem path")
            sys.exit(1)
        # create overlay
        disk = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu_disk.base.disk'
        mem = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu_disk.base.mem'
        overlay_files = create_overlay(disk, mem)
        print "[INFO] meta_info : %s" % overlay_files[0]
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
        hdr = _QemuMemoryHeader(open(in_path))
        domxml = ElementTree.fromstring(hdr.xml)
        domxml.find('uuid').text = "new-uuid"
        new_xml = ElementTree.tostring(domxml)
        copy_with_xml(in_path, out_path, new_xml)

        hdr = _QemuMemoryHeader(open(out_path))
        domxml = ElementTree.fromstring(hdr.xml)
        print "new xml is changed uuid to " + domxml.find('uuid').text
    elif mode == 'nic':
        mem_path = args[1]
        conn = get_libvirt_connection()
        hdr = _QemuMemoryHeader(open(mem_path))
        rettach_nic(conn, hdr.xml)


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
