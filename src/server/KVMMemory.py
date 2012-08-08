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

import os
import sys
import struct
import tool
import mmap
from hashlib import sha256
from operator import itemgetter
from pprint import pprint
from optparse import OptionParser

class KVMMemoryError(Exception):
    pass

class DeltaItem(object):
    REF_RAW             =   0x00
    REF_XDELTA          =   0x01
    REF_SELF            =   0x02
    REF_BASE_DISK       =   0x03
    REF_BASE_MEM        =   0x04
    REF_OVERLAY_DISK    =   0x05
    REF_OVERLAY_MEM     =   0x06

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
        # offset_len    : unsigned int
        # hash_value    : hash of original data
        # ref_id        : unsigned char
        # data_len      : unsigned int
        # data          : char[data_len]
        data = struct.pack("<QI32scI", \
                self.offset,
                self.offset_len,
                self.hash_value,
                chr(self.ref_id),
                self.data_len)
        if self.data_len:
            if self.ref_id == DeltaItem.REF_XDELTA:
                data += struct.pack("<%ds" % self.data_len, self.data)
            elif self.ref_id == DeltaItem.REF_RAW:
                data += struct.pack("<%ds" % KVMMemory.RAM_PAGE_SIZE, self.data)
            else:
                data += struct.pack("<Q", self.data)
        return data

    @staticmethod
    def unpack_stream(stream):
        data = stream.read(8+4+32+1+4)
        if not data:
            return None

        (offset, offset_len, hash_value, ref_id, data_len) = \
                struct.unpack("<QI32scI", data)
        ref_id = ord(ref_id)
        if data_len:
            if ref_id == DeltaItem.REF_XDELTA:
                data = struct.unpack("<%ds" % data_len, stream.read(data_len))[0]
            elif ref_id == DeltaItem.REF_RAW:
                data = struct.unpack("<%ds" % KVMMemory.RAM_PAGE_SIZE, stream.read(KVMMemory.RAM_PAGE_SIZE))[0]
            else:
                data = struct.unpack("<Q", stream.read(data_len))[0]
        item = DeltaItem(offset, offset_len, hash_value, ref_id, data_len, data)
        return item

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
        self.raw_mmap = None

    def __sub__(self, other):
        new_hashlist = other.hash_list
        if len(self.hash_list) != len(new_hashlist):
            raise KVMMemoryError("Cannot compare it: Different length of hashlist")
        diff_list = []
        for index, item in enumerate(self.hash_list):
            new_item = new_hashlist[index]
            if (item.offset_start != new_item.offset_start) or (item.offset_len != new_item.offset_len):
                msg = "Cannot compare it: Different offset"
                raise KVMMemoryError(msg)
            if item.hash_value != new_item.hash_value:
                diff_list.append(item)
        return diff_list

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
            if diff:
                # compare it with self, save only when it is different
                self_hash_value = self.hash_list[offset/self.RAM_PAGE_SIZE][2]
                if self_hash_value != sha256(data).digest():
                    #get xdelta comparing self.raw
                    source_data = self.get_raw_data(offset, self.RAM_PAGE_SIZE)
                    #save xdelta as DeltaItem only when it gives smaller
                    try:
                        patch = tool.diff_data(source_data, data, 2*len(source_data))
                        if len(patch) < len(data):
                            delta_item = DeltaItem(offset, self.RAM_PAGE_SIZE, 
                                    hash_value=sha256(data).digest(),
                                    ref_id=DeltaItem.REF_XDELTA,
                                    data_len=len(patch),
                                    data=patch)
                        else:
                            raise IOError("xdelta3 patch is bigger than origianl")
                    except IOError as e:
                        #print "[INFO] xdelta failed, so save it as raw (%s)" % str(e)
                        delta_item = DeltaItem(offset, self.RAM_PAGE_SIZE, 
                                hash_value=sha256(data).digest(),
                                ref_id=DeltaItem.REF_RAW,
                                data_len=len(data),
                                data=data)
                    hash_list.append(delta_item)
                # memory overusage protection
                if len(hash_list) > 200000: # 800MB if PAGE_SIZE == 4K
                    raise KVMMemoryError("possibly comparing with wrong base VM")
            else:
                # make new hash list
                hash_list.append((offset, self.RAM_PAGE_SIZE, sha256(data).digest()))

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
        #  decomp_stream: write decompress memory to given path
        decomp_stream = kwargs.get("decomp_stream", None)
        diff = kwargs.get("diff", None)
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

    @staticmethod
    def load_from_kvm(filepath, out_path=None):
        # filepath  : input KVM Memory Snapshot file path
        # outpath   : make raw Memory file at a given path
        memory = KVMMemory()
        memory.raw_file = open(out_path, "wb")
        memory.hash_list = memory._load_file(filepath, decomp_stream=memory.raw_file)
        return memory

    @staticmethod
    def load_from_libvirt(filepath):
        memory = KVMMemory()
        pass

    @staticmethod
    def import_from_hashfile(hash_path, raw_path):
        if not os.path.exists(raw_path):
            msg = "Cannot import from hash file, No raw file at : %s" % raw_path
            raise KVMMemoryError(msg)

        memory = KVMMemory()
        memory.raw_file = open(raw_path, "rb")
        fd = open(hash_path, "rb")

        # MAGIC & VERSION
        magic, version = struct.unpack("<qq", fd.read(8+8))
        if magic != KVMMemory.HASH_FILE_MAGIC:
            msg = "Hash file magic number(%ld != %ld) does not match" % (magic, KVMMemory.HASH_FILE_MAGIC)
            raise IOError(msg)
        if version != KVMMemory.HASH_FILE_VERSION:
            msg = "Hash file version(%ld != %ld) does not match" % \
                    (version, KVMMemory.HASH_FILE_VERSION)
            raise IOError(msg)
        while True:
            data = fd.read(8+4+32) # start_offset, length, hash
            if not data:
                break
            value = tuple(struct.unpack("<qI32s", data))
            memory.hash_list.append(value)
        fd.close()
        return memory

    @staticmethod
    def pack_hashlist(hash_list):
        # pack hash list
        original_length = len(hash_list)
        hash_list = dict((x[2], x) for x in hash_list).values()
        print "[Debug] hashlist is packed: from %d to %d : %lf" % \
                (original_length, len(hash_list), 1.0*len(hash_list)/original_length)
        

    def export_to_file(self, f_path):
        fd = open(f_path, "wb")

        # Write MAGIC & VERSION
        fd.write(struct.pack("<q", KVMMemory.HASH_FILE_MAGIC))
        fd.write(struct.pack("<q", KVMMemory.HASH_FILE_VERSION))
        for (start_offset, length, data) in self.hash_list:
            # save it as little endian format
            row = struct.pack("<qI32s", start_offset, length, data)
            fd.write(row)
        fd.close()

    def get_raw_data(self, offset, length):
        if not self.raw_mmap:
            self.raw_mmap = mmap.mmap(self.raw_file.fileno(), 0, prot=mmap.PROT_READ)
        return self.raw_mmap[offset:offset+length]

    def get_modified_page(self, new_kvm_file ):
        return self._load_file(new_kvm_file, diff=True)
    
    def get_delta(self, delta_list, ref_id):
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise KVMMemoryError("Need list of DeltaItem")

        # make self as a unique list for better comparison performance
        KVMMemory.pack_hashlist(self.hash_list)
        self.hash_list.sort(key=itemgetter(2)) # sort by hash value
        delta_list.sort(key=itemgetter('hash_value')) # sort by hash value

        matching_count = 0
        s_index = 0
        index = 0
        while index < len(self.hash_list) and s_index < len(delta_list):
            delta = delta_list[s_index]
            (start, length, hash_value) = self.hash_list[index]
            if hash_value < delta.hash_value:
                index += 1
                #print "[Debug] move to next : %d" % index
                continue

            # compare
            if delta.hash_value == hash_value and delta.ref_id == DeltaItem.REF_XDELTA:
                matching_count += 1
                #print "[Debug] page %ld is matching base %ld" % (s_start, start)
                delta.ref_id = ref_id
                delta.data_len = 8
                delta.data = long(start)
            s_index += 1

        print "[Debug] matching %d out of %d total pages" % (matching_count, len(delta_list))
        return delta_list


