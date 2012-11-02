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
import os
import tool
from operator import itemgetter
import multiprocessing 

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
    def statistics(delta_list, print_out=sys.stdout, discarded_num=0):
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
        from_discarded = discarded_num
        xdelta_size = 0
        raw_size = 0
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
                xdelta_size += len(delta_item.data)
            elif delta_item.ref_id == DeltaItem.REF_RAW:
                from_raw += 1
                raw_size += len(delta_item.data)

        chunk_size = delta_list[0].offset_len
        size_MB = chunk_size/1024.0
        total_count= (len(delta_list)+discarded_num)/100.0

        print_out.write("-"*50 + "\n")
        print_out.write("[INFO] Total Modified page #\t: %ld\t( 100 %% )\n" % 
                (len(delta_list)+discarded_num))
        print_out.write("[INFO] TRIM/FREE discard\t: %ld\t( %f %% )\n" % 
                (from_discarded, from_discarded/total_count))
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
    END_OF_PIPE = -1234

    def __init__(self, base_disk, base_mem, delta_path, output_path, output_size,
            chunk_size, parent=None, overlay_memory_info=None,
            out_pipe=None, time_queue=None, overlay_map_queue=None):
        # recover delta list using base disk/memory
        # You have to specify parent to indicate whether you're recover memory or disk 
        # optionally you can use overlay_memory to recover overlay disk which is
        # de-duplicated with overlay memory

        if base_disk == None and base_mem == None:
            raise MemoryError("Need either base_disk or base_memory")

        self.delta_path = delta_path
        self.output_path = output_path
        self.output_size = output_size
        self.out_pipe = out_pipe
        self.time_queue = time_queue
        self.base_disk = base_disk
        self.base_mem = base_mem
        self.overlay_map_queue = overlay_map_queue

        self.base_disk_fd = None
        self.base_mem_fd = None
        self.raw_disk = None
        self.raw_mem = None
        self.parent_raw = None
        self.mem_overlay_dict = None
        self.raw_mem_overlay = None
        self.chunk_size = chunk_size
        self.zero_data = struct.pack("!s", chr(0x00)) * chunk_size
        self.recovered_delta_dict = dict()
        self.delta_list = list()
        
        # initialize reference data to use mmap
        if base_disk:
            self.base_disk_fd = open(base_disk, "rb")
            self.raw_disk = mmap.mmap(self.base_disk_fd.fileno(), 0, prot=mmap.PROT_READ)
        if base_mem:
            self.base_mem_fd = open(base_mem, "rb")
            self.raw_mem = mmap.mmap(self.base_mem_fd.fileno(), 0, prot=mmap.PROT_READ)
        if parent == base_disk:
            self.parent_raw = self.raw_disk
        elif parent == base_mem:
            self.parent_raw = self.raw_mem
        else:
            raise DeltaError("Parent should be either disk or memory")

        # create output file without content
        # this is designed for mmap of overlay memory before finishing reconstruction
        recover_fd = open(self.output_path, "wr+b")
        recover_fd.seek(self.output_size-1)
        last_one_byte = recover_fd.read(1)
        if (not last_one_byte) or len(last_one_byte) == 0:
            recover_fd.write('0')
            recover_fd.flush()
        recover_fd.close()

        if overlay_memory_info != None:
            overlay_mem_path = overlay_memory_info['path']
            self.overlay_mem_dict = overlay_memory_info['dict']
            self.overlay_mem_fd = open(overlay_mem_path, "rb")
            self.recover_base_size = os.path.getsize(self.base_disk)
        else:
            self.recover_base_size = os.path.getsize(self.base_mem)
        multiprocessing.Process.__init__(self)

    def run(self):
        start_time = time.time()
        recover_fd = open(self.output_path, "wr+b")
        delta_stream = open(self.delta_path, "r")

        overlay_chunk_ids = []
        for delta_item in DeltaList.from_stream(delta_stream):
            self.recover_item(delta_item)
            if len(delta_item.data) != delta_item.offset_len:
                msg = "recovered size is not same as page size, %ld != %ld" % \
                        (len(delta_item.data), self.chunk_size)
                raise DeltaError(msg)

            # save it to dictionary to find self_reference easily
            self.recovered_delta_dict[delta_item.offset] = delta_item
            self.delta_list.append(delta_item)
            # write to output file 
            recover_fd.seek(delta_item.offset)
            recover_fd.write(delta_item.data)
            overlay_chunk_id = long(delta_item.offset/self.chunk_size)
            overlay_chunk_ids.append(overlay_chunk_id)
            if len(overlay_chunk_ids) % 100 == 0:
                self.out_pipe.send(overlay_chunk_ids)
                overlay_chunk_ids[:] = []

        if len(overlay_chunk_ids) > 0:
            self.out_pipe.send(overlay_chunk_ids)
        self.out_pipe.send(Recovered_delta.END_OF_PIPE)
        self.out_pipe.close()
        recover_fd.close()

        if self.overlay_map_queue != None:
            offset_list = self.recovered_delta_dict.keys()
            chunk_list = [("%ld:1" % (offset/self.chunk_size)) for offset in offset_list]
            self.overlay_map_queue.put(",".join(chunk_list))

        end_time = time.time()
        if self.time_queue != None: 
            self.time_queue.put({'start_time':start_time, 'end_time':end_time})
        print "[Delta] : (%s)-(%s)=(%s)" % \
                (start_time, end_time, (end_time-start_time))

    def recover_item(self, delta_item):
        if type(delta_item) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        #print "recovering %ld/%ld" % (index, len(delta_list))
        if (delta_item.ref_id == DeltaItem.REF_RAW):
            recover_data = delta_item.data
            pass
        elif (delta_item.ref_id == DeltaItem.REF_ZEROS):
            recover_data = self.zero_data
        elif (delta_item.ref_id == DeltaItem.REF_BASE_MEM):
            offset = delta_item.data
            recover_data = self.raw_mem[offset:offset+self.chunk_size]
        elif (delta_item.ref_id == DeltaItem.REF_BASE_DISK):
            offset = delta_item.data
            recover_data = self.raw_disk[offset:offset+self.chunk_size]
        elif (delta_item.ref_id == DeltaItem.REF_OVERLAY_MEM):
            if not self.overlay_mem_fd:
                msg = "Need overlay memory if overlay disk is de-duped with it"
                raise DeltaError(msg)
            offset = delta_item.data
            while True:
                if self.overlay_mem_dict.get(offset/self.chunk_size, False) == True:
                    break
                msg = "Need overlay memory info at %ld" % (offset/self.chunk_size)
                print msg
                time.sleep(1)
            self.overlay_mem_fd.seek(offset)
            recover_data = self.overlay_mem_fd.read(self.chunk_size)
            #recover_data = self.raw_mem_overlay[offset:offset+self.chunk_size]
        elif delta_item.ref_id == DeltaItem.REF_SELF:
            ref_offset = delta_item.data
            self_ref_delta_item = self.recovered_delta_dict.get(ref_offset, None)
            if self_ref_delta_item == None:
                raise MemoryError("Cannot find self reference")
            recover_data = self_ref_delta_item.data
        elif delta_item.ref_id == DeltaItem.REF_XDELTA:
            patch_data = delta_item.data
            patch_original_size = delta_item.offset_len
            base_data = self.parent_raw[delta_item.offset:delta_item.offset+patch_original_size]
            recover_data = tool.merge_data(base_data, patch_data, len(base_data)*5)
        else:
            raise MemoryError("Cannot recover: invalid referce id %d" % delta_item.ref_id)

        if len(recover_data) != delta_item.offset_len:
            msg = "Recovered Size Error: %d, ref_id: %d, %ld, %ld" % \
                    (len(recover_data), delta_item.ref_id, \
                    delta_item.data_len, delta_item.data)
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


