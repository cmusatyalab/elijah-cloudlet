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
from cloudlet import synthesis
from cloudlet.Configuration import Const as Const
from cloudlet import delta
from tempfile import NamedTemporaryFile


def get_indexed_delta_list(base_disk, overlay_metapath):
    from cloudlet.tool import decomp_overlay
    temp_overlay = NamedTemporaryFile(prefix="cloudlet-overlay-file-")
    meta = decomp_overlay(overlay_metapath, temp_overlay.name)
    (base_diskmeta, base_mem, base_memmeta) = \
            Const.get_basepath(base_disk, check_exist=True)
    delta_list = synthesis._reconstruct_mem_deltalist( \
            base_disk, base_mem, temp_overlay.name)
    indexed_delta_list = dict()
    for item in delta_list:
        indexed_delta_list[item.index] = item
    return delta_list, indexed_delta_list


def compare_vm_overlay(base_disk, overlay_meta1, overlay_meta2):
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
    unique_memory = len(memory_chunks1-memory_chunks2)
    unique_disk = len(disk_chunks1-disk_chunks2)
    print "unique to %s : (%ld+%ld)" % (overlay_meta1, unique_memory, unique_disk)

    deltalist1, deltadict1 = get_indexed_delta_list(base_disk, overlay_meta1)
    deltalist2, deltadict2 = get_indexed_delta_list(base_disk, overlay_meta2)
    hash_dict = dict()
    for item in deltalist2:
        hash_dict[item.hash_value] = item

    count_total = len(deltadict1)
    count_changed = 0
    count_identical = 0
    for (index, delta_item) in deltadict1.iteritems():
        item2 = deltadict2.get(index, None) 
        dup_found = hash_dict.get(delta_item.hash_value, None)
        if dup_found != None:
            count_identical += 1
        else:
            count_changed += 1

    print "# of deduplicated    : %ld (%4f %%)" % \
            (count_identical, 100.0*count_identical/count_total)
    print "# of changed         : %ld (%4f %%)" % \
            (count_changed, 100.0*count_changed/count_total)

    '''
    ref_id = 0x70
    delta.diff_with_deltalist(deltalist1, deltalist2, ref_id)
    for item in deltalist1:
        if (item.ref_id == ref_id):
            count_duplicated += 1
    '''


def process_command_line(argv):
    VERSION = '%s' % Const.VERSION
    DESCRIPTION = 'Compare VM overlays'

    parser = OptionParser(usage='%prog VM_overlay1 VM_overlay2', 
            version=VERSION, description=DESCRIPTION)
    settings, args = parser.parse_args(argv)

    return settings


def main(argv):
    #settings = process_command_line(sys.argv[1:])
    if len(sys.argv) != 4:
        sys.stderr.write('usage : %prog basepath VM_overlay1 VM_overlay2\n')
        sys.exit(1)

    # sanity check
    base_path = sys.argv[1]
    if os.path.exists(base_path) == False:
        sys.stderr.write("not a valid file at %s" % basepath)
        sys.exit(1)
    vm_overlay1 = sys.argv[2]
    if os.path.exists(vm_overlay1) == False:
        sys.stderr.write("not a valid file at %s" % vm_overlay1)
        sys.exit(1)
    vm_overlay2 = sys.argv[3]
    if os.path.exists(vm_overlay2) == False:
        sys.stderr.write("not a valid file at %s" % vm_overlay2)
        sys.exit(1)

    compare_vm_overlay(base_path, vm_overlay1, vm_overlay2)
    return 0


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
