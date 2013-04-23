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
import vmnetx
import subprocess
from Configuration import Const
from progressbar import AnimatedProgressBar
from delta import DeltaItem
from delta import DeltaList
from hashlib import sha256
from optparse import OptionParser
from delta import Recovered_delta

#GLOBAL
EXT_RAW = ".raw"
EXT_META = "-meta"

class MemoryError(Exception):
    pass

class Memory(object):
    HASH_FILE_MAGIC = 0x1145511a
    HASH_FILE_VERSION = 0x00000001

    # kvm-qemu constant (version 1.0.0)
    RAM_MAGIC = 0x5145564d
    RAM_VERSION = 0x00000003
    RAM_PAGE_SIZE    =  (1<<12)
    RAM_ID_STRING       =   "pc.ram"
    RAM_ID_LENGTH       =   len(RAM_ID_STRING)
    RAM_SAVE_FLAG_COMPRESS = 0x02
    RAM_SAVE_FLAG_MEM_SIZE = 0x04
    RAM_SAVE_FLAG_PAGE     = 0x08
    RAM_SAVE_FLAG_RAW      = 0x40
    RAM_SAVE_FLAG_EOS      = 0x10
    RAM_SAVE_FLAG_CONTINUE = 0x20
    BLK_MIG_FLAG_EOS       = 0x02

    def __init__(self):
        self.hash_list = []
        self.raw_file = ''
        self.raw_mmap = None

    @staticmethod
    def _seek_string(f, string):
        # return: index of end of the found string
        start_index = f.tell()
        memdata = ''
        while True:
            memdata = f.read(Memory.RAM_PAGE_SIZE)
            if not memdata:
                raise MemoryError("Cannot find %s from give memory snapshot" % Memory.RAM_ID_STRING)

            ram_index = memdata.find(Memory.RAM_ID_STRING)
            if ram_index:
                if ord(memdata[ram_index-1]) == len(string):
                    position = start_index + ram_index
                    f.seek(position)
                    return position
            start_index += len(memdata)

    def _get_mem_hash(self, fin, start_offset, end_offset, hash_list, **kwargs):
        # kwargs
        #  diff: compare hash_list with self object
        #  print_out: log/process output 
        #  free_pfn_dict: free memory physical frame number as a dictionary {'#':1, ... }
        diff = kwargs.get("diff", None)
        apply_free_memory = kwargs.get("apply_free_memory", True)
        free_pfn_dict = kwargs.get("free_pfn_dict", None)
        print_out = kwargs.get("print_out", open("/dev/null", "w+b"))
        print_out.write("[INFO] Get hash list of memory page\n")
        prog_bar = AnimatedProgressBar(end=100, width=80, stdout=print_out)

        fin.seek(start_offset)
        total_size = end_offset-start_offset
        ram_offset = 0
        freed_page_counter = 0
        while total_size != ram_offset:
            data = fin.read(Memory.RAM_PAGE_SIZE)
            if not diff:
                hash_list.append((ram_offset, len(data), sha256(data).digest()))
            else:
                # compare input with hash or corresponding base memory, save only when it is different
                self_hash_value = self.hash_list[ram_offset/Memory.RAM_PAGE_SIZE][2]

                if self_hash_value != sha256(data).digest():
                    is_free_memory = False
                    if (free_pfn_dict != None) and \
                            (free_pfn_dict.get(long(ram_offset/Memory.RAM_PAGE_SIZE), None) == 1):
                        is_free_memory = True

                    if is_free_memory and apply_free_memory:
                        # Do not compare. It is free memory
                        freed_page_counter += 1
                    else:
                        #get xdelta comparing self.raw
                        source_data = self.get_raw_data(ram_offset, len(data))
                        #save xdelta as DeltaItem only when it gives smaller
                        try:
                            patch = tool.diff_data(source_data, data, 2*len(source_data))
                            if len(patch) < len(data):
                                delta_item = DeltaItem(DeltaItem.DELTA_MEMORY,
                                        ram_offset, len(data),
                                        hash_value=sha256(data).digest(),
                                        ref_id=DeltaItem.REF_XDELTA,
                                        data_len=len(patch),
                                        data=patch)
                            else:
                                raise IOError("xdelta3 patch is bigger than origianl")
                        except IOError as e:
                            #print "[INFO] xdelta failed, so save it as raw (%s)" % str(e)
                            delta_item = DeltaItem(DeltaItem.DELTA_MEMORY,
                                    ram_offset, len(data),
                                    hash_value=sha256(data).digest(),
                                    ref_id=DeltaItem.REF_RAW,
                                    data_len=len(data),
                                    data=data)
                        hash_list.append(delta_item)

                # memory over-usage protection
                if len(hash_list) > Memory.RAM_PAGE_SIZE*1000000: # 400MB for hashlist
                    raise MemoryError("possibly comparing with wrong base VM")
            ram_offset += len(data)
            # print progress bar for every 100 page
            if (ram_offset % (Memory.RAM_PAGE_SIZE*100)) == 0:
                prog_bar.set_percent(100.0*ram_offset/total_size)
                prog_bar.show_progress()
        prog_bar.finish()
        return freed_page_counter

    @staticmethod
    def _seek_to_end_of_ram(fin):
        # get ram total length
        position = Memory._seek_string(fin, Memory.RAM_ID_STRING)
        memory_start_offset = position-(1+8)
        fin.seek(memory_start_offset)
        total_mem_size = long(struct.unpack(">Q", fin.read(8))[0])
        if total_mem_size & Memory.RAM_SAVE_FLAG_MEM_SIZE == 0:
            raise MemoryError("invalid header format: no total memory size")
        total_mem_size = total_mem_size & ~0xfff

        # get ram length information
        read_ramlen_size = 0
        ram_info = dict()
        while total_mem_size > read_ramlen_size:
            id_string_len = ord(struct.unpack(">s", fin.read(1))[0])
            id_string, mem_size = struct.unpack(">%dsQ" % id_string_len,\
                    fin.read(id_string_len+8))
            ram_info[id_string] = {"length":mem_size}
            read_ramlen_size += mem_size

        read_mem_size = 0
        while total_mem_size != read_mem_size:
            raw_ram_flag = struct.unpack(">Q", fin.read(8))[0]
            if raw_ram_flag & Memory.RAM_SAVE_FLAG_EOS:
                raise MemoryError("Error, Not Fully load yet")
                break
            if raw_ram_flag & Memory.RAM_SAVE_FLAG_RAW == 0:
                raise MemoryError("Error, invalid ram save flag raw\n")

            id_string_len = ord(struct.unpack(">s", fin.read(1))[0])
            id_string = struct.unpack(">%ds" % id_string_len, fin.read(id_string_len))[0]
            padding_len = fin.tell() & (Memory.RAM_PAGE_SIZE-1)
            padding_len = Memory.RAM_PAGE_SIZE-padding_len
            fin.read(padding_len)

            cur_offset = fin.tell()
            block_info = ram_info.get(id_string)
            if not block_info:
                raise MemoryError("Unknown memory block : %s", id_string)
            block_info['offset'] = cur_offset
            memory_size = block_info['length']
            fin.seek(cur_offset + memory_size)
            read_mem_size += memory_size

        return fin.tell(), ram_info

    def _load_file(self, filepath, **kwargs):
        # Load KVM Memory snapshot file and 
        # extract hashlist of each memory page while interpreting the format
        # filepath = file path of the loading file
        # kwargs
        #  diff_file: compare filepath(modified ram) with self hash
        #  print_out: log/process output 
        ####
        diff = kwargs.get("diff", None)
        apply_free_memory = kwargs.get("apply_free_memory", True)
        print_out = kwargs.get("print_out", open("/dev/null", "w+b"))
        if diff and len(self.hash_list) == 0:
            raise MemoryError("Cannot compare give file this self.hashlist")

        # Sanity check
        fin = open(filepath, "rb")
        file_size = os.path.getsize(filepath)
        libvirt_mem_hdr = vmnetx._QemuMemoryHeader(fin)
        libvirt_mem_hdr.seek_body(fin)
        libvirt_header_len = fin.tell()
        if (libvirt_header_len != Memory.RAM_PAGE_SIZE):
            # TODO: need to modify libvirt migration file header 
            # in case it is not aligned with memory page size
            msg = "Error description:\n"
            msg += "libvirt header length : %ld\n" % (libvirt_header_len)
            msg += "This happends when resiude generated multiple times\n"
            msg += "It's not easy to fix since header length change will make VM's memory snapshot size\n"
            msg += "different from base VM"
            raise MemoryError(msg)

        # get memory meta data from snapshot
        fin.seek(libvirt_header_len)
        hash_list = []
        ram_end_offset, ram_info = Memory._seek_to_end_of_ram(fin)
        if ram_end_offset == Memory.RAM_PAGE_SIZE:
            print "end offset: %ld" % (ram_end_offset)
            raise MemoryError("ram header+data is not aligned with page size")

        #import pdb;pdb.set_trace()
        if diff:
            # case for getting modified memory list
            if apply_free_memory == True:
                # get free memory list
                mem_size_mb = ram_info.get('pc.ram').get('length')/1024/1024
                mem_offset_infile = ram_info.get('pc.ram').get('offset')
                self.free_pfn_dict = get_free_pfn_dict(filepath, mem_size_mb, \
                        libvirt_header_len, mem_offset_infile)
            else:
                self.free_pfn_dict = None

            freed_counter = self._get_mem_hash(fin, 0, file_size, hash_list, \
                    diff=diff, free_pfn_dict=self.free_pfn_dict, \
                    apply_free_memory=apply_free_memory, print_out=print_out)
        else:
            # case for generating base memory hash list
            freed_counter = self._get_mem_hash(fin, 0, file_size, hash_list, \
                    diff=diff, free_pfn_dict=None, print_out=print_out)

        # get hash of memory area
        self.freed_counter = freed_counter
        print_out.write("[DEBUG] FREED Memory Counter: %ld(%ld)\n" % \
                (freed_counter, freed_counter*Memory.RAM_PAGE_SIZE))
        
        return hash_list

    @staticmethod
    def import_from_metafile(meta_path, raw_path):
        # Regenerate KVM Base Memory DS from existing meta file
        if (not os.path.exists(raw_path)) or (not os.path.exists(meta_path)):
            msg = "Cannot import from hash file, No raw file at : %s" % raw_path
            raise MemoryError(msg)

        memory = Memory()
        memory.raw_file = open(raw_path, "rb")
        hashlist = Memory.import_hashlist(meta_path)
        memory.hash_list = hashlist
        return memory

    @staticmethod
    def import_hashlist(meta_path):
        fd = open(meta_path, "rb")

        # Read Hash Item List
        hash_list = list()
        count = 0
        while True:
            count += 1
            data = fd.read(8+4+32) # start_offset, length, hash
            if not data:
                break
            value = tuple(struct.unpack("!qI32s", data))
            hash_list.append(value)
        fd.close()
        return hash_list

    @staticmethod
    def pack_hashlist(hash_list):
        # pack hash list
        original_length = len(hash_list)
        hash_list = dict((x[2], x) for x in hash_list).values()
        print "[Debug] hashlist is packed: from %d to %d : %lf" % \
                (original_length, len(hash_list), 1.0*len(hash_list)/original_length)
        
    def export_to_file(self, f_path):
        fd = open(f_path, "wb")
        # Write hash item list
        for (start_offset, length, data) in self.hash_list:
            # save it as little endian format
            row = struct.pack("!qI32s", start_offset, length, data)
            fd.write(row)
        fd.close()

    def get_raw_data(self, offset, length):
        # retrieve page data from raw memory
        if not self.raw_mmap:
            self.raw_mmap = mmap.mmap(self.raw_file.fileno(), 0, prot=mmap.PROT_READ)
        return self.raw_mmap[offset:offset+length]

    def get_modified(self, new_kvm_file, apply_free_memory=True, free_memory_info=None):
        # get modified pages 
        hash_list = self._load_file(new_kvm_file, diff=True, \
                print_out=sys.stdout, apply_free_memory=apply_free_memory)
        if free_memory_info != None:
            free_memory_info['free_pfn_dict'] = self.free_pfn_dict
            free_memory_info['freed_counter'] = self.freed_counter

        return hash_list
    

