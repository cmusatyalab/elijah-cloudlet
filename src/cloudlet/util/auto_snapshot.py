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

import sys
import time
import os
import commands
from xml.etree import ElementTree
from uuid import uuid4

base_wait_time = 300*1 # 1 min
overlay_wait_time = 60*1 # 1 min


def create_base_with_kvm(image_file, base_image, base_mem):
    global base_wait_time
    os_type = 'linux'

    command_str = 'cp ' + image_file + ' ' + base_image
    commands.getoutput(command_str)
    telnet_port = 9999; vnc_port = 1

    cloudlet.run_image(base_image, telnet_port, vnc_port, os_type=os_type, terminal_mode=True)
    print "waiting for %d seconds" % base_wait_time 
    time.sleep(base_wait_time)
    cloudlet.run_migration(telnet_port, vnc_port, base_mem)

    return base_image, base_mem

def create_base_with_libvirt(image_file, base_image, base_mem):
    global base_wait_time
    BaseVM_xml = "./cloudlet_base.xml"

    # make new disk not to modify input disk
    synthesis.copy_disk(image_file, base_image)

    # edit default XML to use give disk image
    domxml = ElementTree.fromstring(open(BaseVM_xml, "r").read())
    name_element = domxml.find('name')
    disk_element = domxml.find('devices/disk/source')
    uuid_element = domxml.find('uuid')
    if name_element == None or disk_element == None or uuid_element == None:
        raise Exception("Malfomed XML input: %s", os.path.abspath(BaseVM_xml))
    name_element.text = "Test Base VM"
    uuid_element.text = str(uuid4())
    disk_element.set("file", os.path.abspath(base_image))

    # launch VM & vnc console
    conn = synthesis.get_libvirt_connection()
    machine = synthesis.run_vm(conn, ElementTree.tostring(domxml), vnc_disable=True)
    print "waiting for %d seconds" % base_wait_time 
    time.sleep(base_wait_time)
    synthesis.save_mem_snapshot(machine, base_mem)

    # make hashing info
    base_disk_hash = tool.extract_hashlist(open(base_image, "rb"))
    base_mem_hash = tool.extract_hashlist(open(base_mem, "rb"))
    tool.hashlist_to_file(base_disk_hash, base_image+".hash")
    tool.hashlist_to_file(base_mem_hash, base_mem+".hash")

    return base_image, base_mem


def create_overlay_with_kvm(base_disk, base_mem, overlay_disk, overlay_mem):
    tmp_disk = base_disk + ".modi"
    tmp_mem = base_mem + ".modi"
    telnet_port = 9999
    vnc_port = 1
    command_str = 'qemu-img create -f qcow2 -b ' + base_disk + ' ' + tmp_disk
    commands.getoutput(command_str)
    cloudlet.run_snapshot(tmp_disk, base_mem, telnet_port, vnc_port, terminal_mode=True, os_type='linux')
    print "waiting for %d seconds" % overlay_wait_time
    time.sleep(overlay_wait_time)

    cloudlet.run_migration(telnet_port, vnc_port, tmp_mem)
    argument = []
    argument.append((base_disk, tmp_disk, overlay_disk))
    argument.append((base_mem, tmp_mem, overlay_mem))
    ret_files = cloudlet.run_delta_compress(argument)
    return ret_files


def create_overlay_with_libvirt(base_disk, base_mem, overlay_diskpath, overlay_mempath, custom_delta=False):
    meta_file_path = "./overlay_meta"

    #create meta file that has base VM information
    meta_file = open(meta_file_path, "w")
    meta_info = {"base_disk_path":base_disk, "base_mem_path":base_mem}
    import json
    meta_file.write(json.dumps(meta_info)) # save it as json format
    meta_file.close()
    
    #make modified disk
    modified_disk = base_disk + ".modi"
    modified_mem = base_mem + ".modi"
    synthesis.copy_disk(base_disk, modified_disk)

    #resume with modified disk
    conn = synthesis.get_libvirt_connection()
    machine = synthesis.run_snapshot(conn, modified_disk, base_mem, vnc_disable=True)
    print "waiting for %d seconds" % overlay_wait_time
    time.sleep(overlay_wait_time)

    synthesis.save_mem_snapshot(machine, modified_mem)
    output_list = []
    output_list.append((base_disk, modified_disk, overlay_diskpath))
    output_list.append((base_mem, modified_mem, overlay_mempath))

    ret_files = synthesis.run_delta_compression(output_list, custom_delta=custom_delta)
    return ret_files


if __name__ == "__main__":
    library_path = "/home/krha/cloudlet/src/server/"
    image = "/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.qcow2"

    sys.path.append(library_path)
    import cloudlet
    import synthesis
    import tool

    disk1, mem1 = create_base_with_kvm(image, "./kvm_base_disk", "./kvm_base_mem")
    libvirt_overlay_disk, libvirt_overlay_mem =create_overlay_with_kvm(disk1, mem1, \
            "kvm_overlay_disk", "./kvm_overlay_mem") 
    
    disk2, mem2 = create_base_with_kvm(image, "./kvm_base_disk", "kvm_base_mem")
    libvirt_overlay_disk, libvirt_overlay_mem =create_overlay_with_kvm(disk2, mem2, \
            "libvirt_overlay_disk", "./libvirt_overlay_mem") 

    disk2, mem2 = create_base_with_libvirt(image, "./libvirt_base_disk", "libvirt_base_mem")
    libvirt_overlay_disk, libvirt_overlay_mem =create_overlay_with_libvirt(disk2, mem2, \
            "libvirt_overlay_disk", "./libvirt_overlay_mem") 
    print "Create base : %s, %s" % (disk2, mem2)
    print "Create overlay: %s, %s" % (libvirt_overlay_disk, libvirt_overlay_mem)

