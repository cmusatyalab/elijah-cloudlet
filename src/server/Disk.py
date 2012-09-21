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

import struct
import os
import sys
import mmap
import tool
import delta
from delta import DeltaItem
from delta import DeltaList
from progressbar import AnimatedProgressBar
from hashlib import sha256
from operator import itemgetter

class DiskError(Exception):
    pass

def hashing(disk_path, meta_path, chunk_size=4096, window_size=512, print_out=None):
    # TODO: need more efficient implementation, e.g. bisect
    # generate hash of base disk
    # disk_path : raw disk path
    # chunk_size : hash chunk size
    # window_size : slicing window size
    # print_out : progress bar

    if print_out:
        print_out.write("[INFO] Start VM Disk hashing\n")
        prog_bar = AnimatedProgressBar(end=100, width=80, stdout=print_out)
        total_iteration = os.path.getsize(disk_path)/window_size
        iter_count = 0
        prog_interval = 100

    disk_file = open(disk_path, "rb")
    out_file = open(meta_path, "w+b")
    data = disk_file.read(chunk_size)
    if (not data) or len(data) < chunk_size:
        raise DiskError("invalid raw disk size")

    s_offset = 0
    data_len = len(data)
    hash_dic = dict()
    while True:
        if print_out:
            if (iter_count)%prog_interval == 0:
                prog_bar.process(100.0*prog_interval/total_iteration)
                prog_bar.show_progress()
            iter_count += 1

        hashed_data = sha256(data).digest()
        if hash_dic.get(hashed_data) == None:
            hash_dic[hashed_data]= (hashed_data, s_offset, data_len)

        added_data = disk_file.read(window_size)
        if (not added_data) or len(added_data) != window_size:
            print ""
            break
        s_offset += window_size
        data = data[window_size:] + added_data

    for hashed_data, s_offset, data_len in list(hash_dic.values()):
        out_file.write(struct.pack("!QI%ds" % len(hashed_data), 
            s_offset, data_len, hashed_data))
    disk_file.close()
    out_file.close()


def _pack_hashlist(hash_list):
    # pack hash list
    original_length = len(hash_list)
    hash_list = dict((x[0], x) for x in hash_list).values()
    print "[Debug] hashlist is packed: from %d to %d : %lf" % \
            (original_length, len(hash_list), 1.0*len(hash_list)/original_length)


def create_memory_overlay(modified_disk, overlay_diskpath, 
            modified_chunk_list, chunk_size,
            basedisk_hashlist=None, basedisk_path=None,
            basemem_hashlist=None, basemem_path=None,
            print_out=None):
    # get disk delta
    # base_diskmeta : hash list of base disk
    # base_disk: path to base VM disk
    # modified_disk_path : path to modified VM disk
    # modified_chunk_list : chunk list of modified
    # overlay_path : path to destination of overlay disk
    base_fd = open(basedisk_path, "rb")
    base_mmap = mmap.mmap(base_fd.fileno(), 0, prot=mmap.PROT_READ)
    modified_fd = open(modified_disk, "rb")

    # 1. get modified page
    print_out.write("[Debug] 1.get modified disk page\n")
    delta_list = list()
    for index, chunk in enumerate(modified_chunk_list):
        offset = int(chunk) * chunk_size
        modified_fd.seek(offset)
        data = modified_fd.read(chunk_size)
        source_data = base_mmap[offset:offset+chunk_size]

        try:
            patch = tool.diff_data(source_data, data, 2*len(source_data))
            if len(patch) < len(data):
                delta_item = DeltaItem(offset, chunk_size, 
                        hash_value=sha256(data).digest(),
                        ref_id=DeltaItem.REF_XDELTA,
                        data_len=len(patch),
                        data=patch)
            else:
                raise IOError("xdelta3 patch is bigger than origianl")
        except IOError as e:
            #print "[INFO] xdelta failed, so save it as raw (%s)" % str(e)
            delta_item = DeltaItem(offset, chunk_size, 
                    hash_value=sha256(data).digest(),
                    ref_id=DeltaItem.REF_RAW,
                    data_len=len(data),
                    data=data)
        delta_list.append(delta_item)

    # 2.find shared with base memory 
    print_out.write("[Debug] 2-1.get delta from base Disk \n")
    delta.diff_with_hashlist(basedisk_hashlist, delta_list, ref_id=DeltaItem.REF_BASE_DISK)
    print_out.write("[Debug] 2-2.get delta from base memory\n")
    delta.diff_with_hashlist(basemem_hashlist, delta_list, ref_id=DeltaItem.REF_BASE_MEM)

    # 3.find shared within self
    print_out.write("[Debug] 3.get delta from itself\n")
    DeltaList.get_self_delta(delta_list)

    # 4. statistics
    DeltaList.statistics(delta_list, print_out)
    DeltaList.tofile(delta_list, overlay_diskpath)


def recover_disk(base_disk, base_mem, overlay_disk, recover_path, chunk_size):
    recover_fd = open(recover_path, "wb")

    # get delta list from file and recover it to origin
    delta_list = DeltaList.fromfile(overlay_disk)
    delta.recover_delta_list(delta_list, base_disk, base_mem, chunk_size, parent=base_disk)

    # overlay map
    chunk_list = []
    # sort delta list using offset
    delta_list.sort(key=itemgetter('offset'))
    for delta_item in delta_list:
        if len(delta_item.data) != chunk_size:
            raise DiskError("recovered size is not same as page size")
        chunk_list.append("%ld:1" % (delta_item.offset/chunk_size))
        recover_fd.seek(delta_item.offset)
        recover_fd.write(delta_item.data)

    # overlay chunk format: chunk_1:1,chunk_2:1,...
    return ','.join(chunk_list)


def base_hashlist(base_meta):
    hash_list = list()
    fd = open(base_meta, "rb")
    while True:
        header = fd.read(8+4)
        if not header:
            break
        offset, length = struct.unpack("!QI", header)
        sha256 = fd.read(32)
        hash_list.append((offset, length, sha256))
    return hash_list

