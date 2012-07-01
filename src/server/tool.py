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

def sha1_fromfile(file_path):
    if not os.path.exists(file_path):
        raise IOError("cannot find file while generating sha1")
    data = open(file_path, "r").read()
    s = sha1()
    s.update(data)
    return s.hexdigest()


if __name__ == "__main__":
    infile = sys.argv[1]
    outfile = sys.argv[2]
    comp_lzma(infile, infile+".lzma")
    decomp_lzma(infile+".lzma", outfile)

