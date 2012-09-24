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
import struct
import mmap
import tool
from operator import itemgetter


DELTA_FILE_MAGIC = 0x1145511b
DELTA_FILE_VERSION = 0x00000001


class DeltaError(Exception):
    pass

class DeltaItem(object):
    REF_RAW = 0x00
    REF_XDELTA = 0x01
    REF_SELF = 0x02
    REF_BASE_DISK = 0x03
    REF_BASE_MEM = 0x04
    REF_OVERLAY_DISK = 0x05
    REF_OVERLAY_MEM = 0x06
    REF_ZEROS = 0x07

    # data exist only when ref_id is not xdelta
    def __init__(self, offset, offset_len, hash_value, ref_id, data_len=0, data=None):
        self.offset = offset
        self.offset_len = offset_len
        self.hash_value = hash_value
        self.ref_id = ref_id
        self.data_len = data_len
        self.data = data

    def __getitem__(self, item):
        return self.__dict__[item]

    def get_serialized(self):
        # offset        : unsigned long long
        # offset_length : unsigned int
        # ref_id        : unsigned char
        data = struct.pack("!QIc", \
                self.offset,
                self.offset_len,
                chr(self.ref_id))

        if self.ref_id == DeltaItem.REF_RAW or \
               self.ref_id == DeltaItem.REF_XDELTA:
            data += struct.pack("!Q", self.data_len)
            data += struct.pack("!%ds" % self.data_len, self.data)
        elif self.ref_id == DeltaItem.REF_SELF:
            data += struct.pack("!Q", self.data)
        elif self.ref_id == DeltaItem.REF_BASE_DISK or \
                self.ref_id == DeltaItem.REF_BASE_MEM or \
                self.ref_id == DeltaItem.REF_OVERLAY_DISK or \
                self.ref_id == DeltaItem.REF_OVERLAY_MEM:
            data += struct.pack("!Q", self.data)
        return data

    @staticmethod
    def unpack_stream(stream):
        data = stream.read(8 + 4 + 1)
        data_len = 0
        if not data:
            return None

        (offset, offset_len, ref_id) = \
                struct.unpack("!QIc", data)
        ref_id = ord(ref_id)
        if ref_id == DeltaItem.REF_RAW or \
                ref_id == DeltaItem.REF_XDELTA:
            data_len = struct.unpack("!Q", stream.read(8))[0]
            data = stream.read(data_len)
        elif ref_id == DeltaItem.REF_SELF:
            data = struct.unpack("!Q", stream.read(8))[0]
        elif ref_id == DeltaItem.REF_BASE_DISK or \
                ref_id == DeltaItem.REF_BASE_MEM or \
                ref_id == DeltaItem.REF_OVERLAY_DISK or \
                ref_id == DeltaItem.REF_OVERLAY_MEM:
            data = struct.unpack("!Q", stream.read(8))[0]

        # hash value does not exist when recovered
        item = DeltaItem(offset, offset_len, None, ref_id, data_len, data)
        return item


