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
import tool
from hashlib import sha256
from operator import itemgetter
from pprint import pprint

class KVMMemoryError(Exception):
    pass

class HashItem(object):
    def __init__(self, s_index=None, e_index=None, hash_value=None):
        self.s_index = s_index
        self.e_index = e_index
        self.hash_value = hash_value


class KVMMemory(object):
    HASH_FILE_MAGIC = 0x1145511a
    HASH_FILE_VERSION = 0x00000001
    DELTA_FILE_MAGIC = 0x1145511b
    DELTA_FILE_VERSION = 0x00000001

    # kvm-qemu constant (version 1.0.0)
    RAM_MAGIC = 0x5145564d
    RAM_VERSION = 0x00000003
    RAM_ID_STRING       =   "pc.ram"
    RAM_ID_LENGTH       =   len(RAM_ID_STRING)
    RAM_PAGE_SIZE       =   1<<12 # 4K bytes
    RAM_SAVE_FLAG_COMPRESS = 0x02
    RAM_SAVE_FLAG_MEM_SIZE = 0x04
    RAM_SAVE_FLAG_PAGE     = 0x08
    RAM_SAVE_FLAG_EOS      = 0x10
    RAM_SAVE_FLAG_CONTINUE = 0x20
    BLK_MIG_FLAG_EOS       = 0x02

    def __init__(self):
        self.hash_list = []
        self.raw_file = ''

    @staticmethod
    def load_from_kvm(filepath, raw_path=None):
        # filepath  : input KVM Memory Snapshot file path
        # outpath   : make raw Memory file at a given path
        memory = KVMMemory()
        memory.hash_list = memory._load_file(filepath, decomp_path=raw_path)
        memory.raw_path = raw_path

        return memory

    @staticmethod
    def load_from_libvirt(filepath):
        memory = KVMMemory()
        pass

    @staticmethod
    def import_from_hashfile(in_path, raw_path):
        memory = KVMMemory()
        fd = open(in_path, "rb")

        # MAGIC & VERSION
        magic, version = struct.unpack("<qq", fd.read(8+8))
        if magic != KVMMemory.HASH_FILE_MAGIC or version != KVMMemory.HASH_FILE_VERSION:
            msg = "Hash file magic number(%ld), version(%ld) does not match" \
                    % (KVMMemory.HASH_FILE_MAGIC, KVMMemory.HASH_FILE_VERSION)
            raise IOError(msg)
        while True:
            data = fd.read(8+8+32) # start_offset, end_offset, hash
            if not data:
                break
            value = tuple(struct.unpack("<qq32s", data))
            memory.hash_list.append(value)
        fd.close()
        return memory

    @staticmethod
    def pack_hashlist(hash_list):
        # pack hash list
        original_length = len(hash_list)
        hash_list = dict((x[2], x) for x in hash_list).values()
        #print "[Debug] hashlist is packed: from %d to %d : %lf" % \
        #        (original_length, len(hash_list), 1.0*len(hash_list)/original_length)
        

    @staticmethod
    def deltalist_tofile(delta_list, f_path):
        fd = open(f_path, "wb")
        # Write MAGIC & VERSION
        fd.write(struct.pack("<q", KVMMemory.DELTA_FILE_MAGIC))
        fd.write(struct.pack("<q", KVMMemory.DELTA_FILE_VERSION))
        for (start_offset, end_offset, ref_id, data) in delta_list:
            # save it as little endian format
            row = struct.pack("<qqc", start_offset, end_offset, chr(ref_id))
            fd.write(row)
            #print "%ld %ld %ld" % (start_offset, end_offset, ref_id)
            if ref_id == 0 or ref_id == None:
                fd.write(struct.pack("<%ds" % KVMMemory.RAM_PAGE_SIZE, data))
            elif ref_id == 1:
                fd.write(struct.pack("<q", long(data)))
            else:
                fd.write(struct.pack("<32s", data))
        fd.close()

    @staticmethod
    def deltalist_fromfile(f_path):
        delta_list = []
        # MAGIC & VERSION
        fd = open(f_path, "rb")
        magic, version = struct.unpack("<qq", fd.read(8+8))
        if magic != KVMMemory.DELTA_FILE_MAGIC or version != KVMMemory.DELTA_FILE_VERSION:
            msg = "delta magic number(%ld), version(%ld) does not match" \
                    % (KVMMemory.DELTA_FILE_MAGIC, KVMMemory.DELTA_FILE_VERSION)
            raise IOError(msg)
        while True:
            data = fd.read(8+8+1) # start_offset, end_offset, ref_id
            if not data:
                break
            (s, e, ref_id) = tuple(struct.unpack("<qqc", data))
            ref_id = ord(ref_id)
            #print "%ld %ld %ld" % (s,e,ref_id)
            if ref_id == 0 or ref_id == None:
                data = struct.unpack("<%ds" % KVMMemory.RAM_PAGE_SIZE, \
                        fd.read(KVMMemory.RAM_PAGE_SIZE))[0]
            elif ref_id == 1:
                data = struct.unpack("<q", fd.read(8))[0]
            else:
                data = struct.unpack("<32s", fd.read(32))[0]
            delta_list.append((s, e, ref_id, data))

        fd.close()
        return delta_list 

    def export_to_file(self, f_path):
        fd = open(f_path, "wb")

        # Write MAGIC & VERSION
        fd.write(struct.pack("<q", self.HASH_FILE_MAGIC))
        fd.write(struct.pack("<q", self.HASH_FILE_VERSION))
        for (start_offset, end_offset, data) in self.hash_list:
            # save it as little endian format
            row = struct.pack("<qq32s", start_offset, end_offset, data)
            fd.write(row)
        fd.close()

    def _seek_string(self, f, string):
        # return: index of end of the found string
        start_index = f.tell()
        memdata = ''
        while True:
            memdata = f.read(4096)
            if not memdata:
                raise KVMMemoryError("Cannot find %s from give memory snapshot" % self.RAM_ID_STRING)

            ram_index = memdata.find(self.RAM_ID_STRING)
            if ram_index:
                if ord(memdata[ram_index-1]) == len(string):
                    position = start_index + ram_index
                    f.seek(position)
                    return position
            start_index += len(memdata)

    def _load_cont_ram_block(self, f, hash_list, max_size, **kwargs):
        # Load KVM Memory snapshot file and 
        # extract hashlist of each memory page while interpreting the format
        # filepath = file path of the loading file
        # kwargs
        #  diff: compare hash_list with self object
        #  decomp_stream: write decompress memory to decopm_stream
        diff = kwargs.get("diff", None)
        decomp_stream = kwargs.get("decomp_stream", None)

        offset = 0
        while True:
            header_flag =  struct.unpack(">q", f.read(8))[0]
            comp_flag = header_flag & 0x0fff
            if comp_flag & self.RAM_SAVE_FLAG_EOS:
                break

            offset = header_flag & ~0x0fff
            if not comp_flag & self.RAM_SAVE_FLAG_CONTINUE:
                id_length, id_string = struct.unpack(">c%ds" % \
                        self.RAM_ID_LENGTH, f.read(1+self.RAM_ID_LENGTH))
                #print "id string : %s" % id_string

            if comp_flag & self.RAM_SAVE_FLAG_COMPRESS:
                #print "processing (%ld)\tcompressed" % (offset)
                compressed_byte = f.read(1)
                data = compressed_byte*self.RAM_PAGE_SIZE
            elif comp_flag & self.RAM_SAVE_FLAG_PAGE:
                #print "processing (%ld)\traw" % (offset)
                data = f.read(self.RAM_PAGE_SIZE)
            else:
                raise KVMMemoryError("Cannot interpret the memory: \
                        invalid header compression flag")

            # kwargs: diff
            if not diff:
                # make new hash list
                hash_list.append((offset, offset+self.RAM_PAGE_SIZE, sha256(data).digest()))
            else:
                # compare it with self, save only when it is different
                self_hash_value = self.hash_list[offset/self.RAM_PAGE_SIZE][2]
                if self_hash_value != sha256(data).digest():
                    hash_list.append((offset, offset+self.RAM_PAGE_SIZE, sha256(data).digest(), data))
                # memory overusage protection
                if len(hash_list) > 100000:
                    raise KVMMemoryError("possibly comparing with wrong base VM")

            # kwargs: decomp_stream
            if decomp_stream:
                decomp_stream.write(data)

            # read can be continued to pc.rom without EOS flag
            if offset+self.RAM_PAGE_SIZE == max_size:
                break;

        return offset

    def _load_file(self, filepath, **kwargs):
        # Load KVM Memory snapshot file and 
        # extract hashlist of each memory page while interpreting the format
        # filepath = file path of the loading file
        # kwargs
        #  diff_file: compare filepath(modified ram) with self hash
        #  decomp_path: write decompress memory to given path
        diff = kwargs.get("diff", None)
        decomp_path = kwargs.get("decomp_path", None)
        if diff and len(self.hash_list) == 0:
            raise KVMMemoryError("Cannot compare give file this self.hashlist")
        decomp_stream = None 
        if decomp_path:
            decomp_stream = open(decomp_path, "wb")

        # Convert big-endian to little-endian
        hash_list = []
        f = open(filepath, "rb")
        magic_number, version = struct.unpack(">II", f.read(4+4))
        if magic_number != KVMMemory.RAM_MAGIC or version != KVMMemory.RAM_VERSION:
            raise KVMMemoryError("Invalid memory image magic/version")

        # find header information about pc.ram
        self._seek_string(f, self.RAM_ID_STRING)
        id_string, total_mem_size = struct.unpack(">%dsq" % self.RAM_ID_LENGTH,\
                f.read(self.RAM_ID_LENGTH+8))

        # interpret details of pc.ram
        position = self._seek_string(f, self.RAM_ID_STRING)
        f.seek(position-(1+8))  # move back to start of the memory section header 

        read_mem_size = 0
        while True:
            read_mem_size = self._load_cont_ram_block(f, hash_list, total_mem_size, diff=diff, decomp_stream=decomp_stream)
            if (read_mem_size+self.RAM_PAGE_SIZE) == total_mem_size:
                break;

            # TODO: This is somewhat hardcoded assuming
            # that block device migration is return with blk_enable = 0
            # See block_save_live() at block-migration.c 
            # Therefore, this script will not work with disk migration
            section_start, section_id = struct.unpack(">cI", f.read(5))
            block_flag = struct.unpack(">q", f.read(8))[0]
            if not block_flag == self.BLK_MIG_FLAG_EOS:
                raise KVMMemoryError("Block migration is enabled, so this script does not compatilbe")
            section_start, section_id = struct.unpack(">cI", f.read(5))

        if decomp_stream:
            decomp_stream.close()
        #print "load %ld memory from %ld file" % (read_mem_size, f.tell())
        return hash_list

    def __sub__(self, other):
        new_hashlist = other.hash_list
        if len(self.hash_list) != len(new_hashlist):
            raise KVMMemoryError("Cannot compare it: Different length of hashlist")

        diff_list = []
        for index, (s_offset, e_offset, value) in enumerate(self.hash_list):
            (new_soffset, new_eoffset, new_value) = new_hashlist[index]
            if (s_offset != new_soffset) or (e_offset != new_eoffset):
                msg = "Cannot compare it: Different offset\nsource:(%ld,%ld) != dest:(%ld,%ld)" % \
                        (s_offset, e_offset, new_soffset, new_eoffset)
                raise KVMMemoryError(msg)
            if value != new_value:
                diff_list.append((value, s_offset, e_offset))
        return diff_list

    def get_modified_page(self, mem_file, raw_file):
        return self._load_file(mem_file, diff=raw_file)
    
    def get_delta(self, modified_list, ref_id):
        # make self as a unique list for better comparison performance
        KVMMemory.pack_hashlist(self.hash_list)
        self.hash_list.sort(key=itemgetter(2)) # sort by hash value
        modified_list.sort(key=itemgetter(2)) # sort by hash value

        matching_count = 0
        s_index = 0
        index = 0
        delta_list = []
        while index < len(self.hash_list) and s_index < len(modified_list):
            (s_start, s_end, s_hash_value, data) = modified_list[s_index]
            (start, end, hash_value) = self.hash_list[index]
            if hash_value < s_hash_value:
                index += 1
                #print "[Debug] move to next : %d" % index
                continue

            # compare
            if s_hash_value == hash_value:
                matching_count += 1
                #print "[Debug] page %ld is matching base %ld" % (s_start, start)
                delta_list.append((s_start, s_end, ref_id, s_hash_value))
                s_index += 1
            else:
                delta_list.append((s_start, s_end, 0, data))
                s_index += 1

        #print "[Debug] matching: %d/%d" % (matching_count, len(modified_list))
        return delta_list