class DeltaList(object):
    @staticmethod
    def tofile(delta_list, f_path):
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise KVMMemoryError("Need list of DeltaItem")

        fd = open(f_path, "wb")
        # Write MAGIC & VERSION
        fd.write(struct.pack("<q", KVMMemory.DELTA_FILE_MAGIC))
        fd.write(struct.pack("<q", KVMMemory.DELTA_FILE_VERSION))
        for item in delta_list:
            # save it as little endian format
            fd.write(item.get_serialized())
        fd.close()

    @staticmethod
    def fromfile(f_path):
        delta_list = []
        # MAGIC & VERSION
        fd = open(f_path, "rb")
        magic, version = struct.unpack("<qq", fd.read(8+8))
        if magic != KVMMemory.DELTA_FILE_MAGIC or version != KVMMemory.DELTA_FILE_VERSION:
            msg = "delta magic number(%ld), version(%ld) does not match" \
                    % (KVMMemory.DELTA_FILE_MAGIC, KVMMemory.DELTA_FILE_VERSION)
            raise IOError(msg)

        while True:
            new_item = DeltaItem.unpack_stream(fd)
            if not new_item:
                break
            delta_list.append(new_item)
        fd.close()
        return delta_list 

    @staticmethod
    def get_self_delta(delta_list):
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise KVMMemoryError("Need list of DeltaItem")

        # delta_list : list of (start, end, ref_id, hash/data)
        delta_list.sort(key=itemgetter('hash_value', 'offset')) # sort by (hash/start offset)

        pivot = delta_list[0]
        matching = 0
        for index, delta_item in enumerate(delta_list[1:]):
            if delta_item.hash_value == pivot.hash_value and \
                    delta_item.ref_id == DeltaItem.REF_XDELTA:
                # same data/hash
                # save reference start offset
                delta_item.ref_id = DeltaItem.REF_SELF
                delta_item.data_len = 8
                delta_item.data = long(pivot.offset)
                matching += 1
            else:
                pivot=delta_item

        print "[Debug] self delta : %ld/%ld" % (matching, len(delta_list))
        delta_list.sort(key=itemgetter('offset'))

    @staticmethod
    def statistics(delta_list):
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise KVMMemoryError("Need list of DeltaItem")

        from_self = 0
        from_raw = 0
        from_base_disk = 0
        from_base_mem = 0
        from_xdelta = 0
        from_overlay_disk = 0
        from_overlay_mem = 0
        for delta_item in delta_list:
            if delta_item.ref_id == DeltaItem.REF_SELF:
                from_self += 1
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

        print "[INFO] Total Modified page #\t:%ld" % len(delta_list)
        print "[INFO] Saved as RAW\t\t:%ld" % from_raw
        print "[INFO] Saved by xdelta3\t\t:%ld" % from_xdelta
        print "[INFO] Shared within Self\t:%ld" % from_self
        print "[INFO] Shared with Base Disk\t:%ld" % from_base_disk
        print "[INFO] Shared with Base Mem\t:%ld" % from_base_mem
        print "[INFO] Shared with Overlay Disk\t:%ld" % from_overlay_disk
        print "[INFO] Shared with Overlay Mem\t:%ld" % from_overlay_mem


