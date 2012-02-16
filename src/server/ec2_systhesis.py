#!/usr/bin/env python
#
# EC2 synthesis is designed for synthesis approach for EC2 Cloudl Application
# Basic purpose of this script is preparing kexec by making customized VM
# at block device.
#
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
from optparse import OptionParser
from datetime import datetime
import subprocess
import sys
import tempfile


def copy_vmimage(mount_path, output_path):
    # Re-create directory
    if os.path.exists(output_path):
        os.removedirs(output_path)
    os.mkdir(output_path)

    # Copy
    cmd_rsync = "rsync -aHx %s/* %s" % (mount_path, output_path)
    subprocess.Popen(cmd_rsync, shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()

    # Unmount
    cmd_umount = "fusermount -u %s" % os.path.abspath(mount_path)
    print "umount %s" % cmd_umount
    subprocess.Popen(cmd_umount, shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()

def exec_kernel_reboot(new_rootfs):
    pass

def mount_vmimage(image_path, mount_path):
    if not os.path.exists(mount_path):
        os.mkdir(mount_path)
    cmd_mount = "guestmount --ro --format=qcow2 -a %s -i %s" % (os.path.abspath(image_path), os.path.abspath(mount_path))
    print cmd_mount
    subprocess.Popen(cmd_mount, shell=True, stdin=sys.stdin, stdout=sys.stdout).wait()

    # check boot directory
    boot_path = os.path.join(mount_path, 'boot')
    if not os.path.exists(boot_path):
        print >> sys.stderr, "Cannot find boot path or mount failed : %s" % (boot_path)
        sys.exit(2)
    return mount_path


def process_command_line(argv):
    help_message = "\nEC2 synthesis is designed for synthesis approach for EC2 Cloud\n" + \
            "Application Basic. purpose of this script is preparing kexec() by making\n" + \
            "customized VM at block device."

    parser = OptionParser(usage="usage: %prog -i [Disk Image] -o [Output Path]\n" + help_message,
            version="EC2 Synthesys 0.1")
    parser.add_option(
            '-i', '--input', action='store', type='string', dest='image_path',
            help='Set input Disk Image Path.')
    parser.add_option(
            '-o', '--output', action='store', type='string', dest='output_path',
            help='Set output file system path.\nMust be different from current Root file system')
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    if settings.output_path == None or settings.image_path == None:
        parser.error('program requires INPUT and OUTPUT')

    return settings, args


def main(argv=None):
    settings, args = process_command_line(sys.argv[1:])
    temp_dir = tempfile.mkdtemp()
    mount_path = mount_vmimage(settings.image_path, temp_dir)
    if not mount_path:
        print >> sys.stderr, "Cannot mount %s to temp directory %s" % (settings.image_path, temp_dir)
        sys.exit(2)
    copy_vmimage(mount_path, settings.output_path)
    exec_kernel_reboot(settings.output_path)
    return 0


if __name__ == "__main__":
    status = main()
    sys.exit(status)