def get_self_delta(delta_list, ref_id):
    # delta_list : list of (start, end, ref_id, hash/data)
    delta_list.sort(key=itemgetter(3, 0)) # sort by (hash/start offset)

    pivot = delta_list[0]
    matching = 0
    for index, delta_item in enumerate(delta_list[1:]):
        if delta_item[3]== pivot[3]:
            # same data/hash
            # save reference start offset
            new_data = (delta_item[0], delta_item[1], ref_id, pivot[0])
            delta_list[index+1] = new_data
            matching += 1
        else:
            pivot=delta_item

    print "[Debug] self delta : %ld/%ld" % (matching, len(delta_list))


def print_delta_statistics(delta_list, self_id=1, base_memory_id=2):
    self_delta = 0
    base_delta = 0
    for (s, e, ref_id, data) in delta_list:
        if ref_id == self_id:
            self_delta += 1
        elif ref_id == base_memory_id:
            base_delta += 1

    print "[INFO] Modified page #\t:%ld" % len(delta_list)
    print "[INFO] Delta from Base\t:%ld" % base_delta
    print "[INFO] Delta from Self\t:%ld" % self_delta


if __name__ == "__main__":
    command = sys.argv[1]
    if command == "hashing":
        infile = sys.argv[2]
        base = KVMMemory.load_from_kvm(infile, raw_path=infile+".raw")
        base.export_to_file(infile+".hash")
    elif command == "diff":
        raw_file = sys.argv[2]+".raw"
        base = KVMMemory.import_from_hashfile(sys.argv[2], raw_file)

        # 1.get modified page
        print "[Debug] get modified page list"
        self_id = 1
        base_mem_id = 2
        modified_pages = base.get_modified_page(sys.argv[3], raw_file)

        # 2.find shared with base memory 
        print "[Debug] get delta from base Memory"
        delta_list = base.get_delta(modified_pages, ref_id=base_mem_id)

        # 3.find shared with self
        print "[Debug] get delta from itself"
        get_self_delta(delta_list, ref_id=self_id)
        delta_list.sort(key=itemgetter(0))

        print_delta_statistics(delta_list, self_id=1, base_memory_id=2)
        KVMMemory.deltalist_tofile(delta_list, sys.argv[3]+".delta")
        new_delta_list = KVMMemory.deltalist_fromfile(sys.argv[3]+".delta")
        for index, values in enumerate(delta_list):
            new_values = new_delta_list[index]
            if values != new_values:
                raise Exception("import/export failed")


