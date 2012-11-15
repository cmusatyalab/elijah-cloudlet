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
import struct
import mmap
import tool
import os
from Const import Const
from operator import itemgetter
from hashlib import sha256
import multiprocessing 
from lzma import LZMACompressor

class DeltaError(Exception):
    pass

class DeltaItem(object):
    # Don't change order or memory and disk
    # We're using this value to sort for self referencing
    DELTA_MEMORY        = 0x01
    DELTA_DISK          = 0x02

    REF_RAW             = 0x10
    REF_XDELTA          = 0x20
    REF_SELF            = 0x30
    REF_BASE_DISK       = 0x40
    REF_BASE_MEM        = 0x50
    REF_ZEROS           = 0x60

    # data exist only when ref_id is not xdelta
    def __init__(self, delta_type, offset, offset_len, hash_value, ref_id, data_len=0, data=None):
        self.delta_type = delta_type
        self.offset = long(offset)
        self.offset_len = offset_len
        self.hash_value = hash_value
        self.ref_id = ref_id
        self.data_len = data_len
        self.data = data

        # new field for identify delta_item
        # because offset is not unique once we use both memory and disk
        self.index = DeltaItem.get_index(self.delta_type, self.offset)

    @staticmethod
    def get_index(delta_type, offset):
        return long((offset << 1) | (delta_type & 0x0F))

    def __getitem__(self, item):
        return self.__dict__[item]

    def get_serialized(self):
        # offset        : unsigned long long
        # offset_length : unsigned short
        # ref_id        : unsigned char
        data = struct.pack("!QHc", \
                self.offset,
                self.offset_len,
                chr(self.delta_type | self.ref_id))

        if self.ref_id == DeltaItem.REF_RAW or \
               self.ref_id == DeltaItem.REF_XDELTA:
            data += struct.pack("!Q", self.data_len)
            data += struct.pack("!%ds" % self.data_len, self.data)
        elif self.ref_id == DeltaItem.REF_SELF:
            data += struct.pack("!Q", self.data)
        elif self.ref_id == DeltaItem.REF_BASE_DISK or \
                self.ref_id == DeltaItem.REF_BASE_MEM:
            data += struct.pack("!Q", self.data)
        return data

    @staticmethod
    def unpack_stream(stream):
        data = stream.read(8 + 2 + 1)
        data_len = 0
        if not data:
            return None

        (offset, offset_len, ref_info) = \
                struct.unpack("!QHc", data)
        ref_id = ord(ref_info) & 0xF0
        delta_type = ord(ref_info) & 0x0F

        if ref_id == DeltaItem.REF_RAW or \
                ref_id == DeltaItem.REF_XDELTA:
            data_len = struct.unpack("!Q", stream.read(8))[0]
            data = stream.read(data_len)
        elif ref_id == DeltaItem.REF_SELF:
            data = struct.unpack("!Q", stream.read(8))[0]
        elif ref_id == DeltaItem.REF_BASE_DISK or \
                ref_id == DeltaItem.REF_BASE_MEM:
            data = struct.unpack("!Q", stream.read(8))[0]

        # hash value does not exist when recovered
        item = DeltaItem(delta_type, offset, offset_len, None, ref_id, data_len, data)
        return item


