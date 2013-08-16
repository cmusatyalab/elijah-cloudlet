#!/usr/bin/env python 
#
# Cloudlet Infrastructure for Mobile Computing
#
#   Author: Kiryong Ha <krha@cmu.edu>
#
#   Copyright (C) 2011-2013 Carnegie Mellon University
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

import os
import sys
from optparse import OptionParser

sort_key = ['local', 'cage', 'hail', 'east', 'west', 'eu', 'asia']


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
    summary = "%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f" % (rtt_sorted[0],
            rtt_sorted[int(total_rtt_number*0.01)], \
            rtt_sorted[int(total_rtt_number*0.25)], \
            rtt_sorted[int(total_rtt_number*0.5)], \
            rtt_sorted[int(total_rtt_number*0.75)], \
            rtt_sorted[int(total_rtt_number*0.99)], \
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
    global sort_key

    settings, args = process_command_line(sys.argv[1:])
    if settings.input_file and os.path.exists(settings.input_file):
        convert_to_CDF(settings.input_file, settings.input_file + ".cdf")
    elif settings.input_dir and len(os.listdir(settings.input_dir)) > 0 :
        summary_list = []
        cdf_all_list = []
        input_file_list = []
        for each_file in os.listdir(settings.input_dir):
            if os.path.isdir(os.path.join(settings.input_dir, each_file)):
                continue
            if each_file.find(".") != -1:
                continue
            input_file_list.append(each_file)

        # sort by keyword
        file_list = []
        counter = 0
        for key_word in sort_key:
            for each_file in input_file_list:
                if each_file.find(key_word) != -1:
                    counter += 1
                    file_list.append(each_file)
                    print "File : %s" % each_file

        for each_file in file_list:
            input_file = os.path.join(settings.input_dir, each_file)
            summary_str, cdf_list = convert_to_CDF(input_file, input_file + ".cdf")
            summary_list.append(summary_str)
            cdf_all_list.append(cdf_list)

        # print out all data
        print "="*50
        print "\tmin\t1%\t25%\t50%\t75%\t99%\tmax\tjitter\trun_time"
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
