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

def diff_files(source_file, target_file, output_file):
    if os.path.exists(source_file) == False:
        print '[Error] No such file %s' % (source_file)
        return None
    if os.path.exists(target_file) == False:
        print '[Error] No such file %s' % (target_file)
        return None
    if os.path.exists(output_file):
        os.remove(output_file)

    print '[INFO] %s(base) - %s  =  %s' % (os.path.basename(source_file), os.path.basename(target_file), os.path.basename(output_file))
    command_delta = ['xdelta3', '-f', '-s', source_file, target_file, output_file]
    ret = xdelta3.xd3_main_cmdline(command_delta)
    if ret == 0:
        return output_file
    else:
        return None


def merge_files(source_file, overlay_file, output_file):
    #command_patch = ['xdelta3', '-df', '-s', source_file, overlay_file, output_file]
    # ret = xdelta3.xd3_main_cmdline(command_patch)
    command_patch = "xdelta3 -df -s %s %s %s" % (source_file, overlay_file, output_file)
    proc = subprocess.Popen(command_patch, shell=True)
    proc.wait()

    #print command_patch
    if proc.returncode == 0:
        #print "output : %s (%d)" % (output_file, os.path.getsize(output_file))
        return output_file
    else:
        return None


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
def comp_lzma(inputname, outputname):
    prev_time = time()
    fin = open(inputname, 'rb')
    fout = open(outputname, 'wb')
    ret = subprocess.call(['xz', '-9cv'], stdin=fin, stdout=fout)
    if ret:
        raise IOError('XZ compressor failed')
    time_diff = str(time()-prev_time)
    return outputname, str(time_diff)


# lzma decompression
def decomp_lzma(inputname, outputname):
    prev_time = time()
    fin = open(inputname, 'rb')
    fout = open(outputname, 'wb')
    ret = subprocess.call(['xz', '-d'], stdin=fin, stdout=fout)
    if ret:
        raise IOError('XZ decompressor failed')

    time_diff = str(time()-prev_time)
    return outputname, str(time_diff)


if __name__ == "__main__":
    infile = sys.argv[1]
    outfile = sys.argv[2]
    comp_lzma(infile, infile+".lzma")
    decomp_lzma(infile+".lzma", outfile)