class DeltaList(object):
    @staticmethod
    def tofile(delta_list, f_path):
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        fd = open(f_path, "wb")
        # Write list if delta item
        for item in delta_list:
            # save it as little endian format
            fd.write(item.get_serialized())
        fd.close()

    @staticmethod
    def fromfile(f_path):
        delta_list = []
        fd = open(f_path, "rb")
        while True:
            new_item = DeltaItem.unpack_stream(fd)
            if not new_item:
                break
            delta_list.append(new_item)
        fd.close()
        return delta_list 

    @staticmethod
    def from_stream(stream):
        while True:
            new_item = DeltaItem.unpack_stream(stream)
            if new_item == None:
                raise StopIteration()
            yield new_item

    @staticmethod
    def from_chunk(in_queue):
        while True:
            new_item = DeltaItem.unpack_stream(stream)
            if new_item == None:
                raise StopIteration()
            yield new_item

    @staticmethod
    def tofile_with_footer(footer_delta, delta_list, f_path):
        if not footer_delta:
            raise MemoryError("invalid footer delta")
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        fd = open(f_path, "wb")

        # Write Header & Footer delta
        fd.write(struct.pack("!q", len(footer_delta)))
        fd.write(footer_delta)

        # Write list if delta item
        for item in delta_list:
            # save it as little endian format
            fd.write(item.get_serialized())
        fd.close()

    @staticmethod
    def get_self_delta(delta_list):
        if len(delta_list) == 0:
            print "[Debug] Nothing to compare. Length is 0"
            delta_list.sort(key=itemgetter('offset'))
            return
        if type(delta_list[0]) != DeltaItem:
            raise DeltaError("Need list of DeltaItem")

        # delta_list : list of (type, start, end, ref_id, hash/data)
        # sort by (hash/start offset)
        delta_list.sort(key=itemgetter('hash_value', 'delta_type', 'offset'))

        pivot = delta_list[0]
        matching = 0
        for delta_item in delta_list[1:]:
            if delta_item.hash_value == pivot.hash_value:
                if delta_item.ref_id == DeltaItem.REF_XDELTA or delta_item.ref_id == DeltaItem.REF_RAW:
                    # same data/hash
                    # save reference start offset
                    delta_item.ref_id = DeltaItem.REF_SELF
                    delta_item.data_len = 8
                    delta_item.data = long(pivot.index)
                    matching += 1
                    '''
                    print "type:%ld, offset:%ld, index:%ld <- %ld, %ld, %ld" % \
                            (pivot.delta_type, pivot.offset, pivot.index,
                            delta_item.delta_type, delta_item.offset, delta_item.index)
                    '''
            else:
                # change pivot only when no duplicated hash value
                pivot=delta_item

        print "[Debug] self delta : %ld/%ld" % (matching, len(delta_list))

    @staticmethod
    def statistics(delta_list, print_out=sys.stdout, mem_discarded=0, disk_discarded=0):
        if len(delta_list) == 0:
            print "[Debug] Nothing to compare. Length is 0"
            delta_list.sort(key=itemgetter('offset'))
            return
        if type(delta_list[0]) != DeltaItem:
            raise DeltaError("Need list of DeltaItem")

        from_self = 0
        from_zeros = 0
        from_raw = 0
        from_base_disk = 0
        from_base_mem = 0
        from_xdelta = 0
        from_overlay_disk = 0
        from_overlay_mem = 0
        xdelta_size = 0
        raw_size = 0
        # to quickly find memory-disk dedup
        previous_delta_dict = dict() 

        for delta_item in delta_list:
            previous_delta_dict[delta_item.index] = delta_item
            if delta_item.ref_id == DeltaItem.REF_SELF:
                ref_index = delta_item.data
                ref_delta = previous_delta_dict.get(ref_index, None)
                if ref_delta == None:
                    raise DeltaError("Cannot calculate statistics for self_referencing")
                if delta_item.delta_type == DeltaItem.DELTA_DISK:
                    if (ref_delta.delta_type == DeltaItem.DELTA_MEMORY):
                        from_overlay_mem += 1
                    else:
                        from_self += 1
                elif delta_item.delta_type == DeltaItem.DELTA_MEMORY:
                    if (ref_delta.delta_type == DeltaItem.DELTA_DISK):
                        from_overlay_mem += 1
                    else:
                        from_self += 1
            elif delta_item.ref_id == DeltaItem.REF_ZEROS:
                from_zeros += 1
            elif delta_item.ref_id == DeltaItem.REF_BASE_DISK:
                from_base_disk += 1
            elif delta_item.ref_id == DeltaItem.REF_BASE_MEM:
                from_base_mem += 1
            elif delta_item.ref_id == DeltaItem.REF_XDELTA:
                from_xdelta += 1
                xdelta_size += len(delta_item.data)
            elif delta_item.ref_id == DeltaItem.REF_RAW:
                from_raw += 1
                raw_size += len(delta_item.data)

        chunk_size = delta_list[0].offset_len
        size_MB = chunk_size/1024.0
        discarded_num = mem_discarded+disk_discarded
        total_count= (len(delta_list)+discarded_num)/100.0

        print_out.write("-"*50 + "\n")
        print_out.write("[INFO] Total Modified page #\t: %ld\t( 100 %% )\n" % 
                (len(delta_list)+discarded_num))
        print_out.write("[INFO] TRIM discard\t\t: %ld\t( %f %% )\n" % 
                (disk_discarded, disk_discarded/total_count))
        print_out.write("[INFO] FREE discard\t\t: %ld\t( %f %% )\n" % 
                (mem_discarded, mem_discarded/total_count))
        print_out.write("[INFO] Zero pages\t\t: %ld\t( %f %% )\n" % 
                (from_zeros, from_zeros/total_count))
        print_out.write("[INFO] Shared with Base Disk\t: %ld\t( %f %% )\n" % 
                (from_base_disk, from_base_disk/total_count))
        print_out.write("[INFO] Shared with Base Mem\t: %ld\t( %f %% )\n" % 
                (from_base_mem, from_base_mem/total_count))
        print_out.write("[INFO] Shared within Self\t: %ld\t( %f %% )\n" % 
                (from_self, from_self/total_count))
        print_out.write("[INFO] Shared with Overlay Disk\t: %ld\t( %f %% )\n" % 
                (from_overlay_disk, from_overlay_disk/total_count))
        print_out.write("[INFO] Shared with Overlay Mem\t: %ld\t( %f %% )\n" % 
                (from_overlay_mem, from_overlay_mem/total_count))
        print_out.write("[INFO] xdelta\t\t\t: %ld\t( %f %%, real_size: %.0f KB )\n" %
                (from_xdelta, from_xdelta/total_count, xdelta_size))
        print_out.write("[INFO] raw\t\t\t: %ld\t( %f %%, real_size: %.0f KB )\n" % 
                (from_raw, from_raw/total_count, raw_size))
        print_out.write("-"*50 + "\n")


