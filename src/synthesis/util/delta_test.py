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

