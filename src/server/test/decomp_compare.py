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

import pylzma
from datetime import datetime, timedelta
import sys
import commands
import os

def comp_lzma(inputname, outputname):
    # comparable with 'xz -7 [filename]', which uses 200 MB Dictionary'
    # original 723218356, this: 145385481, xz: 143296700
    prev_time = datetime.now()
    in_file = open(inputname, 'rb')
    ret_file = open(outputname, 'wb')
    c_fp = pylzma.compressfile(in_file, eos=1, algorithm=2, dictionary=28)
    while True:
        chunk = c_fp.read(8192)
        if not chunk: break
        ret_file.write(chunk)
    in_file.close()
    ret_file.close()
    time_diff = (datetime.now()-prev_time)
    if time_diff.seconds == 0:
        return outputname, str(time_diff), '-1'
    else:
        return outputname, str(time_diff), str(os.path.getsize(inputname)/time_diff.seconds)


def decomp_lzma(inputname, outputname):
    prev_time = datetime.now()
    comp_file = open(inputname, 'rb')
    ret_file = open(outputname, 'wb')
    obj = pylzma.decompressobj()
    while True:
        tmp = comp_file.read(8192)
        if not tmp: break
        ret_file.write(obj.decompress(tmp))
    ret_file.write(obj.flush())
    comp_file.close()
    ret_file.close()
    time_diff = (datetime.now()-prev_time)
    if time_diff.seconds == 0:
        return outputname, str(time_diff), '-1'
    else:
        return outputname, str(time_diff), str(os.path.getsize(inputname)/time_diff.seconds)


def comp_gzip(inputname, outputname):
    prev_time = datetime.now()
    comp_file = open(inputname, 'rb')
    ret_file = open(outputname, 'wb')
    cmd_str = 'gzip -c ' + inputname + ' > ' + outputname
    #print cmd_str
    ret = commands.getoutput(cmd_str)
    comp_file.close()
    ret_file.close()
    time_diff = (datetime.now()-prev_time)
    if time_diff.seconds == 0:
        return outputname, str(time_diff), '-1'
    else:
        return outputname, str(time_diff), str(os.path.getsize(inputname)/time_diff.seconds)


def decomp_gzip(inputname, outputname):
    prev_time = datetime.now()
    decomp_file = open(inputname, 'rb')
    ret_file = open(outputname, 'wb')
    cmd_str = 'gzip -cd ' + inputname + ' > ' + outputname
    #print cmd_str
    ret = commands.getoutput(cmd_str)
    decomp_file.close()
    ret_file.close()
    time_diff = (datetime.now()-prev_time)
    if time_diff.seconds == 0:
        return outputname, str(time_diff), '-1'
    else:
        return outputname, str(time_diff), str(os.path.getsize(inputname)/time_diff.seconds)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print "%s [infile] [outputfile]" % (sys.argv[0])
        sys.exit(2)
    inputname = sys.argv[1]
    outputname = sys.argv[2]

    tmp_name, time_str, bw_str = comp_lzma(inputname, inputname + ".lzma")
    print 'lzma compression time : %s, %s' % (time_str, bw_str)
    '''
    out_name, time_str, bw_str = decomp_lzma(tmp_name, outputname)
    print 'lzma decompression time : %s, %s ' % (time_str, bw_str)
    tmp_name, time_str, bw_str = comp_gzip(inputname, inputname + ".gz")
    print 'gzip compression time : %s, %s' % (time_str, bw_str)
    out_name, time_str, bw_str = decomp_gzip(tmp_name, outputname)
    print 'gzip decompression time : %s, %s ' % (time_str, bw_str)
    '''
    sys.exit(0)
 