def recover_delta_list(delta_list, base_disk, base_mem, chunk_size, 
        parent=None, overlay_memory=None):
    # recover delta list using base disk/memory
    # You have to specify parent to indicate whether you're recover memory or disk 
    # optionally you can use overlay_memory to recover overlay disk which is
    # de-duplicated with overlay memory
    if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
        raise MemoryError("Need list of DeltaItem")
    if base_disk == None and base_mem == None:
        raise MemoryError("Need either base_disk or base_memory")

    # initialize reference data to use mmap
    base_disk_fd = None
    raw_disk = None
    base_mem_fd = None
    raw_mem = None
    
    if base_disk:
        base_disk_fd = open(base_disk, "rb")
        raw_disk = mmap.mmap(base_disk_fd.fileno(), 0, prot=mmap.PROT_READ)
    if base_mem:
        base_mem_fd = open(base_mem, "rb")
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
        #print "recovering %ld/%ld" % (index, len(delta_list))
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
            traverse_index = index-1
            # TODO: need optimization for better comparison
            while traverse_index >= 0:
                #if parent == base_disk:
                    #print "%ld/%ld, self referencing : %ld == %ld" % (index, len(delta_list), delta_list[index].offset, ref_offset)
                if delta_list[traverse_index].offset == ref_offset:
                    recover_data = delta_list[traverse_index].data
                    break
                traverse_index -= 1
            if traverse_index < 0:
                raise MemoryError("Cannot find self reference")
        elif delta_item.ref_id == DeltaItem.REF_XDELTA:
            patch_data = delta_item.data
            patch_original_size = delta_item.offset_len
            base_data = parent_raw[delta_item.offset:delta_item.offset+patch_original_size]
            recover_data = tool.merge_data(base_data, patch_data, len(base_data)*2)
        else:
            raise MemoryError("Cannot recover: invalid referce id %d" % delta_item.ref_id)

        if len(recover_data) != delta_item.offset_len:
            msg = "Recovered Size Error: %d, ref_id: %d, %ld, %ld" % \
                    (len(recover_data), delta_item.ref_id, delta_item.data_len, delta_item.data)
            raise MemoryError(msg)

        # recover
        delta_item.ref_id = DeltaItem.REF_RAW
        delta_item.data = recover_data

    if base_disk_fd:
        base_disk_fd.close()
    if base_mem_fd:
        base_mem_fd.close()
    if raw_disk:
        raw_disk.close()
    if raw_mem:
        raw_mem.close()

