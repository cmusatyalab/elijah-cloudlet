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

import libvirt
import time
import ResourceConst as Const
import threading


class ResourceMonitorError(Exception):
    pass


class ResourceMonitorThread(threading.Thread):
    def __init__(self, log=None):
        self.stop = threading.Event()
        if log:
            self.log = log
        else:
            self.log = open("/dev/null", "w+b")

        # static machine info
        self.machine_info = self._get_static_resource()
        threading.Thread.__init__(self, target=self.monitor)

    def monitor(self):
        while (not self.stop.wait(0.01)):
            if self.stop.wait(10):
                break

    def _get_static_resource(self):
        conn = libvirt.open("qemu:///session")
        if not conn:
            return dict()
        machine_info = conn.getInfo()
        mem_total = machine_info[1]
        clock_speed = machine_info[3]
        number_socket = machine_info[5]
        number_cores = machine_info[6]
        number_threads_pcore = machine_info[7]

        info_dict = {
                Const.MACHINE_NUMBER_SOCKET : int(number_socket),
                Const.MACHINE_NUMBER_CORES_PSOCKET : int(number_cores),
                Const.MACHINE_NUMBER_THREADS_PCORE : int(number_threads_pcore),
                Const.MACHINE_NUMBER_TOTAL_CPU: int(number_socket*number_cores*number_threads_pcore),
                Const.MACHINE_CLOCK_SPEED: float(clock_speed),
                Const.MACHINE_MEM_TOTAL: long(mem_total),
                }
        return info_dict

    def get_static_resource(self):
        return self.machine_info

    def get_dynamic_resource(self):
        import psutil
        cpu_usage = float(psutil.cpu_percent())
        free_memory = long(psutil.virtual_memory()[1]/1024/1024)

        # disk io during 1 sec
        '''
        old_disk = psutil.disk_io_counters()
        time.sleep(1)
        new_disk = psutil.disk_io_counters()
        disk_read_bps = new_disk.read_bytes - old_disk.read_bytes
        disk_write_bps = new_disk.write_bytes - old_disk.write_bytes
        '''
        
        info_dict = {
                Const.TOTAL_CPU_USE_PERCENT: cpu_usage,
                Const.TOTAL_FREE_MEMORY: free_memory,
                #Const.TOTAL_DISK_READ_BPS: disk_read_bps,
                #Const.TOTAL_DISK_WRITE_BPS: disk_write_bps,
                }

        return info_dict

    def terminate(self):
        self.stop.set()


if __name__ == "__main__":
    from pprint import pprint
    monitor = ResourceMonitorThread()
    pprint(monitor.get_static_resource())
    pprint(monitor.get_dynamic_resource())
    