class DeltaList(object):
    @staticmethod
    def tofile(delta_list, f_path):
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        fd = open(f_path, "wb")
        # Write MAGIC & VERSION
        fd.write(struct.pack("!q", DELTA_FILE_MAGIC))
        fd.write(struct.pack("!q", DELTA_FILE_VERSION))

        # Write list if delta item
        for item in delta_list:
            # save it as little endian format
            fd.write(item.get_serialized())
        fd.close()

    @staticmethod
    def fromfile(f_path):
        delta_list = []
        # MAGIC & VERSION
        fd = open(f_path, "rb")
        magic, version = struct.unpack("!qq", fd.read(8+8))
        if magic != DELTA_FILE_MAGIC or version != DELTA_FILE_VERSION:
            msg = "delta magic number(%x != %x), version(%ld != %ld) does not match" \
                    % (DELTA_FILE_MAGIC, magic, \
                    DELTA_FILE_VERSION, version)
            raise IOError(msg)

        while True:
            new_item = DeltaItem.unpack_stream(fd)
            if not new_item:
                break
            delta_list.append(new_item)
        fd.close()
        return delta_list 


    @staticmethod
    def tofile_with_footer(footer_delta, delta_list, f_path):
        if not footer_delta:
            raise MemoryError("invalid footer delta")
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        fd = open(f_path, "wb")
        # Write MAGIC & VERSION
        fd.write(struct.pack("!q", DELTA_FILE_MAGIC))
        fd.write(struct.pack("!q", DELTA_FILE_VERSION))

        # Write Header & Footer delta
        fd.write(struct.pack("!q", len(footer_delta)))
        fd.write(footer_delta)

        # Write list if delta item
        for item in delta_list:
            # save it as little endian format
            fd.write(item.get_serialized())
        fd.close()

    @staticmethod
    def fromfile_with_footer(f_path):
        delta_list = []
        # MAGIC & VERSION
        fd = open(f_path, "rb")
        magic, version = struct.unpack("!qq", fd.read(8+8))
        if magic != DELTA_FILE_MAGIC or version != DELTA_FILE_VERSION:
            msg = "delta magic number(%x != %x), version(%ld != %ld) does not match" \
                    % (DELTA_FILE_MAGIC, magic, \
                    DELTA_FILE_VERSION, version)
            raise IOError(msg)

        # Read Footer delta
        footer_len = struct.unpack("!q", fd.read(8))[0]
        footer_delta = fd.read(footer_len)
        while True:
            new_item = DeltaItem.unpack_stream(fd)
            if not new_item:
                break
            delta_list.append(new_item)
        fd.close()
        return footer_delta, delta_list 

    @staticmethod
    def get_self_delta(delta_list):
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        # delta_list : list of (start, end, ref_id, hash/data)
        delta_list.sort(key=itemgetter('hash_value', 'offset')) # sort by (hash/start offset)

        pivot = delta_list[0]
        matching = 0
        for delta_item in delta_list[1:]:
            if delta_item.hash_value == pivot.hash_value:
                if delta_item.ref_id == DeltaItem.REF_XDELTA or delta_item.ref_id == DeltaItem.REF_RAW:
                    # same data/hash
                    # save reference start offset
                    delta_item.ref_id = DeltaItem.REF_SELF
                    delta_item.data_len = 8
                    delta_item.data = long(pivot.offset)
                    matching += 1
                    continue
            pivot=delta_item

        print "[Debug] self delta : %ld/%ld" % (matching, len(delta_list))
        delta_list.sort(key=itemgetter('offset'))

    @staticmethod
    def statistics(delta_list, print_out=sys.stdout):
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        from_self = 0
        from_zeros = 0
        from_raw = 0
        from_base_disk = 0
        from_base_mem = 0
        from_xdelta = 0
        from_overlay_disk = 0
        from_overlay_mem = 0
        for delta_item in delta_list:
            if delta_item.ref_id == DeltaItem.REF_SELF:
                from_self += 1
            elif delta_item.ref_id == DeltaItem.REF_ZEROS:
                from_zeros += 1
            elif delta_item.ref_id == DeltaItem.REF_BASE_DISK:
                from_base_disk += 1
            elif delta_item.ref_id == DeltaItem.REF_BASE_MEM:
                from_base_mem += 1
            elif delta_item.ref_id == DeltaItem.REF_OVERLAY_DISK:
                from_overlay_disk += 1
            elif delta_item.ref_id == DeltaItem.REF_OVERLAY_MEM:
                from_overlay_mem += 1
            elif delta_item.ref_id == DeltaItem.REF_XDELTA:
                from_xdelta += 1
            elif delta_item.ref_id == DeltaItem.REF_RAW:
                from_raw += 1
        print_out.write("-"*80 + "\n")
        print_out.write("[INFO] Total Modified page #\t:%ld\n" % len(delta_list))
        print_out.write("[INFO] Zero pages\t\t:%ld\n" % from_zeros)
        print_out.write("[INFO] Shared with Base Disk\t:%ld\n" % from_base_disk)
        print_out.write("[INFO] Shared with Base Mem\t:%ld\n" % from_base_mem)
        print_out.write("[INFO] Shared within Self\t:%ld\n" % from_self)
        print_out.write("[INFO] Shared with Overlay Disk\t:%ld\n" % from_overlay_disk)
        print_out.write("[INFO] Shared with Overlay Mem\t:%ld\n" % from_overlay_mem)
        print_out.write("[INFO] No Reference\t\t:%ld(RAW:%ld, xdelta3:%ld)\n" % \
                ((from_raw+from_xdelta), from_raw, from_xdelta))
        print_out.write("-"*80 + "\n")


