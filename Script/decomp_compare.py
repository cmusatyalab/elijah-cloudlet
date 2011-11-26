#!/usr/bin/env python

import pylzma
from datetime import datetime, timedelta
import sys, commands, os

def comp_lzma(inputname, outputname):
    prev_time = datetime.now()
    in_file = open(inputname, 'rb')
    ret_file = open(outputname, 'wb')
    c_fp = pylzma.compressfile(in_file, eos=1, algorithm=2)
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
    out_name, time_str, bw_str = decomp_lzma(tmp_name, outputname)
    print 'lzma decompression time : %s, %s ' % (time_str, bw_str)
    tmp_name, time_str, bw_str = comp_gzip(inputname, inputname + ".gz")
    print 'gzip compression time : %s, %s' % (time_str, bw_str)
    out_name, time_str, bw_str = decomp_gzip(tmp_name, outputname)
    print 'gzip decompression time : %s, %s ' % (time_str, bw_str)
    sys.exit(0)
 