def diff_with_deltalist(delta_list, const_deltalist, ref_id):
    # update source_deltalist using const_deltalist
    # Example) source_deltalist: disk delta list,
    #       const_deltalist: memory delta list
    if len(delta_list) == 0:
        return delta_list
    if type(delta_list[0]) != DeltaItem:
        raise DeltaError("Need list of DeltaItem")
    if len(const_deltalist) == 0 or type(const_deltalist[0]) != DeltaItem:
        raise DeltaError("Need list of DeltaItem for const")

    delta_list.sort(key=itemgetter('hash_value')) # sort by hash value
    const_deltalist.sort(key=itemgetter('hash_value')) # sort by hash value

    matching_count = 0
    s_index = 0
    index = 0
    while index < len(const_deltalist) and s_index < len(delta_list):
        delta = delta_list[s_index]
        const_delta = const_deltalist[index]
        if const_delta.hash_value < delta.hash_value:
            index += 1
            continue

        # compare
        if delta.hash_value == const_delta.hash_value and \
                ((delta.ref_id == DeltaItem.REF_XDELTA) or (delta.ref_id == DeltaItem.REF_RAW)):
            if delta.offset_len != const_delta.offset_len:
                message = "Hash is same but length is different %d != %d" % \
                        (delta.offset_len, const_delta.offset_len)
                raise DeltaError(message)
            matching_count += 1
            delta.ref_id = ref_id
            delta.data_len = 8
            delta.data = long(const_delta.offset)
        s_index += 1
    return delta_list 


def diff_with_hashlist(base_hashlist, delta_list, ref_id):
    # update delta_list using base_hashlist
    if len(delta_list) == 0:
        return delta_list
    if type(delta_list[0]) != DeltaItem:
        raise DeltaError("Need list of DeltaItem")

    base_hashlist.sort(key=itemgetter(2)) # sort by hash value
    delta_list.sort(key=itemgetter('hash_value')) # sort by hash value

    matching_count = 0
    s_index = 0
    index = 0
    while index < len(base_hashlist) and s_index < len(delta_list):
        delta = delta_list[s_index]
        (start, length, hash_value) = base_hashlist[index]
        if hash_value < delta.hash_value:
            index += 1
            continue

        # compare
        if delta.hash_value == hash_value and \
                ((delta.ref_id == DeltaItem.REF_XDELTA) or (delta.ref_id == DeltaItem.REF_RAW)):
            matching_count += 1
            #print "[Debug] page %ld is matching base %ld" % (s_start, start)
            delta.ref_id = ref_id
            delta.data_len = 8
            delta.data = long(start)
        s_index += 1

    print "[Debug] matching (%d/%d) with base" % (matching_count, len(delta_list))
    return delta_list


