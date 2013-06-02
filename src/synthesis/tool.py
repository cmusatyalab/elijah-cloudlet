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

import xdelta3
import os
import filecmp
import sys
import subprocess
from time import time
from hashlib import sha1
from hashlib import sha256
import mmap
import struct
from lzma import LZMACompressor
from lzma import LZMADecompressor

from synthesis import msgpack 
from synthesis.Configuration import Const


#global
HASHFILE_MAGIC = 0x1145511a
HASHFILE_VERSION = 0x00000001
HASH_CHUNKING_SIZE = 4012
LZMA_OPTION = {'format':'xz', 'level':9}

def diff_files(source_file, target_file, output_file, **kwargs):
    # kwargs
    # skip_validation   :   skipp sha1 validation
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    log = kwargs.get("log", None)
    nova_util = kwargs.get('nova_util', None)

    if os.path.exists(source_file) == False or open(source_file, "rb") == None:
        raise IOError('[Error] No such file %s' % (source_file))
        return None
    if os.path.exists(target_file) == False or open(target_file, "rb") == None:
        raise IOError('[Error] No such file %s' % (target_file))
    if os.path.exists(output_file):
        os.remove(output_file)

    if nova_util:
        nova_util.execute("xdelta3", "-f", "-s", str(source_file), str(target_file), str(output_file))
        return 0
    else:
        print '[INFO] %s(base) - %s  =  %s' % (os.path.basename(source_file), os.path.basename(target_file), os.path.basename(output_file))
        command_delta = ['xdelta3', '-f', '-s', source_file, target_file, output_file]
        ret = xdelta3.xd3_main_cmdline(command_delta)
        if ret != 0:
            raise IOError('Cannot do file diff')
        return ret


def diff_data(source_data, modi_data, buf_len):
    if len(source_data) == 0 or len(modi_data) == 0:
        raise IOError("[Error] Not valid data length: %d, %d" % (len(source_data), len(modi_data)))

    result, patch = xdelta3.xd3_encode_memory(modi_data, source_data, buf_len, xdelta3.XD3_COMPLEVEL_9)
    if result != 0:
        msg = "Error while xdelta3: %d" % result
        raise IOError(msg)
    return patch
    '''
    s_fd, s_path = tempfile.mkstemp(prefix="xdelta-")
    m_fd, m_path = tempfile.mkstemp(prefix="xdelta-")
    d_fd, d_path = tempfile.mkstemp(prefix="xdelta-")
    os.write(s_fd, source_data)
    os.write(m_fd, modi_data)
    diff_files(s_path, m_path, d_path)
    patch = open(d_path, "rb").read()
    os.close(s_fd)
    os.close(m_fd)
    os.close(d_fd)
    os.remove(s_path)
    os.remove(m_path)
    os.remove(d_path)

    return patch

    '''


def diff_files_custom(source_file, target_file, output_file, **kwargs):
    # kwargs
    # skip_validation   :   skipp sha1 validation
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    log = kwargs.get("log", None)
    nova_util = kwargs.get('nova_util', None)

    #sanity check
    if os.path.exists(source_file) == False or open(source_file, "rb") == None:
        raise IOError('[Error] No such file %s' % (source_file))
        return None
    if os.path.exists(target_file) == False or open(target_file, "rb") == None:
        raise IOError('[Error] No such file %s' % (target_file))
    if os.path.exists(output_file):
        os.remove(output_file)

    '''
    base_disk_hash = hashlist_from_file()
    base_mem_hash = hashlist_from_file()
    modified_mem_hash = extract_hashlist(open(modified_mem, "rb"))
    hash_for_disk = [(1, base_disk, base_disk_hash), (2, base_mem, base_mem_hash), (3, modified_mem, modified_mem_hash)]
    hash_for_mem = [(1, base_disk, base_disk_hash), (2, base_mem, base_mem_hash)]
    disk_deltalist = get_delta(open(modified_mem, "rb"), hash_for_disk)
    mem_deltalist = get_delta(open(modified_mem, "rb"), hash_for_mem)
    '''

