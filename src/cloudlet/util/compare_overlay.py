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

import os
import sys
from optparse import OptionParser
sys.path.insert(0, "../src/")

from cloudlet import msgpack
from cloudlet import synthesis as synthesis
from cloudlet.Configuration import Const as Const


def compare_vm_overlay(overlay_meta1, overlay_meta2):
    def _get_modified_chunks(metafile):
        meta1 = msgpack.unpackb(open(metafile, "r").read())
        overlay_files = meta1[Const.META_OVERLAY_FILES]
        disk_chunks = list()
        memory_chunks = list()
        for each_file in overlay_files:
            disk_chunks += each_file[Const.META_OVERLAY_FILE_DISK_CHUNKS]
            memory_chunks += each_file[Const.META_OVERLAY_FILE_MEMORY_CHUNKS]

        disk_chunks_set = set(disk_chunks)
        memory_chunks_set = set(memory_chunks)
        if len(disk_chunks_set) != len(disk_chunks):
            raise Exception("Have duplicated data while converting from list to set")
        if len(memory_chunks_set) != len(memory_chunks):
            raise Exception("Have duplicated data while converting from list to set")
        return disk_chunks_set, memory_chunks_set

    disk_chunks1, memory_chunks1 = _get_modified_chunks(overlay_meta1)
    disk_chunks2, memory_chunks2 = _get_modified_chunks(overlay_meta2)
    import pdb;pdb.set_trace()


def process_command_line(argv):
    VERSION = '%s' % Const.VERSION
    DESCRIPTION = 'Compare VM overlays'

    parser = OptionParser(usage='%prog VM_overlay1 VM_overlay2', 
            version=VERSION, description=DESCRIPTION)
    settings, args = parser.parse_args(argv)

    return settings


def main(argv):
    #settings = process_command_line(sys.argv[1:])
    if len(sys.argv) != 3:
        sys.stderr.write('usage : %prog VM_overlay1 VM_overlay2\n')
        sys.exit(1)

    # sanity check
    vm_overlay1 = sys.argv[1]
    if os.path.exists(vm_overlay1) == False:
        sys.stderr.write("not a valid file at %s" % vm_overlay1)
        sys.exit(1)
    vm_overlay2 = sys.argv[2]
    if os.path.exists(vm_overlay2) == False:
        sys.stderr.write("not a valid file at %s" % vm_overlay2)
        sys.exit(1)

    compare_vm_overlay(vm_overlay1, vm_overlay2)


    return 0

if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
