#!/usr/bin/env python
import xdelta3
import os
import commands
import filecmp
import sys
import getopt
from datetime import datetime, timedelta

ISR_PARCEL_FOLDER = 

def print_usage(prog_name):
    print 'usage: %s [log_file]' % (prog_name)

def main(argv):
    if len(argv) < 2:
        print_usage(os.path.basename(argv[0]))
        sys.exit(2)

    timelist = []
    for line in open(argv[1], 'r'):
        tokens = line.strip()[7:].split("-")
        description = tokens[0]
        timedata = tokens[1].strip()
        timelist.append(datetime.strptime(timedata, "%X.%f"))

    if len(timelist) != 8:
        print 'Log format changed'
        sys.exit(2)

    print 'Total time :\t\t' + str(timelist[7] - timelist[0])
    print 'Overlay transfer time :\t' + str(timelist[4] - timelist[1])
    print 'VM launch time:\t\t' + str(timelist[6] - timelist[5])

if __name__ == "__main__":
    main(sys.argv)
