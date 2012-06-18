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
from xml.dom.minidom import parse
from xml.dom.minidom import parseString
from uuid import uuid4
from tempfile import NamedTemporaryFile
from time import time
from optparse import OptionParser

from vmnext import _QemuMemoryHeader
from tool import comp_lzma
from tool import decomp_lzma
from tool import diff_files
from tool import merge_files

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
        raise CloudletGenerationError("Malfomed XML input: %s", os.path.abspath(BaseVM_xml))
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


def create_overlay(base_image, base_mem):
    #check sanity
    if not os.path.exists(base_image) or not os.path.exists(base_mem):
        raise CloudletGenerationError("Cannot find base path at \n%s, \n%s" % (base_image, base_mem))

    #make modified disk
    modified_disk = NamedTemporaryFile(prefix="cloudlet-disk-", delete=False)
    modified_mem = NamedTemporaryFile(prefix="cloudlet-mem-", delete=False)
    modified_disk.close()
    modified_mem.close()
    copy_disk(base_image, modified_disk.name)

    #filename for overlay VM
    image_name = os.path.basename(base_image).split(".")[0]
    overlay_diskpath = os.path.join(os.path.dirname(base_image), image_name+".overlay.disk")
    overlay_mempath = os.path.join(os.path.dirname(base_mem), image_name+".overlay.mem")

    #resume with modified disk
    machine = run_snapshot(modified_disk.name, base_mem, wait_vnc=True)
    #generate modified memory snapshot
    save_mem_snapshot(machine, modified_mem.name)

    output_list = []
    output_list.append((base_image, modified_disk.name, overlay_diskpath))
    output_list.append((base_mem, modified_mem.name, overlay_mempath))

    # xdelta and compression
    ret_files = []
    for (base, modified, overlay) in output_list:
        start_time = time()

        # xdelta
        ret = diff_files(base, modified, overlay)
        print '[TIME] time for creating overlay : ', str(time()-start_time)
        print '[INFO] (%d)-(%d)=(%d): ' % (os.path.getsize(base), os.path.getsize(modified), os.path.getsize(overlay))
        if ret == None:
            print >> sys.stderr, '[ERROR] cannot create overlay ' + str(overlay)
            if os.path.exists(modified):
                os.remove(modified)
            continue
        
        # compression
        comp = overlay + '.lzma'
        comp, time1 = comp_lzma(overlay, comp)
        ret_files.append(comp)

        # remove temporary files
        os.remove(modified)
        os.remove(overlay)

    return ret_files


def run_vm(libvirt_xml, **kwargs):
    # kargs
    # vnc_disable       :   show vnc console
    # wait_vnc          :   wait until vnc finishes if vnc_enabled

    conn = libvirt.open("qemu:///session")
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
    uuid_elements = domxml.getElementsByTagName('uuid')
    if not disk_elements:
        raise CloudletGenerationError("Malfomed XML embedded: %s", os.path.abspath(mem_snapshot))
    disk_source_element = disk_elements[0].getElementsByTagName('source')[0] 
    disk_source_element.setAttribute("file", os.path.abspath(disk_image))
    uuid = str(uuid_elements[0].firstChild.nodeValue)
    print "[INFO] XML modified to new disk path (%s)" % (uuid)
    print domxml.toxml()

    # resume
    conn = libvirt.open("qemu:///session")
    print "[INFO] restoring VM..."
    try:
        conn.restoreFlags(mem_snapshot, domxml.toxml(), libvirt.VIR_DOMAIN_SAVE_RUNNING)
    except libvirt.libvirtError, e:
        raise CloudletGenerationError(str(e))

    # get machine
    machine = conn.lookupByUUIDString(uuid)

    # Run VNC and wait until user finishes working
    if kwargs.get('vnc_disable'):
        return machine
    vnc_process = subprocess.Popen("gvncviewer localhost:0", shell=True)
    if kwargs.get('wait_vnc'):
        print "[INFO] waiting for finishing VNC interaction"
        vnc_process.wait()
    return machine


def main(argv):
    MODE = ('base', 'overlay', 'synthesis')
    USAGE = 'Usage: %prog ' + ("[%s]" % "|".join(MODE))
    VERSION = '%prog 0.1'
    DESCRIPTION = 'Cloudlet Overlay Generation & Synthesis'

    parser = OptionParser(usage=USAGE, version=VERSION, description=DESCRIPTION)
    opts, args = parser.parse_args()
    if len(args) == 0:
        parser.error("Incorrect mode among %s" % "|".join(MODE))
    mode = str(args[0]).lower()
    if mode not in MODE:
        parser.error("Incorrect mode %s" % mode)

    if mode == MODE[0]: #base VM generation
        # creat base VM
        vm_name = 'test_ubuntu_base'
        disk_image_path = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.moped.qcow2'
        disk_path, mem_path = create_baseVM(vm_name, disk_image_path)
        print "Base VM is created from %s" % disk_image_path
        print "Disk: %s" % disk_path
        print "Mem: %s" % mem_path
    elif mode == MODE[1]:   #overlay VM creation
        # create overlay
        disk = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.moped.qcow2'
        mem = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.base.mem'
        overlay_files = create_overlay(disk, mem)
        print "[INFO] disk overlay : %s" % overlay_files[0]
        print "[INFO] mem overlay : %s" % overlay_files[1]
    elif mode == MODE[2]:   #synthesis  
        print "To be implemented"

    # run snapshot
    '''
    disk = './ubuntu.base.disk'
    mem = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.base.mem'
    run_snapshot(disk, mem, vnc_wait=True)
    '''


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
