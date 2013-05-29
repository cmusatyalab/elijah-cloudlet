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

if __name__ == "__main__":
    library_path = "/home/krha/cloudlet/src/server/"
    base_disk = "/home/krha/cloudlet/src/server/util/libvirt_base_disk"
    base_mem = "/home/krha/cloudlet/src/server/util/libvirt_base_mem"
    overlay_disk = "/home/krha/cloudlet/src/server/util/libvirt_base_disk.modi"
    overlay_mem = "/home/krha/cloudlet/src/server/util/libvirt_base_mem.modi"

    sys.path.append(library_path)
    import tool

    #extract hash
    base_disk_hash = tool.extract_hashlist(open(base_disk, "rb"))
    base_mem_hash = tool.extract_hashlist(open(base_mem, "rb"))
    overlay_mem_hash = tool.extract_hashlist(open(overlay_mem, "rb"))

    #delta
    hash_for_disk = [(1, base_disk, base_disk_hash), (2, base_mem, base_mem_hash), (3, overlay_mem, overlay_mem_hash)]
    hash_for_mem = [(1, base_disk, base_disk_hash), (2, base_mem, base_mem_hash)]
    disk_deltalist = tool.get_delta(open(overlay_disk, "rb"), hash_for_disk)
    mem_deltalist = tool.get_delta(open(overlay_mem, "rb"), hash_for_mem)
    tool.deltalist_to_file(disk_deltalist, overlay_disk+".delta")
    tool.deltalist_to_file(mem_deltalist, overlay_mem+".delta")

    #merge
    recovered_disk = tool.merge_delta(disk_deltalist, hash_for_disk)
    recovered_mem = tool.merge_delta(mem_deltalist, hash_for_mem)
    open(overlay_disk+".recover", "wb").write(recovered_disk)
    open(overlay_mem+".recover", "wb").write(recovered_mem)

    #check sanity
    original = tool.sha1_fromfile(overlay_disk)
    recover = tool.sha1_fromfile(overlay_disk+".recover")
    if recover != original:
        print "Error, recover failed %s != %s" % (recover, original)
    else:
        print "Success, recovered %s == %s" % (recover, original)

