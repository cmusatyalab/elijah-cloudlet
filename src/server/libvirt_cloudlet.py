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
import tempfile
import subprocess
from xml.dom.minidom import parse
from xml.dom.minidom import parseString
from uuid import uuid4
from vmnext import _QemuMemoryHeader

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


def create_baseVM(vm_name, disk_image_path):
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
    dom = parse(os.path.abspath(BaseVM_xml))
    name_elements = dom.getElementsByTagName('name')
    disk_elements = dom.getElementsByTagName('disk')
    uuid_elements = dom.getElementsByTagName('uuid')
    if not name_elements or not disk_elements or not uuid_elements:
        raise CloudletGenerationError("Malfomed XML input: %s", os.path.abspath(BASEVM_xml))
    name_elements[0].firstChild.nodeValue = vm_name
    uuid_elements[0].firstChild.nodeValue = uuid4()
    disk_source_element = disk_elements[0].getElementsByTagName('source')[0] 
    disk_source_element.setAttribute("file", os.path.abspath(base_diskpath))
    print "XML Converted"
    print dom.toxml()

    # launch VM & vnc console
    machine = run_vm(dom.toxml(), wait_vnc=True)

    # make a snapshot
    save_mem_snapshot(machine, base_mempath)

    return base_diskpath, base_mempath


def run_vm(libvirt_xml, **kwargs):
    # kargs
    # vnc_disable       :   show vnc console
    # wait_vnc          :   wait until vnc finishes if vnc_enabled

    conn = libvirt.open("qemu:///system")
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


def save_mem_snapshot(machine, fout_path):
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


def run_snapshot(disk_image, mem_snapshot, **kwargs):
    # kargs
    # vnc_disable       :   show vnc console
    # wait_vnc          :   wait until vnc finishes if vnc_enabled

    # read embedded XML at memory snapshot to change disk path
    hdr = _QemuMemoryHeader(open(mem_snapshot))
    domxml = parseString(hdr.xml)
    disk_elements = domxml.getElementsByTagName('disk')
    uuid = domxml.getElementsByTagName('disk')
    if not disk_elements:
        raise CloudletGenerationError("Malfomed XML embedded: %s", os.path.abspath(mem_snapshot))
    disk_source_element = disk_elements[0].getElementsByTagName('source')[0] 
    disk_source_element.setAttribute("file", os.path.abspath(disk_image))
    print "[INFO] XML modified to new disk path"
    print domxml.toxml()

    # resume
    conn = libvirt.open("qemu:///system")
    print "[INFO] restoring VM..."
    try:
        conn.restoreFlag(mem_snapshot, domxml.toxml(), libvirt.VIR_DOMAIN_SAVE_RUNNIGN)
    except libvirt.libvirtError, e:
        raise CloudletGenerationError(str(e))

    # get machine
    machine = libvirt.lookupByUUID(uuid)

    # Run VNC and wait until user finishes working
    if kwargs.get('vnc_disable'):
        return machine
    vnc_process = subprocess.Popen("gvncviewer localhost:0", shell=True)
    if kwargs.get('wait_vnc'):
        print "[INFO] waiting for finishing VNC interaction"
        vnc_process.wait()
    return machine


def main(argv):
    # creat base VM
    vm_name = 'test_ubuntu_base'
    disk_image_path = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.moped.qcow2'
    disk_path, mem_path = create_baseVM(vm_name, disk_image_path)
    print "Base VM is created from %s" % disk_image_path
    print "Disk: %s" % disk_path
    print "Mem: %s" % mem_path

    # run snapshot
    raw_input("Will resume VM? ")
    disk = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.base.disk'
    mem = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.base.mem'
    run_snapshot(disk, mem, vnc_wait=True)


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
