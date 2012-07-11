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

class KVMMemory(object):

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

    @staticmethod
    def load_from_libvirt(filepath):
        memory = KVMMemory()
        pass

    @staticmethod
    def load_from_hashfile(filepath):
        memory = KVMMemory()
        memory.hash_list = tool.hashlist_from_file(filepath)
        return memory

    @staticmethod
    def load_from_kvm(filepath):
        memory = KVMMemory()
        memory.hash_list = memory._load_file(filepath)
        return memory

    @staticmethod
    def pack_hashlist(hash_list):
        # pack hash list
        original_length = len(hash_list)
        hash_list = dict((x[0], x) for x in hash_list).values()
        hash_list.sort(key=itemgetter(1,2))
        print "[Debug] hashlist is packed: from %d to %d : %lf" % \
                (original_length, len(hash_list), 1.0*len(hash_list)/original_length)
        
    def export_to_file(self, f_path):
        tool.hashlist_to_file(self.hash_list, f_path)

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

    def _load_cont_ram_block(self, f, hash_list, max_size, diff=False):
        # Load KVM Memory snapshot file and 
        # extract hashlist of each memory page while interpreting the format
        # filepath = file path of the loading file
        # param diff: compare this file's hash with self

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

            if not diff:
                # make new hash list
                hash_list.append((sha256(data).digest(), offset, offset+self.RAM_PAGE_SIZE))
            else:
                # compare it with self, save only when it is different
                self_hash_value = self.hash_list[offset/self.RAM_PAGE_SIZE][0]
                if self_hash_value != sha256(data).digest():
                    hash_list.append((data, offset, offset+self.RAM_PAGE_SIZE))

            # read can be continued to pc.rom without EOS flag
            if offset+self.RAM_PAGE_SIZE == max_size:
                break;

        return offset

    def _load_file(self, filepath, diff=False):
        # Load KVM Memory snapshot file and 
        # extract hashlist of each memory page while interpreting the format
        # filepath = file path of the loading file
        # param diff: compare this file's hash with self
        if diff and len(self.hash_list) == 0:
            raise KVMMemoryError("Cannot compare give file this self.hashlist")

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
            read_mem_size = self._load_cont_ram_block(f, hash_list, total_mem_size, diff=diff)
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

        #print "load %ld memory from %ld file" % (read_mem_size, f.tell())
        return hash_list

    def __hash__(self):
        return self.hash_list

    def __sub__(self, other):
        new_hashlist = other.hash_list
        if len(self.hash_list) != len(new_hashlist):
            raise KVMMemoryError("Cannot compare it: Different length of hashlist")

        diff_list = []
        for index, (value, s_offset, e_offset) in enumerate(self.hash_list):
            (new_value, new_soffset, new_eoffset) = new_hashlist[index]
            if (s_offset != new_soffset) or (e_offset != new_eoffset):
                msg = "Cannot compare it: Different offset\nsource:(%ld,%ld) != dest:(%ld,%ld)" % \
                        (s_offset, e_offset, new_soffset, new_eoffset)
                raise KVMMemoryError(msg)
            if value != new_value:
                diff_list.append((value, s_offset, e_offset))
        return diff_list

    def get_modified_page(self, mem_file):
        return self._load_file(mem_file, diff=True)
    
    def get_delta(self, source_list, ref_id):
        # make self as a unique list for better comparison performance
        KVMMemory.pack_hashlist(self.hash_list)

        total_count = 0
        matching_count = 0
        latest_index = 0
        delta_list = []
        for (value, start, end) in source_list:
            total_count += 1
            found_index, s_offset, e_offset = tool._search_matching(value, self.hash_list, latest_index)
            if found_index:
                matching_count += 1
                print "[Debug] page %ld is matching base %ld" % (start, s_offset)
                delta_list.append((start, end, ref_id, value))
            else:
                delta_list.append((start, end, 0, value))

        print "[Debug] matching: %d/%d" % (matching_count, total_count)
        return delta_list

def get_self_delta(delta_list, ref_id):
    pass

if __name__ == "__main__":
    command = sys.argv[1]
    if command == "hashing":
        base = KVMMemory.load_from_kvm(sys.argv[2])
        base.export_to_file(sys.argv[2]+".hash")
    elif command == "diff":
        base = KVMMemory.load_from_hashfile(sys.argv[2])

        # 1.get modified page
        print "[Debug] get modified page list"
        modified_hash = base.get_modified_page(sys.argv[3])

        # 2.find shared with base memory 
        print "[Debug] get delta from base Memory"
        delta_list = base.get_delta(modified_hash, ref_id=2)

        # 3.find shared with self
        print "[Debug] get delta from itself"
        get_self_delta(delta_list, ref_id=1)

        print "Diff page: %d" % len(delta_list)

