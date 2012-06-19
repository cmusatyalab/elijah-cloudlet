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
from tempfile import mkstemp 
from time import time
from optparse import OptionParser
from json import dumps
from json import loads

from vmnext import _QemuMemoryHeader
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
    #print "XML Converted"
    #print dom.toxml()

    # launch VM & vnc console
    machine = run_vm(dom.toxml(), wait_vnc=True)
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
    machine = run_snapshot(modified_disk, base_mem, wait_vnc=True)
    #generate modified memory snapshot
    save_mem_snapshot(machine, modified_mem)

    output_list = []
    output_list.append((base_image, modified_disk, overlay_diskpath))
    output_list.append((base_mem, modified_mem, overlay_mempath))

    # xdelta and compression
    ret_files = []
    ret_files.append(meta_file_path)
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


def recover_launchVM(meta, overlay_disk, overlay_mem, **kargs):
    # kargs
    # vnc_disable       :   show vnc console
    # wait_vnc          :   wait until vnc finishes if vnc_enabled

    # find base VM using meta
    if not os.path.exists(meta):
        message = "cannot find meta file at : %s" % os.path.abspath(meta)
        raise IOError(message)
    meta_info = loads(open(meta, "r").read()) # load json formatted meta info
    base_disk_path = meta_info['base_disk_path']
    base_mem_path = meta_info['base_mem_path']
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
        decomp_lzma(comp, overlay)
        print '[Time] Decompression(%s) - %s' % (comp, str(time()-prev_time))

        # merge with base image
        recover = os.path.join(os.path.dirname(base), os.path.basename(comp) + '.recover'); 
        prev_time = time()
        merge_files(base, overlay, recover)
        print '[Time] Recover(xdelta) image(%s) - %s' %(recover, str(time()-prev_time))

        #delete intermeidate files
        os.remove(overlay)
        recover_outputs.append(recover)
        
    return recover_outputs


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
    #print "[INFO] XML modified to new disk path (%s)" % (uuid)
    #print domxml.toxml()

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
    USAGE = 'Usage: %prog ' + ("[%s]" % "|".join(MODE)) + " [paths..]"
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
        launch_disk, launch_mem = recover_launchVM(meta, overlay_disk, overlay_mem)
        run_snapshot(launch_disk, launch_mem, wait_vnc=True)


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