def merge_files(source_file, overlay_file, output_file, **kwargs):
    # kwargs
    # skip_validation   :   skipp sha1 validation
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    log = kwargs.get("log", None)
    nova_util = kwargs.get('nova_util', None)

    fout = open(output_file, "wb")
    fout.close()
    if log:
        log.debug("merge: %s (%d)" % (source_file, os.path.getsize(source_file)))

    if nova_util:
        nova_util.execute("xdelta3", "-df", "-s", str(source_file), str(overlay_file), str(output_file))
        return 0
    else:
        command_patch = ["xdelta3", "-df", "-s", source_file, overlay_file, output_file]
        ret = subprocess.call(command_patch)
        if ret != 0:
            raise IOError('xdelta merge failed')
        return 0


def merge_data(source_data, overlay_data, buf_len):
    if len(source_data) == 0 or len(overlay_data) == 0:
        raise IOError("[Error] Not valid data length: %d, %d" % (len(source_data), len(overlay_data)))
    
    result, recover = xdelta3.xd3_decode_memory(overlay_data, source_data, buf_len)
    if result != 0:
        raise IOError("Error while xdelta3 : %d" % result)
    return recover 


def compare_same(filename1, filename2):
    print '[INFO] checking validity of generated file'
    compare = filecmp.cmp(filename1, filename2)
    if compare == False:
        print >> sys.stderr, '[ERROR] %s != %s' % (os.path.basename(filename1), os.path.basename(filename2))
        return False
    else:
        print '[INFO] SUCCESS to recover'
        return True


# lzma compression
def comp_lzma(inputname, outputname, **kwargs):
    # kwargs
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    log = kwargs.get("log", None)
    nova_util = kwargs.get('nova_util', None)

    prev_time = time()
    fin = open(inputname, 'rb')
    fout = open(outputname, 'wb')
    if nova_util:
        (stdout, stderr) = nova_util.execute('xz', '-9cv', process_input=fin.read())
        fout.write(stdout)
    else:
        ret = subprocess.call(['xz', '-9cv'], stdin=fin, stdout=fout)
        if ret:
            raise IOError('XZ compressor failed')

    fin.close()
    fout.close()
    time_diff = str(time()-prev_time)
    return outputname, str(time_diff)


# lzma decompression
def decomp_lzma(inputname, outputname, **kwargs):
    # kwargs
    # skip_validation   :   skipp sha1 validation
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess 
    #           will be returned without finishing their work
    log = kwargs.get("log", None)
    nova_util = kwargs.get('nova_util', None)

    prev_time = time()
    fin = open(inputname, 'rb')
    fout = open(outputname, 'wb')
    if nova_util:
        (stdout, stderr) = nova_util.execute('xz', '-d', process_input=fin.read())
        fout.write(stdout)
    else:
        ret = subprocess.call(['xz', '-d'], stdin=fin, stdout=fout)
        if ret:
            raise IOError('XZ decompressor failed')
    fin.close()
    fout.close()

    time_diff = str(time()-prev_time)
    return outputname, str(time_diff)


def decomp_overlay(meta, output_path, print_out=sys.stdout):
    meta_dict = msgpack.unpackb(open(meta, "r").read())
    decomp_start_time = time()
    comp_overlay_files = meta_dict[Const.META_OVERLAY_FILES]
    comp_overlay_files = [item[Const.META_OVERLAY_FILE_NAME] for item in comp_overlay_files]
    comp_overlay_files = [os.path.join(os.path.dirname(meta), item) for item in comp_overlay_files]
    overlay_file = open(output_path, "w+b")
    for comp_file in comp_overlay_files:
        decompressor = LZMADecompressor()
        comp_data = open(comp_file, "r").read()
        decomp_data = decompressor.decompress(comp_data)
        decomp_data += decompressor.flush()
        overlay_file.write(decomp_data)
    print_out.write("[Debug] Overlay decomp time for %d files: %f at %s\n" % \
            (len(comp_overlay_files), (time()-decomp_start_time), output_path))
    overlay_file.close()

    return meta_dict


def sha1_fromfile(file_path):
    if not os.path.exists(file_path):
        raise IOError("cannot find file while generating sha1")
    data = open(file_path, "r").read()
    s = sha1()
    s.update(data)
    return s.hexdigest()


