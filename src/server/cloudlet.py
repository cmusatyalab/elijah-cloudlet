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

import lib_cloudlet as lib_cloudlet
import sys
import os
import time
from Configuration import Options
from optparse import OptionParser

def process_command_line(argv, commands):
    USAGE = 'Usage: %prog ' + ("[%s]" % "|".join(commands)) + " [base VM] [option] -- [qemu-options]\n"
    USAGE += "  EX) cloudlet.py base /path/to/disk.img"
    VERSION = '%prog 0.7'
    DESCRIPTION = 'Cloudlet Overlay Generation & Synthesis'

    parser = OptionParser(usage=USAGE, version=VERSION, description=DESCRIPTION)

    parser.add_option(
            '-t', '--no-trim', action='store_true', dest='disable_trim_support', default=False,
            help='This will disable TRIM Support, mainly for test purposes. \
                    Normal user does not need to care about this option')
    parser.add_option(
            '-m', '--no-free-memory', action='store_true', dest='disable_free_support', default=False,
            help='This will disable extracting Free memory, mainly for test purposes. \
                    Normal user does not need to care about this option')
    parser.add_option(
            '-d', '--disk', action='store_true', dest='disk_only', default=False,
            help='[overlay_creation] create only disk overlay only')
    settings, args = parser.parse_args(argv)

    if len(args) < 2:
        parser.error("Choose command among [%s] and [base vm path]" % "|".join(commands))
    mode = str(args[0]).lower()
    base_path = str(args[1])

    if mode not in commands:
        parser.error("%s is invalid mode. Choose among %s" % (mode, "|".join(commands)))
    if os.path.exists(base_path) != True:
        parser.error("%s does not exist. Need valid path for base disk" % (base_path))


    return mode, base_path, args[2:], settings


def main(argv):
    if not lib_cloudlet.validate_congifuration():
        sys.stderr.write("failed to validate configuration\n")
        sys.exit(1)

    CMD_BASE_CREATION       = "base"
    CMD_OVERLAY_CREATION    = "overlay"
    CMD_SYNTEHSIS           = "synthesis"

    command = (CMD_BASE_CREATION, CMD_OVERLAY_CREATION, CMD_SYNTEHSIS)
    mode, base_path, left_args, settings = process_command_line(sys.argv[1:], command)

    if mode == CMD_BASE_CREATION:
        # creat base VM
        disk_image_path = base_path
        if len(left_args) != 0:
            sys.stderr("Warning, qemu argument won't be applied to creating base vm")
        disk_path, mem_path = lib_cloudlet.create_baseVM(disk_image_path)
        print "Base VM is created from %s" % disk_image_path
        print "Disk: %s" % disk_path
        print "Mem: %s" % mem_path

    elif mode == CMD_OVERLAY_CREATION:
        # create overlay
        start_time = time.time()
        disk_path = base_path
        qemu_args = left_args
        options = Options()
        options.TRIM_SUPPORT = not settings.disable_trim_support
        options.FREE_SUPPORT = not settings.disable_free_support
        options.DISK_ONLY = settings.disk_only

        overlay_files = lib_cloudlet.create_overlay(disk_path, options, qemu_args=qemu_args)
        print "[INFO] overlay metafile : %s" % overlay_files[0]
        print "[INFO] overlay : %s" % str(overlay_files[1])
        #print "[INFO] overlay creation time: %f" % (time.time()-start_time)
    elif mode == CMD_SYNTEHSIS:
        if len(left_args) < 1:
            sys.stderr.write("Synthesis requires at least 2 arguments\n \
                    1) base-disk path\n \
                    2) overlay meta path\n")
            sys.exit(1)
        meta = left_args[0]
        qemu_args = left_args[1:]

        lib_cloudlet.synthesis(base_path, meta, settings.disk_only, qemu_args=qemu_args)

    return 0


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
