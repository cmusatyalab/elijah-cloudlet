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

import sys
import time
import os
import commands
from xml.etree import ElementTree
from uuid import uuid4

base_wait_time = 60*1 # 1 min


def create_base_with_kvm(image_file, base_image, base_mem):
    global base_wait_time
    os_type = 'linux'

    command_str = 'cp ' + image_file + ' ' + base_image
    commands.getoutput(command_str)
    telnet_port = 9999; vnc_port = 1

    cloudlet.run_image(base_image, telnet_port, vnc_port, os_type=os_type, terminal_mode=True)
    time.sleep(base_wait_time)
    cloudlet.run_migration(telnet_port, vnc_port, base_mem)

    return base_image, base_mem

def create_base_with_libvirt(image_file, base_image, base_mem):
    global base_wait_time
    BaseVM_xml = "../config/cloudlet_base.xml"

    # make new disk not to modify input disk
    libvirt_cloudlet.copy_disk(image_file, base_image)

    # edit default XML to use give disk image
    domxml = ElementTree.fromstring(BaseVM_xml)
    name_element = domxml.find('name')
    disk_element = domxml.getElementsByTagName('devices/disk/source')
    uuid_element = domxml.getElementsByTagName('uuid')
    if not name_element or not disk_element or not uuid_element:
        raise Exception("Malfomed XML input: %s", os.path.abspath(BaseVM_xml))
    name_element.text = "Test Base VM"
    uuid_element.text = uuid4()
    disk_element.set("file", os.path.abspath(base_image))
    #print "XML Converted"
    #print dom.toxml()

    # launch VM & vnc console
    conn = libvirt_cloudlet.get_libvirt_connection()
    machine = libvirt_cloudlet.run_vm(conn, ElementTree.tostring(domxml), vnc_disable=True)
    time.sleep(base_wait_time)
    libvirt_cloudlet.save_mem_snapshot(machine, base_mem)

    return base_image, base_mem

if __name__ == "__main__":
    library_path = "/home/krha/cloudlet/src/server/"
    image = "/home/krha/cloudlet/image/ubuntu-11.10-x86_64-server/ubuntu-11.qcow2"

    sys.path.append(library_path)
    import cloudlet
    import libvirt_cloudlet

    disk1, mem1 = create_base_with_kvm(image, "./kvm_base_disk", "./kvm_base_mem" )
    print "Create base : %s, %s" % (disk1, mem1)

    disk2, mem2 = create_base_with_libvirt(image, "./libvirt_base_disk", "libvirt_base_mem")
    print "Create base : %s, %s" % (disk2, mem2)

