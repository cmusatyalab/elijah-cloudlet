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
from optparse import OptionParser


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog [option]", version="MOPED Desktop Client")
    parser.add_option(
            '-i', '--input', action='store', type='string', dest='input_file',
            help='Set Input file')
    parser.add_option(
            '-d', '--dir', action='store', type='string', dest='input_dir',
            help='Set Input directory')
    parser.add_option(
            '-o', '--output', action='store', type='int', dest='output_file',
            help='Set output file')
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    
    return settings, args

def convert_to_CDF(input_file, output_file):
    input_lines = open(input_file, "r").read().split("\n")
    rtt_list = []
    jitter_sum = 0.0
    start_time = 0.0
    end_time = 0.0
    for index, oneline in enumerate(input_lines):
        if len(oneline.split("\t")) != 6:
            print "Error at input line at %d, %s" % (index, oneline)
            continue
        try:

            rtt_list.append(float(oneline.split("\t")[3]) * 1000)
            jitter_sum += (float(oneline.split("\t")[4])*1000)

            if start_time == 0.0:
                start_time = float(oneline.split("\t")[1]) * 1000
            end_time = float(oneline.split("\t")[2]) * 1000
        except ValueError:
            print "Error at input line at %d, %s" % (index, oneline)
            continue

    rtt_sorted = sorted(rtt_list)
    total_rtt_number = len(rtt_sorted)
    cdf = []
    print "="*50
    print "min\t25%\t50%\t75%\tmax\tjitter\trun_time"
    print "%014.2f\t%014.2f\t%014.2f\t%014.2f\t%014.2f\t%014.2f\t%014.2f" % (rtt_sorted[0], rtt_sorted[int(total_rtt_number*0.25)], \
            rtt_sorted[int(total_rtt_number*0.5)], \
            rtt_sorted[int(total_rtt_number*0.75)], \
            rtt_sorted[-1], \
            jitter_sum/total_rtt_number, \
            (end_time-start_time))
    print "="*50
    for index, value in enumerate(rtt_sorted):
        data = (value, 1.0 * (index+1)/total_rtt_number)
        print "%7f\t%4.4f" % (data[0], data[1])
        cdf.append(data)


def main(argv=None):
    global LOCAL_IPADDRESS
    settings, args = process_command_line(sys.argv[1:])
    if settings.input_file and os.path.exists(settings.input_file):
        convert_to_CDF(settings.input_file, settings.input_file + ".cdf")

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
