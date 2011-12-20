#!/usr/bin/env python
from datetime import datetime, timedelta
from cloudlet import run_snapshot

if __name__ == "__main__":
    tmp_disk = '/home/krha/Cloudlet/image/FACE_BaseVM/recover.qcow2'
    base_mem = '/home/krha/Cloudlet/image/FACE_BaseVM/recover.mem'
    telnet_port = 9999
    vnc_port = 2
    ret_time = run_snapshot(tmp_disk, base_mem, telnet_port, vnc_port, wait_vnc_end=False)
    print "Time for run Snapshot : ", ret_time