def process_cmd(argv):
    COMMANDS = ['hashing', 'delta']
    USAGE = "Usage: %prog " + "[%s] [option]" % '|'.join(COMMANDS)
    VERSION = '%prog ' + str(1.0)
    DESCRIPTION = "KVM Memory struction interpreste"

    parser = OptionParser(usage=USAGE, version=VERSION, description=DESCRIPTION)
    parser.add_option("-m", "--migrated_file", type="string", dest="mig_file", action='store', \
            help="Migrated file path")
    parser.add_option("-r", "--raw_file", type="string", dest="raw_file", action='store', \
            help="Raw memory path")
    parser.add_option("-s", "--hash_file", type="string", dest="hash_file", action='store', \
            help="Hashsing file path")
    parser.add_option("-d", "--delta", type="string", dest="delta_file", action='store', \
            default="mem_delta", help="path for delta list")
    settings, args = parser.parse_args()
    if len(args) != 1:
        parser.error("Cannot find command")
    command = args[0]
    if command not in COMMANDS:
        parser.error("Invalid Command: %s, supporing %s" % (command, ' '.join(COMMANDS)))
    return settings, command


if __name__ == "__main__":
    settings, command = process_cmd(sys.argv)
    if command == "hashing":
        if not settings.mig_file:
            sys.stderr.write("Error, Cannot find migrated file. See help\n")
            sys.exit(1)
        infile = settings.mig_file
        base = KVMMemory.load_from_kvm(infile, out_path=infile+".raw")
        base.export_to_file(infile+".hash")
    elif command == "delta":
        if (not settings.raw_file) or (not settings.hash_file):
            sys.stderr.write("Error, Cannot find raw/hash file. See help\n")
            sys.exit(1)
        if (not settings.mig_file) or (not settings.delta_file):
            sys.stderr.write("Error, Cannot find modified memory file. See help\n")
            sys.exit(1)
        raw_path = settings.raw_file
        hash_path = settings.hash_file
        modi_mem_path = settings.mig_file
        out_path = settings.delta_file

        base = KVMMemory.import_from_hashfile(hash_path, raw_path)

        # 1.get modified page
        print "[Debug] get modified page list"
        delta_list = base.get_modified_page(modi_mem_path)

        # 2.find shared with base memory 
        print "[Debug] get delta from base Memory"
        base.get_delta(delta_list, ref_id=DeltaItem.REF_BASE_MEM)

        # 3.find shared within self
        print "[Debug] get delta from itself"
        DeltaList.get_self_delta(delta_list)

        DeltaList.statistics(delta_list)
        DeltaList.tofile(delta_list, out_path)
        new_delta_list = DeltaList.fromfile(out_path)
        for index, values in enumerate(delta_list):
            new_values = new_delta_list[index]
            if values.offset != new_values.offset or \
                    values.hash_value != new_values.hash_value:
                print "new: " + str(new_values.offset)
                print "old: " + str(values.offset)
                raise Exception("import/export failed")

