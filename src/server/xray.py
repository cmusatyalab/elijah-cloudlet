#!/usr/bin/env python

import bson
import struct
import sys
import subprocess
import os
from tempfile import NamedTemporaryFile

XRAY_BIN = "./xray/disk_analyzer"

class FS_Analyzer(Exception):
    pass


def _analyze_fs(disk_path, bson_path):
    if os.path.exists(XRAY_BIN) == False:
        raise FS_Analyzer("Cannot find binary at %s" % XRAY_BIN);
    cmd = "%s %s %s" % (os.path.abspath(XRAY_BIN), os.path.abspath(disk_path), bson_path)
    _PIPE = subprocess.PIPE
    proc = subprocess.Popen(cmd, stdout=_PIPE, stderr=_PIPE, shell=True)
    out, err = proc.communicate()
    if proc.returncode > 0:
        raise FS_Analyzer("XRAY returned status %d" % proc.returncode)


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
def get_diff(image1, image2):
    # return:
    #   sector list of new_files
    #   modified sector list of modified_files
    for document in _bson_yielder(bson_file):
        if 'path' in document:
            path = (document['path']).encode("utf-8")
            inode = _iNode(document['inode'])
            if path.find("/home/cloudlet/") == 0:
                print "%s -> %s" % (path, inode.i_mtime)


def get_files_from_sectors(raw_path, sector_list):
    # returns file that is associated with give sector
    # return:
    #   sec_file_dict : dictionary with key(associated File), value(sector #)
    bson_file = NamedTemporaryFile(prefix="xray-bson", delete=False)
    _analyze_fs(raw_path, bson_file.name)
    sec_file_dict = dict([(sector, "Not Found") for sector in sector_list])
    for document in _bson_yielder(bson_file.name):
        if 'sectors' in document:
            path = (document['path']).encode("utf-8")
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

def get_deleted_files(image1, image2):
    pass

def get_modified_files(image1, image2):
    pass

def get_new_files(image1, image2):
    pass


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
    else:
        print "Cannot found command : %s" % command

