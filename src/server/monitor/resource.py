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
import psutil
import time
from optparse import OptionParser
import libvirt
from Const import Const as Const
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
            pass
        pass

    def _get_static_resource(self):
        # CPU
        res = open('/proc/cpuinfo').read()
        clock = [item for item in res.split("\n") if item.find("cpu MHz") == 0][0].split(": ")[1]
        #n_proc = [item for item in res.split("\n") if item.find("cpu cores") == 0][0].split(": ")[1]
        #n_socket = len(set([item for item in res.split("\n") if item.find("physical") == 0]))
        #cache_size = [item for item in res.split("\n") if item.find("cache size") == 0][0].split(": ")[1][:-3]
        #cache_block_size = [item for item in res.split("\n") if item.find("cache_alignment") == 0][0].split(": ")[1]

        # Memory
        mem = open('/proc/meminfo').read().split("\n")
        total_mem = 0
        for line in mem:
            if line.find("MemTotal") == 0:
                total_mem = long(line.split(":")[1].strip()[:-3])

        info_dict = {
                Const.MACHINE_NUMBER_TOTAL_CPU: int(psutil.NUM_CPUS),
                Const.MACHINE_CLOCK_SPEED: float(clock),
                Const.MACHINE_MEM_TOTAL: long(total_mem),
                }
        return info_dict

    def get_static_resource(self):
        return self.machine_info


    def get_dynamic_resource(self):
        info_dict = {
                Const.MACHINE_NUMBER_TOTAL_CPU: int(psutil.NUM_CPUS),
                Const.MACHINE_CLOCK_SPEED: float(clock),
                Const.MACHINE_MEM_TOTAL: long(total_mem),
                }
        return info_dict

    def terminate(self):
        self.stop.set()


if __name__ == "__main__":
    from pprint import pprint
    monitor = ResourceMonitorThread()
    pprint(monitor.get_static_resource())
    pprint(monitor.get_dynamic_resource())
    