def hashing(filepath):
    # Contstuct KVM Base Memory DS from KVM migrated memory
    # filepath  : input KVM Memory Snapshot file path
    memory = Memory()
    hash_list =  memory._load_file(filepath, print_out=sys.stdout)
    memory.hash_list = hash_list
    return memory


def _process_cmd(argv):
    COMMANDS = ['hashing', 'delta', 'recover']
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
    parser.add_option("-b", "--base", type="string", dest="base_file", action='store', \
            help="path for base memory file")
    settings, args = parser.parse_args()
    if len(args) != 1:
        parser.error("Cannot find command")
    command = args[0]
    if command not in COMMANDS:
        parser.error("Invalid Command: %s, supporing %s" % (command, ' '.join(COMMANDS)))
    return settings, command


def create_memory_deltalist(modified_mempath,
            basemem_meta=None, basemem_path=None,
            apply_free_memory=True,
            free_memory_info=None,
            print_out=None):
    # get memory delta
    # modified_mempath : file path for modified memory
    # basemem_meta : hashlist file for base mem
    # basemem_path : raw base memory path
    # freed_counter_ret : return pointer for freed counter
    # print_out : log stream

    # Create Base Memory from meta file
    base = Memory.import_from_metafile(basemem_meta, basemem_path)

    # 1.get modified page
    print_out.write("[Debug] 1.get modified page list\n")
    delta_list = base.get_modified(modified_mempath, 
            apply_free_memory=apply_free_memory,
            free_memory_info=free_memory_info)


    return delta_list