class Recovered_delta(multiprocessing.Process):
    FUSE_INDEX_DISK = 1
    FUSE_INDEX_MEMORY = 2
    END_OF_PIPE = -133421

    def __init__(self, base_disk, base_mem, overlay_path, 
            output_mem_path, output_mem_size, 
            output_disk_path, output_disk_size, chunk_size,
            out_pipe=None, time_queue=None):
        # recover delta list using base disk/memory
        # You have to specify parent to indicate whether you're recover memory or disk 
        # optionally you can use overlay_memory to recover overlay disk which is
        # de-duplicated with overlay memory

        if base_disk == None and base_mem == None:
            raise MemoryError("Need either base_disk or base_memory")

        self.overlay_path = overlay_path
        self.output_mem_path = output_mem_path
        self.output_mem_size = output_mem_size
        self.output_disk_path = output_disk_path
        self.output_disk_size = output_disk_size
        self.out_pipe = out_pipe
        self.time_queue = time_queue
        self.base_disk = base_disk
        self.base_mem = base_mem

        self.base_disk_fd = None
        self.base_mem_fd = None
        self.raw_disk = None
        self.raw_mem = None
        self.mem_overlay_dict = None
        self.raw_mem_overlay = None
        self.chunk_size = chunk_size
        self.zero_data = struct.pack("!s", chr(0x00)) * chunk_size
        self.recovered_delta_dict = dict()
        self.delta_list = list()
        
        # initialize reference data to use mmap
        self.base_disk_fd = open(base_disk, "rb")
        self.raw_disk = mmap.mmap(self.base_disk_fd.fileno(), 0, prot=mmap.PROT_READ)
        self.base_mem_fd = open(base_mem, "rb")
        self.raw_mem = mmap.mmap(self.base_mem_fd.fileno(), 0, prot=mmap.PROT_READ)

        multiprocessing.Process.__init__(self)

    def run(self):
        start_time = time.time()
        count = 0
        self.recover_mem_fd = open(self.output_mem_path, "wrb")
        self.recover_disk_fd = open(self.output_disk_path, "wrb")
        overlay_stream = open(self.overlay_path, "r")

        overlay_chunk_ids = []
        for delta_item in DeltaList.from_stream(overlay_stream):
            self.recover_item(delta_item)
            if len(delta_item.data) != delta_item.offset_len:
                msg = "recovered size is not same as page size, %ld != %ld" % \
                        (len(delta_item.data), self.chunk_size)
                raise DeltaError(msg)

            # save it to dictionary to find self_reference easily
            #print "recovred_delta_dict[%ld] exists" % (delta_item.index)
            self.recovered_delta_dict[delta_item.index] = delta_item
            self.delta_list.append(delta_item)

            # write to output file 
            overlay_chunk_id = long(delta_item.offset/self.chunk_size)
            if delta_item.delta_type == DeltaItem.DELTA_MEMORY:
                self.recover_mem_fd.seek(delta_item.offset)
                self.recover_mem_fd.write(delta_item.data)
                overlay_chunk_ids.append("%d:%ld" % 
                        (Recovered_delta.FUSE_INDEX_MEMORY, overlay_chunk_id))
            elif delta_item.delta_type == DeltaItem.DELTA_DISK:
                self.recover_disk_fd.seek(delta_item.offset)
                self.recover_disk_fd.write(delta_item.data)
                overlay_chunk_ids.append("%d:%ld" % 
                        (Recovered_delta.FUSE_INDEX_DISK, overlay_chunk_id))

            if len(overlay_chunk_ids) % 1000 == 0:
                self.recover_mem_fd.flush()
                self.recover_disk_fd.flush()
                self.out_pipe.send(overlay_chunk_ids)
                count += len(overlay_chunk_ids)
                overlay_chunk_ids[:] = []

        if len(overlay_chunk_ids) > 0:
            self.out_pipe.send(overlay_chunk_ids)
            count += len(overlay_chunk_ids)

        self.out_pipe.send(Recovered_delta.END_OF_PIPE)
        self.out_pipe.close()
        self.recover_mem_fd.close()
        self.recover_disk_fd.close()
        end_time = time.time()

        if self.time_queue != None: 
            self.time_queue.put({'start_time':start_time, 'end_time':end_time})
        print "[Delta] : (%s)-(%s)=(%s), delta %ld chunks" % \
                (start_time, end_time, (end_time-start_time), count)

    def recover_item(self, delta_item):
        if type(delta_item) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        #print "recovering %ld/%ld" % (index, len(delta_list))
        if (delta_item.ref_id == DeltaItem.REF_RAW):
            recover_data = delta_item.data
        elif (delta_item.ref_id == DeltaItem.REF_ZEROS):
            recover_data = self.zero_data
        elif (delta_item.ref_id == DeltaItem.REF_BASE_MEM):
            offset = delta_item.data
            recover_data = self.raw_mem[offset:offset+self.chunk_size]
        elif (delta_item.ref_id == DeltaItem.REF_BASE_DISK):
            offset = delta_item.data
            recover_data = self.raw_disk[offset:offset+self.chunk_size]
        elif delta_item.ref_id == DeltaItem.REF_SELF:
            ref_index = delta_item.data
            self_ref_delta_item = self.recovered_delta_dict.get(ref_index, None)
            if self_ref_delta_item == None:
                msg = "Cannot find self reference: type(%ld), offset(%ld), index(%ld), ref_index(%ld)" % \
                        (delta_item.delta_type, delta_item.offset, delta_item.index, ref_index)
                raise MemoryError(msg)
            recover_data = self_ref_delta_item.data
        elif delta_item.ref_id == DeltaItem.REF_XDELTA:
            patch_data = delta_item.data
            patch_original_size = delta_item.offset_len
            if delta_item.delta_type == DeltaItem.DELTA_MEMORY:
                base_data = self.raw_mem[delta_item.offset:delta_item.offset+patch_original_size]
            elif delta_item.delta_type == DeltaItem.DELTA_DISK:
                base_data = self.raw_disk[delta_item.offset:delta_item.offset+patch_original_size]
            else:
                raise DeltaError("Delta type should be either disk or memory")
            recover_data = tool.merge_data(base_data, patch_data, len(base_data)*5)
        else:
            raise MemoryError("Cannot recover: invalid referce id %d" % delta_item.ref_id)

        if len(recover_data) != delta_item.offset_len:
            msg = "Recovered Size Error: %d, ref_id: %s, %ld %ld" % \
                    (len(recover_data), delta_item.ref_id, \
                    delta_item.data_len, delta_item.offset)
            raise MemoryError(msg)

        # recover
        delta_item.ref_id = DeltaItem.REF_RAW
        delta_item.data = recover_data

        return delta_item

    def finish(self):
        if self.base_disk_fd:
            self.base_disk_fd.close()
        if self.base_mem_fd:
            self.base_mem_fd.close()
        if self.raw_disk:
            self.raw_disk.close()
        if self.raw_mem:
            self.raw_mem.close()
        if self.raw_mem_overlay:
            self.raw_mem_overlay.close()
        print "[DEBUG] Recover finishes"