def _chunking_fixed_size(in_stream, size):
    s_index = 0
    while True:
        data = in_stream.read(size)
        data_len = len(data)
        if data_len <= 0:
            break
        #print "%d %d %d" % (s_index, s_index+data_len, data_len)
        yield (s_index, s_index+data_len, data)
        s_index += data_len


def extract_hashlist(in_stream):
    global HASH_CHUNKING_SIZE
    hash_list = []
    for (start, end, data) in _chunking_fixed_size(in_stream, HASH_CHUNKING_SIZE): # 4Kbyte fixed chunking
        hash_list.append((sha256(data).digest(), start, end))

    from operator import itemgetter
    hash_list.sort(key=itemgetter(1,2))
    hashlist_statistics(hash_list)
    return hash_list


def hashlist_statistics(hash_list):
    # hash_list : list of (hash_value, start_index, end_index)
    # hash_list must be sorted 
    cur_hash = 0
    duplicated = 0
    unique_counter = 0
    unique_size = 0
    total_size = 0
    for (hash_value, s_index, e_index) in hash_list:
        total_size += (e_index-s_index)
        if cur_hash != hash_value:
            cur_hash = hash_value
            unique_counter += 1
            unique_size += (e_index-s_index)
        else:
            duplicated += 1

    print "total : %d, unique: %d, compress: %lf" % (total_size, unique_size, (1.0*unique_size/total_size))


def _search_matching(hash_value, hash_list, search_start_index):
    # search from the previous search point
    # good for search performance if list is already sorted
    for index, (value, s_offset, e_offset) in enumerate(hash_list[search_start_index:]):
        print "Search start at : %d(%s), %d(%s)" % (index, search_start_index)
        if hash_value == value:
            return (search_start_index+index, s_offset, e_offset)
        if hash_value > value:
            return (search_start_index+index, None, None)

    return None, None, None


def get_delta(in_stream, hash_lists):
    global HASH_CHUNKING_SIZE

    matching_count = 0
    total_count = 0
    latest_found = {}
    for ref_id, path, hast_list in hash_lists:
        latest_found[ref_id] = 0

    delta_result = []
    for (start, end, data) in _chunking_fixed_size(in_stream, HASH_CHUNKING_SIZE):
        total_count += 1
        hash_value = sha256(data).digest()

        if total_count%1000 == 0:
            done_size = total_count*HASH_CHUNKING_SIZE
            print "%d/%d , %lf percent" % (matching_count, total_count, 100.0*done_size/400000000)

        #find matching hash for each hash_list
        for ref_hashlist_id, path, hash_list in hash_lists:
            found_index , s_offset, e_offset = _search_matching(hash_value, hash_list, latest_found[ref_hashlist_id])
            if found_index:
                break

        # save ref if it is found
        if found_index:
            # save it with hash value
            delta_result.append((start, end, ref_hashlist_id, hash_value))
            latest_found[ref_hashlist_id] = found_index
            matching_count += 1
            #print "found from hast_list %d, at the %d" % (ref_hashlist_id, latest_found[ref_hashlist_id])
        else:
            delta_result.append((start, end, 0, data))

    print "matching %d/%d = %lf" % (matching_count, total_count, 1.0*matching_count/total_count)
    return delta_result 


def merge_delta(delta_list, hash_lists):
    global HASH_CHUNKING_SIZE

    ref_mmap = {}
    for (ref_id, filepath, hast_list) in hash_lists:
        f = open(filepath, "rb")
        ref_mmap[ref_id] = mmap.mmap(f.fileno(), 0, prot=mmap.PROT_READ)

    recover_data = ''
    # delta list is sorted by start_offset when saved it to file
    for (start_offset, end_offset, ref_id, data) in delta_list:
        if ref_id:
            for (id, path, hash_list) in hash_lists:
                if ref_id == id:
                    break
            index, s_offset, e_offset = _search_matching(data, hash_list, 0)
            ref_data = ref_mmap[ref_id][s_offset:e_offset]
            #print "from hash[%d]\t%d ~ %d : %d" % (ref_id, start_offset, end_offset, len(ref_data))
        else:
            ref_data = data
            #print "from data\t%d ~ %d : %d" % (start_offset, end_offset, len(ref_data))
        recover_data += ref_data
    return recover_data


