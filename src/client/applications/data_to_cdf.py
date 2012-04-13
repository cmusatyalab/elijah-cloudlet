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
    print "filename: " + input_file
    input_lines = open(input_file, "r").read().split("\n")
    output_file = open(output_file, "w")
    rtt_list = []
    jitter_sum = 0.0
    start_time = 0.0
    end_time = 0.0
    for index, oneline in enumerate(input_lines):
        if len(oneline.split("\t")) != 6 and len(oneline.split("\t")) != 5:
            #sys.stderr.write("Error at input line at %d, %s\n" % (index, oneline))
            continue
        try:
            if float(oneline.split("\t")[2]) == 0:
                sys.stderr.write("Error at input line at %d, %s\n" % (index, oneline))
                continue
        except ValueError:
            continue
        try:

            rtt_list.append(float(oneline.split("\t")[3]))
            if not index == 0:
                # protect error case where initial jitter value is equals to latency
                jitter_sum += (float(oneline.split("\t")[4]))

            if start_time == 0.0:
                start_time = float(oneline.split("\t")[1])
            end_time = float(oneline.split("\t")[2])
        except ValueError:
            sys.stderr.write("Error at input line at %d, %s\n" % (index, oneline))
            continue

    rtt_sorted = sorted(rtt_list)
    total_rtt_number = len(rtt_sorted)
    cdf = []
    summary = "%f\t%f\t%f\t%f\t%f\t%f\t%f" % (rtt_sorted[0], rtt_sorted[int(total_rtt_number*0.25)], \
            rtt_sorted[int(total_rtt_number*0.5)], \
            rtt_sorted[int(total_rtt_number*0.75)], \
            rtt_sorted[-1], \
            jitter_sum/total_rtt_number, \
            (end_time-start_time))
    for index, value in enumerate(rtt_sorted):
        data = (value, 1.0 * (index+1)/total_rtt_number)
        cdf_string = "%f\t%f\n" % (data[0], data[1])
        output_file.write(cdf_string)
        cdf.append(data)
    return summary, cdf


def main(argv=None):
    global LOCAL_IPADDRESS
    settings, args = process_command_line(sys.argv[1:])
    if settings.input_file and os.path.exists(settings.input_file):
        convert_to_CDF(settings.input_file, settings.input_file + ".cdf")
    elif settings.input_dir and len(os.listdir(settings.input_dir)) > 0 :
        summary_list = []
        cdf_all_list = []
        file_list = []
        for each_file in os.listdir(settings.input_dir):
            print "File : %s" % each_file
            if os.path.isdir(os.path.join(settings.input_dir, each_file)):
                print "This is directory : %s" % each_file
                continue
            if each_file.find(".") != -1:
                continue
            file_list.append(each_file)

        for each_file in file_list:
            input_file = os.path.join(settings.input_dir, each_file)
            summary_str, cdf_list = convert_to_CDF(input_file, input_file + ".cdf")
            summary_list.append(summary_str)
            cdf_all_list.append(cdf_list)

        # print out all data
        print "="*50
        print "\tmin\t25%\t50%\t75%\tmax\tjitter\trun_time"
        for index, summary in enumerate(summary_list):
            print "%s\t%s" % (file_list[index], summary)
        print "\n"*2

        for each_file in file_list:
            sys.stdout.write("%s\t\t" % os.path.splitext(os.path.basename(each_file))[0])
        sys.stdout.write("\n")

        # Get longest CDF
        max_length = 0
        for cdf_ret in cdf_all_list:
            if len(cdf_ret) > max_length:
                max_length = len(cdf_ret)
            
        for index in xrange(max_length):
            for cdf_list in cdf_all_list:
                if len(cdf_list) > index:
                    sys.stdout.write("%f\t%f\t" % (cdf_list[index][0], cdf_list[index][1]))
                else:
                    sys.stdout.write("\t\t")
            sys.stdout.write("\n")

    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