def recover_memory(base_disk, base_mem, delta_path, out_path, verify_with_original=None):
    # Recover modified memory snapshot
    # base_path: base memory snapshot, delta pages will be applied over it
    # delta_path: memory overlay
    # out_path: path to recovered modified memory snapshot
    # verify_with_original: original modification file for recover verification

    recovered_memory = Recovered_delta(base_disk, base_mem, delta_path, out_path, \
            Memory.RAM_PAGE_SIZE, parent=base_mem)

    chunk_list = []
    for chunk_number in recovered_memory.recover_chunks():
        chunk_list.append("%ld:1" % chunk_number)
    recovered_memory.finish()

    # varify with original
    if verify_with_original:
        modi_mem = open(verify_with_original, "rb")
        base_file = open(base_mem, "rb")
        delta_list_index = 0
        while True:
            offset = base_file.tell()
            if len(delta_list) == delta_list_index:
                break

            base_data = base_file.read(Memory.RAM_PAGE_SIZE)
            
            if offset != delta_list[delta_list_index].offset:
                #print "from base data: %d" % len(base_data)
                modi_mem.seek(offset)
                modi_data = modi_mem.read(len(base_data))
                if modi_data != base_data:
                    msg = "orignal data is not same at %ld" % offset
                    raise MemoryError(msg)
            else:
                modi_mem.seek(offset)
                recover_data = delta_list[delta_list_index].data
                origin_data = modi_mem.read(len(recover_data))
                #print "from recovered data: %d at %ld" % (len(recover_data), delta_list[delta_list_index].offset)
                delta_list_index += 1
                if recover_data != origin_data:
                    msg = "orignal data is not same at %ld" % offset
                    raise MemoryError(msg)

        for delta_item in delta_list:
            offset = delta_item.offset
            data = delta_item.data
            modi_mem.seek(offset)
            origin_data = modi_mem.read(len(data))
            if data != origin_data:
                msg = "orignal data is not same at %ld" % offset
                raise MemoryError(msg)
        print "Pass all varification - Successfully recovered"

    return ','.join(chunk_list)