def hashlist_to_file(hash_list, out_path):
    fd = open(out_path, "wb")

    # Write MAGIC & VERSION
    fd.write(struct.pack("<q", HASHFILE_MAGIC))
    fd.write(struct.pack("<q", HASHFILE_VERSION))

    for (data, start_offset, end_offset) in hash_list:
        # save it as little endian format
        row = struct.pack("<32sqq", data, start_offset, end_offset)
        fd.write(row)
    fd.close()


def hashlist_from_file(in_path):
    fd = open(in_path, "rb")
    hash_list = []

    # Write MAGIC & VERSION
    magic, version = struct.unpack("<qq", fd.read(8+8))
    if magic != HASHFILE_MAGIC or version != HASHFILE_VERSION:
        msg = "Hash file magic number(%ld), version(%ld) does not match" \
                % (HASHFILE_MAGIC, HASHFILE_VERSION)
        raise IOError(msg)

    while True:
        data = fd.read(32+8+8) # hash value, start_offset, end_offset
        if not data:
            break
        sha256, start_offset, end_offset = struct.unpack("<32sqq", data)
        hash_list.append((sha256, start_offset, end_offset))

    fd.close()
    return hash_list 

def deltalist_to_file(delta_list, out_path):
    # delta list format
    # list of (start_offset, end_offset, ref_hashlist_id, data)
    # data : hash value of reference hash list, if ref_hashlist_id exist
    #       real data, if ref_hashlist_id is None
    from operator import itemgetter
    delta_list.sort(key=itemgetter(1))  # sort by start_offset

    fd = open(out_path, "wb")
    for (start_offset, end_offset, ref_id, data) in delta_list:
        # save it as little endian format,
        header = struct.pack("<qqc", start_offset, end_offset, chr(ref_id))
        fd.write(header)
        fd.write(data)
    fd.close()


def deltalist_from_file(in_path):
    # delta list format
    # list of (start_offset, end_offset, ref_hashlist_id, data)
    # data : hash value of reference hash list, if ref_hashlist_id exist
    #       real data, if ref_hashlist_id is 0

    fd = open(in_path, "rb")
    delta_list = []
    while True:
        header = fd.read(8+8+1)
        if not header:
            break

        start_offset, end_offset, ref_hashlist_id = struct.unpack("<qqc", header)
        ref_hashlist_id = ord(ref_hashlist_id)
        if ref_hashlist_id:
            # get hash value, which is size of SHA256(==256 bit)
            data = fd.read(256/8)
            #print "hash, recovering : %d %d" % (start_offset, end_offset)
        else:
            # get real data, which is HASH_CHUNKING_SIZE
            data = fd.read(HASH_CHUNKING_SIZE)
            #print "data, recovering : %d %d" % (start_offset, end_offset)
        delta_list.append((start_offset, end_offset, ref_hashlist_id, data))

    fd.close()
    return delta_list


if __name__ == "__main__":
    import random
    import string

    if sys.argv[1] == "comp":
        base = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(2096))
        compressor = LZMACompressor(LZMA_OPTION)
        comp = compressor.compress(base)
        comp += compressor.flush()

        decompressor = LZMADecompressor()
        decomp = decompressor.decompress(comp)
        decomp += decompressor.flush()

        if base != decomp:
            print "result is wrong"
            print "%d == %d" % (len(base), len(decomp))
            sys.exit(1)
        print "success"

    elif sys.argv[1] == "xdelta":
        base = ''.join(random.choice(string.ascii_uppercase + string.digits) for x in range(4096))
        modi = "~"*4096
        patch = diff_data(base, modi, len(base))
        recover = merge_data(base, patch, len(base))
    
        if sha256(modi).digest() == sha256(recover).digest():
            print "SUCCESS"
            print len(patch)
        else:
            print "Failed %d == %d" % (len(modi), len(recover))

