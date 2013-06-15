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
