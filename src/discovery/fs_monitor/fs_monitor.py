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

import sys
import os
import pprint
import datetime

def get_modified_files(dirname, prev_time):
    if os.path.isdir(dirname) != True:
        sys.stderr.write("%s is not a valid directory\n" % dirname)
        return None

    filelist = list()
    for root, dirnames, filenames in os.walk(dirname):
        for filename in filenames:
            path = os.path.join(root, filename)
            st = os.stat(path)
            mtime = datetime.datetime.fromtimestamp(st.st_mtime)
            if mtime > prev_time:
                print('%s \t\t - modified %s' % (path, mtime))
                filelist.append((path, "%s" % mtime))
        if filenames == None or len(filenames) == 0:
            path = os.path.join(root, ".")
            st = os.stat(path)
            mtime = datetime.datetime.fromtimestamp(st.st_mtime)
            if mtime > prev_time:
                filelist.append((path, "%s" % mtime))

    return filelist
    

def main(argv):
    if len(argv) != 2:
        sys.stderr.write("Need path to the monitoring directory\n")
        return 1

    dirname = os.path.abspath(argv[1])
    prev_time = datetime.datetime.now() #- datetime.timedelta(minutes=300)
    raw_input("Enter to proceed")
    new_files = get_modified_files(dirname, prev_time)
    pprint.pprint(new_files)
    return 0

if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