def create_overlay(memory_deltalist, memory_chunk_size,
        disk_deltalist, disk_chunk_size,
        basedisk_hashlist=None, basemem_hashlist=None, 
        print_out=sys.stdout):

    if memory_chunk_size != disk_chunk_size:
        raise DeltaError("Expect same chunk size for Disk and Memory")
    chunk_size = disk_chunk_size
    delta_list = memory_deltalist+disk_deltalist

    #Memory
    # Create Base Memory from meta file
    print_out.write("[Debug] 2-1.Find zero page\n")
    zero_hash = sha256(struct.pack("!s", chr(0x00))*chunk_size).digest()
    zero_hash_list = [(-1, chunk_size, zero_hash)]
    diff_with_hashlist(zero_hash_list, delta_list, ref_id=DeltaItem.REF_ZEROS)

    print_out.write("[Debug] 2-2.get delta from base Memory\n")
    diff_with_hashlist(basemem_hashlist, delta_list, ref_id=DeltaItem.REF_BASE_MEM)
    print_out.write("[Debug] 2-3.get delta from base Disk\n")
    diff_with_hashlist(basedisk_hashlist, delta_list, ref_id=DeltaItem.REF_BASE_DISK)

    # 3.find shared within self
    print_out.write("[Debug] 3.get delta from itself\n")
    DeltaList.get_self_delta(delta_list)

    return delta_list