def base_hashlist(base_memmeta_path):
    # get the hash list from the meta file
    hashlist = Memory.import_hashlist(base_memmeta_path)
    return hashlist


def get_free_pfn_dict(snapshot_path, mem_size, libvirt_header_len, mem_offset_infile):
    pglist_addr = 'c1840a80'
    pgn0_addr = 'f73fd000'
    mem_size_mb = 1024
    if mem_size_mb != mem_size:
        sys.stdout.write("WARNING: Ignore free memory information\n")
        return None

    free_pfn_list = _get_free_pfn_list(snapshot_path, pglist_addr, pgn0_addr, \
            mem_size_mb, libvirt_header_len)
    if free_pfn_list:
        # shift 4096*2 for libvirt header and KVM header
        offset = mem_offset_infile/Memory.RAM_PAGE_SIZE
        free_pfn_dict_aligned = dict([(long(page)+offset,1) for page in free_pfn_list])
        return free_pfn_dict_aligned
    else:
        return None


def _get_free_pfn_list(snapshot_path, pglist_addr, pgn0_addr, mem_size_gb, libvirt_offset):
    # get list of free memory page number
    BIN_PATH = Const.FREE_MEMORY_BIN_PATH
    cmd = [
            "%s" % BIN_PATH,
            "%s" % snapshot_path,
            "%s" % pglist_addr,
            "%s" % pgn0_addr,
            "%d" % mem_size_gb,
        ]
    _PIPE = subprocess.PIPE
    proc = subprocess.Popen(cmd, close_fds=True, stdin=_PIPE, stdout=_PIPE, stderr=_PIPE)
    out, err = proc.communicate()
    if err:
        print "Error: " + err
        return list()
    free_pfn_list = out.split("\n")
    if len(free_pfn_list[-1].strip()) == 0:
        free_pfn_list = free_pfn_list[:-1]
    return free_pfn_list