def diff_with_deltalist(source_deltalist, const_deltalist, ref_id):
    # update source_deltalist using const_deltalist
    # Example) source_deltalist: disk delta list,
    #       const_deltalist: memory delta list
    if len(source_deltalist) == 0 or type(source_deltalist[0]) != DeltaItem:
        raise DeltaError("Need list of DeltaItem for source")
    if len(const_deltalist) == 0 or type(const_deltalist[0]) != DeltaItem:
        raise DeltaError("Need list of DeltaItem for const")

    source_deltalist.sort(key=itemgetter('hash_value')) # sort by hash value
    const_deltalist.sort(key=itemgetter('hash_value')) # sort by hash value

    matching_count = 0
    s_index = 0
    index = 0
    while index < len(source_deltalist) and s_index < len(const_deltalist):
        source_delta = source_deltalist[s_index]
        const_delta = const_deltalist[index]
        if const_delta.hash_value < source_delta.hash_value:
            index += 1
            continue

        # compare
        if source_delta.hash_value == const_delta.hash_value and \
                ((source_delta.ref_id == DeltaItem.REF_XDELTA) or (source_delta.ref_id == DeltaItem.REF_RAW)):
            if source_delta.offset_len != const_delta.offset_len:
                message = "Hash is same but length is different %d != %d" % \
                        (source_delta.offset_len, const_delta.offset_len)
                raise DeltaError(message)
            matching_count += 1
            source_delta.ref_id = ref_id
            source_delta.data_len = 8
            source_delta.data = long(const_delta.offset)
        s_index += 1
    return source_deltalist


def diff_with_hashlist(base_hashlist, delta_list, ref_id):
    # update delta_list using base_hashlist

    if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
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


def recover_delta_list(delta_list, base_disk, base_mem, chunk_size, 
        parent=None, overlay_memory=None):
    # recover delta list using base disk/memory
    # You have to specify parent to indicate whether you're recover memory or disk 
    # optionally you can use overlay_memory to recover overlay disk which is
    # de-duplicated with overlay memory
    if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
        raise MemoryError("Need list of DeltaItem")

    # initialize reference data to use mmap
    base_disk_fd = open(base_disk, "rb")
    base_mem_fd = open(base_mem, "rb")
    raw_disk = mmap.mmap(base_disk_fd.fileno(), 0, prot=mmap.PROT_READ)
    raw_mem = mmap.mmap(base_mem_fd.fileno(), 0, prot=mmap.PROT_READ)
    if parent == base_disk:
        parent_raw = raw_disk
    elif parent == base_mem:
        parent_raw = raw_mem
    else:
        raise DeltaError("Parent should be either disk or memory")
    if overlay_memory:
        overlay_mem_fd = open(overlay_memory, "rb")
        raw_mem_overlay = mmap.mmap(overlay_mem_fd.fileno(), 0, prot=mmap.PROT_READ)
    else:
        raw_mem_overlay = None

    delta_list.sort(key=itemgetter('offset'))
    zero_data = struct.pack("!s", chr(0x00)) * chunk_size
    for index, delta_item in enumerate(delta_list):
        if delta_item.ref_id == DeltaItem.REF_RAW:
            continue
        elif (delta_item.ref_id == DeltaItem.REF_ZEROS):
            recover_data = zero_data
        elif (delta_item.ref_id == DeltaItem.REF_BASE_MEM):
            offset = delta_item.data
            recover_data = raw_mem[offset:offset+chunk_size]
        elif (delta_item.ref_id == DeltaItem.REF_BASE_DISK):
            offset = delta_item.data
            recover_data = raw_disk[offset:offset+chunk_size]
        elif (delta_item.ref_id == DeltaItem.REF_OVERLAY_MEM):
            if not raw_mem_overlay:
                msg = "Need overlay memory if overlay disk is de-duplicated with it"
                raise DeltaError(msg)
            offset = delta_item.data
            recover_data = raw_mem_overlay[offset:offset+chunk_size]
        elif delta_item.ref_id == DeltaItem.REF_SELF:
            ref_offset = delta_item.data
            index = 0
            while index < len(delta_list):
                #print "self referencing : %ld == %ld" % (delta_list[index].offset, ref_offset)
                if delta_list[index].offset == ref_offset:
                    recover_data = delta_list[index].data
                    break
                index += 1
            if index >= len(delta_list):
                raise MemoryError("Cannot find self reference")
        elif delta_item.ref_id == DeltaItem.REF_XDELTA:
            patch_data = delta_item.data
            base_data = parent_raw[delta_item.offset:delta_item.offset+chunk_size]
            recover_data = tool.merge_data(base_data, patch_data, len(base_data)*2)
        else:
            raise MemoryError("Cannot recover: invalid referce id %d" % delta_item.ref_id)

        if len(recover_data) != chunk_size:
            msg = "Recovered Size Error: %d, ref_id: %d, %ld, %ld" % \
                    (len(recover_data), delta_item.ref_id, delta_item.data_len, delta_item.data)
            raise MemoryError(msg)

        # recover
        delta_item.ref_id = DeltaItem.REF_RAW
        delta_item.data = recover_data

    raw_disk.close()
    raw_mem.close()