def reorder_deltalist_linear(chunk_size, delta_list):
    if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
        raise MemoryError("Need list of DeltaItem")
    start_time = time.time()
    delta_dict = dict()
    for item in delta_list:
        delta_dict[item.index] = item

    delta_list.sort(key=itemgetter('delta_type', 'offset'))
    for index, delta_item in enumerate(delta_list):
        if delta_item.ref_id == DeltaItem.REF_SELF:
            ref_index = long(delta_item.data)
            ref_item = delta_dict.get(ref_index, None)
            ref_pos = delta_list.index(ref_item)
            if ref_pos > index:
                print "[Debug][REORDER] move reference from %d to %d" % (ref_pos, (index-1))
                delta_list.remove(ref_item)
                delta_list.insert(index, ref_item)
    print "[Debug][REORDER] reordering takes : %f" % (time.time()-start_time)

def reorder_deltalist(mem_access_file, chunk_size, delta_list):
    # chunks that appear earlier in access file comes afront in deltalist
    if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
        raise MemoryError("Need list of DeltaItem")

    start_time = time.time()
    access_list = open(mem_access_file, "r").read().split()
    if len(access_list[-1].strip()) == 0:
        access_list = access_list[:-1]

    delta_dict = dict()
    for item in delta_list:
        delta_dict[item.index] = item

    access_list.reverse()
    before_length = len(delta_list)
    count = 0 
    for chunk_number in access_list:
        chunk_index = DeltaItem.get_index(DeltaItem.DELTA_MEMORY, long(chunk_number)*chunk_size)
        delta_item = delta_dict.get(chunk_index, None)
        #print "%ld in access list" % (long(chunk_number))
        if delta_item:
            #print "chunk(%ld) moved from %d --> 0" % (delta_item.offset/chunk_size, delta_list.index(delta_item))
            delta_list.remove(delta_item)
            delta_list.insert(0, delta_item)
            count += 1

            # moved item has reference
            if delta_item.ref_id == DeltaItem.REF_SELF:
                ref_index = delta_item.data
                ref_delta = delta_dict[ref_index]
                delta_list.remove(ref_delta)
                delta_list.insert(0, ref_delta)
                #print "chunk(%ld) moving because its reference of chunk(%ld)" % \
                #        (ref_delta.offset/chunk_size, delta_item.offset/chunk_size)
    after_length = len(delta_list)
    if before_length != after_length:
        raise DeltaError("DeltaList size shouldn't be changed after reordering")

    delta_dict_new = dict()
    for item in delta_list:
        delta_dict_new[item.index] = item

    prev_indexes = delta_dict.keys().sort()
    new_indexes = delta_dict_new.keys().sort()
    if prev_indexes != new_indexes:
        print "Reordered delta list is not same as previous"
        sys.exit(1)

    end_time = time.time()
    print "[DEBUG][REORDER] time %f" % (end_time-start_time)
    print "[DEBUG][REORDER] changed %d deltaitem (total access pattern: %d)" % (count, len(access_list))


