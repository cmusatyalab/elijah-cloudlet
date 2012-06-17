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

# default XML for Base VM
BaseVM_xml = "./config/cloudlet_base.xml"


def create_baseVM(vm_name, disk_image_path):
    global BaseVM_xml

    # check sanity
    if not os.path.exists(os.path.abspath(BaseVM_xml)):
        sys.stderr.write("Cannot find Base VM default XML at %s\n" % BaseVM_xml)
        sys.exit(1)

    # edit default XML to use give disk image
    dom = parse(os.path.abspath(BaseVM_xml))
    name_elements = dom.getElementsByTagName('name')
    disk_elements = dom.getElementsByTagName('disk')
    if not name_elements or not disk_elements:
        sys.stderr.write("Malformd Base XML: %s\n", BaseVM_xml)
        sys.exit(1)

    print name_elements[0].firstChild.nodeValue
    name_elements[0].firstChild.nodeValue = vm_name
    disk_source_element = disk_elements[0].getElementsByTagName('source')[0] 
    disk_source_element.setAttribute("file", os.path.abspath(disk_image_path))

    #tempxml_fd, tempxml_path = tempfile.mkstemp('cloudlet-xml-')
    #os.write(tempxml_fd, dom.toxml())
    #os.close(tempxml_fd)
    print "XML Converted"
    print dom.toxml()

    # launch VM & vnc console
    run_vm(dom.toxml(), wait_vnc=True)

    # make a snapshot


def run_vm(libvirt_xml, **kwargs):
    # kargs
    # vnc_disable       :   show vnc console
    # wait_vnc          :   wait until vnc finishes if vnc_enabled

    conn = libvirt.open("qemu:///system")
    # TODO: get right parameter for second argument
    conn.createXML(libvirt_xml, 0)

    # Run VNC and wait until user finishes working
    if kwargs.get('vnc_disable'):
        return;

    vnc_process = subprocess.Popen("gvncviewer localhost:0", shell=True)
    if kwargs.get('wait_vnc'):
        print "[INFO] waiting for finishing VNC interaction"
        vnc_process.wait()


def main(argv):
    # creat base VM
    vm_name = 'test_ubuntu_base'
    disk_image_path = '/home/krha/cloudlet/image/ubuntu-10.04-x86_64-desktop/ubuntu.moped.qcow2'
    uuid = create_baseVM(vm_name, disk_image_path)

if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
