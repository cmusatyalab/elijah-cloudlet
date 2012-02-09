#!/usr/bin/env python
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
from datetime import datetime, timedelta
from cloudlet import run_snapshot

if __name__ == "__main__":
    tmp_disk = '/home/krha/Cloudlet/image/FACE_BaseVM/recover.qcow2'
    base_mem = '/home/krha/Cloudlet/image/FACE_BaseVM/recover.mem'
    telnet_port = 9999
    vnc_port = 2
    ret_time = run_snapshot(tmp_disk, base_mem, telnet_port, vnc_port, wait_vnc_end=False)
    print "Time for run Snapshot : ", ret_time