def _save_blob(start_index, delta_list, self_ref_dict, blob_name, blob_size, statistics=None):
    # mode = 2 indicates LZMA_SYNC_FLUSH, which show all output right after input
    comp_option = {'format':'xz', 'level':9}
    comp = LZMACompressor(options=comp_option)
    disk_offset_list = list()
    memory_offset_list= list()
    comp_data = ''
    original_length = 0
    index = start_index
    item_count = 0
    
    while index < len(delta_list):
        delta_item = delta_list[index]

        if delta_item.ref_id != DeltaItem.REF_SELF:
            delta_bytes = delta_item.get_serialized()
            original_length += len(delta_bytes)
            comp_data += comp.compress(delta_bytes)
            item_count += 1
            if delta_item.delta_type == DeltaItem.DELTA_MEMORY:
                memory_offset_list.append(delta_item.offset)
            elif delta_item.delta_type == DeltaItem.DELTA_DISK:
                disk_offset_list.append(delta_item.offset)
            else:
                raise DeltaError("Delta should be either memory or disk")

            # remove dependece getting required index by finding reference
            deduped_list = self_ref_dict.get(delta_item.index, None)
            if deduped_list != None:
                #print "moving %d deduped delta item" % (len(deduped_list))
                for deduped_item in deduped_list:
                    deduped_bytes = deduped_item.get_serialized()
                    original_length += len(deduped_bytes)
                    comp_data += comp.compress(deduped_bytes)
                    item_count += 1
                    if deduped_item.delta_type == DeltaItem.DELTA_MEMORY:
                        memory_offset_list.append(deduped_item.offset)
                    elif deduped_item.delta_type == DeltaItem.DELTA_DISK:
                        disk_offset_list.append(deduped_item.offset)
                    else:
                        raise DeltaError("Delta should be either memory or disk")
            
        if len(comp_data) >= blob_size:
            print "savefile for %s(%ld delta item) %ld --> %ld" % \
                    (blob_name, item_count, original_length, len(comp_data))
            comp_data += comp.flush()
            blob_file = open(blob_name, "w+b")
            blob_file.write(comp_data)
            blob_file.close()
            if statistics != None:
                statistics['item_count'] = item_count
            return index, memory_offset_list, disk_offset_list
        index += 1

    comp_data += comp.flush()
    if len(comp_data) > 0 :
        print "savefile for %s(%ld delta item) %ld --> %ld" % \
                (blob_name, item_count, original_length, len(comp_data))
        blob_file = open(blob_name, "w+b")
        blob_file.write(comp_data)
        blob_file.close()
        if statistics != None:
            statistics['item_count'] = item_count
        return index, memory_offset_list, disk_offset_list
    else:
        raise DeltaError("LZMA compression is zero")


def divide_blobs(delta_list, overlay_path, blob_size_kb, 
        disk_chunk_size, memory_chunk_size,
        print_out=sys.stdout):
    # save delta list into multiple files with LZMA compression
    start_time = time.time()

    # build reference table
    self_ref_dict = dict()
    for delta_item in delta_list:
        if delta_item.ref_id == DeltaItem.REF_SELF:
            ref_index = delta_item.data
            if self_ref_dict.get(ref_index, None) == None:
                self_ref_dict[ref_index] = list()
            self_ref_dict[ref_index].append(delta_item)

    blob_size = blob_size_kb*1024
    blob_number = 1
    overlay_list = list()
    statistics = dict()
    index = 0
    comp_counter = 0
    while index < len(delta_list):
        blob_name = "%s_%d.xz" % (overlay_path, blob_number)
        end_index, memory_offsets, disk_offsets = \
                _save_blob(index, delta_list, self_ref_dict, blob_name, blob_size, statistics)
        index = (end_index+1)
        blob_number += 1
        if statistics.get('item_count', None) != None:
            comp_counter += statistics.get('item_count')

        memory_chunks = [offset/memory_chunk_size for offset in memory_offsets]
        disk_chunks = [offset/disk_chunk_size for offset in disk_offsets]
        file_size = os.path.getsize(blob_name)
        blob_dict = {
            Const.META_OVERLAY_FILE_NAME:os.path.basename(blob_name),
            Const.META_OVERLAY_FILE_SIZE:file_size,
            Const.META_OVERLAY_FILE_DISK_CHUNKS: disk_chunks,
            Const.META_OVERLAY_FILE_MEMORY_CHUNKS: memory_chunks
            }
        overlay_list.append(blob_dict)
    end_time = time.time()
    print_out.write("[Debug] Overlay Compression time: %f, delta_item: %ld\n" % 
            ((end_time-start_time), comp_counter))
    return overlay_list 


