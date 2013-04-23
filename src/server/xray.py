#!/usr/bin/env python

import bson
import struct
import sys
import subprocess
import os
from tempfile import NamedTemporaryFile
from Configuration import Const


class xrayError(Exception):
    pass


def _analyze_fs(disk_path, bson_path):
    XRAY_BIN = Const.XRAY_BIN_PATH
    if os.path.exists(XRAY_BIN) == False:
         raise xrayError("Cannot find binary at %s" % XRAY_BIN);
    cmd = [
            "%s" % os.path.abspath(XRAY_BIN),
            "%s" % os.path.abspath(disk_path),
            "%s" % bson_path,
        ]
    _PIPE = subprocess.PIPE
    proc = subprocess.Popen(cmd, stdout=_PIPE, stderr=_PIPE, close_fds=True)
    out, err = proc.communicate()
    if proc.returncode > 0:
         raise xrayError("XRAY returned status %d" % proc.returncode)


# Magic function loading packed BSON documents from a file
def _bson_yielder(fname):
    fd = open(fname, "rb")
    try:
        while True:
            buf = ''
            buf += fd.read(4)
            doc_size, = struct.unpack("<I", buf)
            buf += fd.read(doc_size - 4)
            yield bson.loads(buf)
    except:
        raise StopIteration()


def _bson_last_element(fname):
    fd = open(fname, "rb")
    file_size = os.path.getsize(fname)
    last_bson = None
    try:
        while True:
            buf = ''
            buf += fd.read(4)
            doc_size, = struct.unpack("<I", buf)
            position = fd.tell()
            fd.seek((doc_size-4), os.SEEK_CUR)
            if fd.tell() == file_size:
                fd.seek(position, os.SEEK_SET)
                buf += fd.read(doc_size - 4)
                last_bson = bson.loads(buf)
    except:
        return last_bson


class _iNode(object):
    def __init__(self, inode_raw):
        unpacked = struct.unpack("<hhIIIII", inode_raw[:24])
        self.i_mode = unpacked[0]
        self.i_uid =  unpacked[1]
        self.i_size_lo =  unpacked[2]
        self.i_atime =  unpacked[3]
        self.i_ctime =  unpacked[4]
        self.i_mtime = unpacked[5]
        self.i_dtime = unpacked[6]


#public method
def get_used_blocks(raw_path):
    # get used sector dictionary using xray
    bson_file = NamedTemporaryFile(prefix="xray-bson", delete=False)
    _analyze_fs(raw_path, bson_file.name)
    ret_dict = dict()
    document = _bson_last_element(bson_file.name)
    if document['type'] and document['type'] == 'used_sectors':
        used_sectors = document['sectors']
        for sec in used_sectors:
            if sec%8 == 0:
                ret_dict[sec] = True
    else:
        raise xrayError("cannot find document at last element")
    return ret_dict


def get_files_from_sectors(raw_path, sector_list):
    # returns file that is associated with give sector
    # return:
    #   sec_file_dict : dictionary with key(associated File), value(sector #)
    bson_file = NamedTemporaryFile(prefix="xray-bson")
    _analyze_fs(raw_path, bson_file.name)
    sec_file_dict = dict([(sector, "Not Found") for sector in sector_list])
    for document in _bson_yielder(bson_file.name):
        if ('sectors' in document) and (document['type'] != 'used_sectors'):
            if document.get('path', None) != None:
                path = (document['path']).encode("utf-8")
            elif document.get('type', None) != None:
                path = (document['type']).encode("utf-8")
            sectors = document['sectors']
            for sector in sectors:
                if (sec_file_dict.get(sector, None)) != None:
                    sec_file_dict[sector] = path

    # reverse key-value: ret_dict[path] = list(sectors)
    ret_dict = dict()
    for sector, path in sec_file_dict.iteritems():
        path_sector_list = ret_dict.get(path)
        if path_sector_list:
            path_sector_list.append(sector)
        else:
            ret_dict[path] = [sector]
    return ret_dict

if __name__ == '__main__':
    if (len(sys.argv) != 3):
        print "program [command] bson"
        sys.exit(1)

    command = sys.argv[1]
    disk_path = sys.argv[2]

    print 'Analyzing disk file: %s' % disk_path
    if command == 'sectors':
        # get files that has specific sector
        sectors = [330224, 268976, 544632]
        sec_file_dict = get_files_from_sectors(disk_path, sectors)
        import pprint
        pprint.pprint(sec_file_dict)
    elif command == "discard":
        sectors = [330224, 268976, 544632, 1]
        import time
        start_time = time.time()
        used_dict = get_used_blocks(disk_path)
        for sector in sectors:
            if used_dict.get(sector, 0) == True:
                print "%d is used" % sector
            else:
                print "%d is not used" % sector
        end_time = time.time()
        print "computation time: %d" % (end_time-start_time)
    elif command == "verify":
        used_sec_dic = get_used_blocks(disk_path)
        sectors = list(used_sec_dic)
        sec_file_dict = get_files_from_sectors(disk_path, sectors)
        log_file = open("xray_verify", "w+b")
        import pprint
        pprint.pprint(sec_file_dict, log_file)
        log_file.close()


    else:
        print "Cannot found command : %s" % command

