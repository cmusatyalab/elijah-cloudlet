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
from math import ceil
from delta import DeltaItem
from delta import DeltaList
from delta import Recovered_delta
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


def parse_qemu_log(qemu_logfile, chunk_size):
    # return dma_dict, discard_dict
    # element of dictionary has (chunk_%:discarded_time) format
    # CAVEAT: DMA Memory Address should be sift 4096*2 bytes because 
    # of libvirt(4096) and KVM(4096) header offset
    MEM_SIFT_OFFSET = 4096+4096
    if (qemu_logfile == None) or (not os.path.exists(qemu_logfile)):
        return dict(), dict()

    discard_dict = dict()
    dma_dict = dict()
    lines = open(qemu_logfile, "r").read().split("\n")
    discard_counter = 0
    dma_counter = 0
    mal_aligned_sector = 0
    for line in lines:
        if not line:
            break
        splits = line.split(",")
        event_time = float(splits[0].strip().split(":")[-1])
        header = splits[1].strip()
        data = splits[2:]
        if header == 'dma':
            mem_addr = long(data[0].split(":")[-1])
            sec_num = long(data[1].split(":")[-1])
            sec_len = long(data[2].split(":")[-1])
            from_disk = long(data[3].split(":")[-1])
            mem_chunk = (mem_addr+MEM_SIFT_OFFSET)/chunk_size
            disk_chunk = sec_num*512.0/chunk_size
            if sec_len != chunk_size:
                msg = "DMA sector length(%d) is not same as chunk size(%d)" % (sec_len, chunk_size)
                raise DiskError(msg)
            if sec_num%8 == 0:
                dma_dict[disk_chunk] = {'time':event_time, 'mem_chunk':mem_chunk, 'read':(True if from_disk else False)}
                dma_counter += 1
            else:
                if sec_num != -1:
                    mal_aligned_sector += 0
        elif header == 'bdrv_discard':
            start_sec_num = long(data[0].split(":")[-1])
            total_sec_len = long(data[1].split(":")[-1])
            start_chunk_num = start_sec_num*512.0/chunk_size
            end_chunk_num = (start_sec_num*512 + total_sec_len*512)/chunk_size
            if (start_sec_num*512)%chunk_size != 0:
                pass
                #print "Warning, disk sector is not aligned with chunksize"

            start_chunk_num = int(ceil(start_chunk_num))
            for chunk_num in xrange(start_chunk_num, end_chunk_num):
                discard_dict[chunk_num] = event_time
                discard_counter += 1

    if dma_counter != 0 :
        print "[DEBUG] net DMA ratio : %ld/%ld = %f %%" % (len(dma_dict), dma_counter, 100.0*len(dma_dict)/dma_counter)
    if discard_counter != 0:
        print "[DEBUG] net discard ratio : %ld/%ld = %f %%" % (len(discard_dict), discard_counter, 100.0*len(discard_dict)/discard_counter)
    if mal_aligned_sector != 0:
        print "Warning, mal-alignedsector count: %d" % (mal_aligned_sector)
    return dma_dict, discard_dict


def create_disk_deltalist(modified_disk, 
            modified_chunk_dict, chunk_size,
            basedisk_hashlist=None, basedisk_path=None,
            trim_dict=None, dma_dict=None,
            used_blocks_dict=None,
            ret_statistics=None,
            print_out=None):
    # get disk delta
    # base_diskmeta : hash list of base disk
    # base_disk: path to base VM disk
    # modified_disk_path : path to modified VM disk
    # modified_chunk_dict : chunk dict of modified
    # overlay_path : path to destination of overlay disk
    # dma_dict : dma information, 
    #           dma_dict[disk_chunk] = {'time':time, 'memory_chunk':memory chunk number, 'read': True if read from disk'}
    base_fd = open(basedisk_path, "rb")
    base_mmap = mmap.mmap(base_fd.fileno(), 0, prot=mmap.PROT_READ)
    modified_fd = open(modified_disk, "rb")

    # 0. get info from qemu log file
    # dictionary : (chunk_%, discarded_time)
    trim_counter = 0
    xray_counter = 0

    # TO BE DELETED
    trimed_list = []
    xrayed_list = []


    # 1. get modified page
    print_out.write("[Debug] 1.get modified disk page\n")
    delta_list = list()
    for index, chunk in enumerate(modified_chunk_dict.keys()):
        offset = chunk * chunk_size
        ctime = modified_chunk_dict[chunk]

        # check TRIM discard
        is_discarded = False
        if trim_dict:
            trim_time = trim_dict.get(chunk, None)
            if trim_time:
                if (trim_time > ctime):
                    trimed_list.append(chunk)
                    trim_counter += 1
                    is_discarded = True
        # check xray discard
        if used_blocks_dict:
            start_sector = offset/512
            if used_blocks_dict.get(start_sector) != True:
                xrayed_list.append(chunk)
                xray_counter +=1
                is_discarded = True

        if is_discarded == True:
            continue

        # check file system 
        modified_fd.seek(offset)
        data = modified_fd.read(chunk_size)
        source_data = base_mmap[offset:offset+len(data)]
        try:
            patch = tool.diff_data(source_data, data, 2*len(source_data))
            if len(patch) < len(data):
                delta_item = DeltaItem(DeltaItem.DELTA_DISK,
                        offset, len(data),
                        hash_value=sha256(data).digest(),
                        ref_id=DeltaItem.REF_XDELTA,
                        data_len=len(patch),
                        data=patch)
            else:
                raise IOError("xdelta3 patch is bigger than origianl")
        except IOError as e:
            #print "[INFO] xdelta failed, so save it as raw (%s)" % str(e)
            delta_item = DeltaItem(DeltaItem.DELTA_DISK,
                    offset, len(data),
                    hash_value=sha256(data).digest(),
                    ref_id=DeltaItem.REF_RAW,
                    data_len=len(data),
                    data=data)
        delta_list.append(delta_item)
    if ret_statistics != None:
        ret_statistics['trimed'] = trim_counter
        ret_statistics['xrayed'] = xray_counter
        ret_statistics['trimed_list'] = trimed_list
        ret_statistics['xrayed_list'] = xrayed_list
    print_out.write("[Debug] 1-1. Trim(%d), Xray(%d)\n" % (trim_counter, xray_counter))

    return delta_list


def recover_disk(base_disk, base_mem, overlay_mem, overlay_disk, recover_path, chunk_size):
    recover_fd = open(recover_path, "wb")

    # get delta list from file and recover it to origin
    delta_stream = open(overlay_disk, "r")
    recovered_memory = Recovered_delta(base_disk, base_mem, chunk_size, \
            parent=base_disk, overlay_memory=overlay_mem)
    for delta_item in DeltaList.from_stream(delta_stream):
        recovered_memory.recover_item(delta_item)
    delta_list = recovered_memory.delta_list

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
        last_write_offset = delta_item.offset + len(delta_item.data)

    # fill zero to the end of the modified file
    if last_write_offset:
        diff_offset = os.path.getsize(base_disk) - last_write_offset
        if diff_offset > 0:
            recover_fd.seek(diff_offset-1, os.SEEK_CUR)
            recover_fd.write('0')
    recover_fd.close()

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


if __name__ == "__main__":
    parse_qemu_log("log", 4096)