if __name__ == "__main__":
    settings, command = _process_cmd(sys.argv)
    if command == "hashing":
        if not settings.base_file:
            sys.stderr.write("Error, Cannot find migrated file. See help\n")
            sys.exit(1)
        infile = settings.base_file
        base = hashing(infile)
        base.export_to_file(infile+EXT_META)

        # Check Integrity
        re_base = Memory.import_from_metafile(infile+".meta", infile)
        for index, hashitem in enumerate(re_base.hash_list):
            if base.hash_list[index] != hashitem:
                raise MemoryError("footer data is different")
        print "[SUCCESS] meta file information is matched with original"
    elif command == "delta":
        if (not settings.mig_file) or (not settings.base_file):
            sys.stderr.write("Error, Cannot find modified memory file. See help\n")
            sys.exit(1)
        raw_path = settings.base_file
        meta_path = settings.base_file + EXT_META
        modi_mem_path = settings.mig_file
        out_path = settings.mig_file + ".delta"
        #delta_list = create_memory_overlay(modi_mem_path, raw_path, modi_mem_path, out_path, print_out=sys.stdout)

        mem_deltalist= create_memory_deltalist(modi_mem_path,
                basemem_meta=meta_path, basemem_path=raw_path,
                print_out=sys.stdout)
        DeltaList.statistics(mem_deltalist, print_out=sys.stdout)
        DeltaList.tofile(mem_deltalist, modi_mem_path + ".delta")

    elif command == "recover":
        if (not settings.base_file) or (not settings.delta_file):
            sys.stderr.write("Error, Cannot find base/delta file. See help\n")
            sys.exit(1)
        base_mem = settings.base_file
        overlay_mem = settings.delta_file
        base_memmeta = settings.base_file + EXT_META
        
        out_path = base_mem + ".recover"
        memory_overlay_map = recover_memory(None, base_mem, overlay_mem, \
                base_memmeta, out_path, verify_with_original="./tmp/modi")

