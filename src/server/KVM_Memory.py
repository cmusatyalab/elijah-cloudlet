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
import sys
import os


class KVM_Memory(Object):
    pass


if __name__ == "__main__":
    memory_path = sys.args[1]
    KVM_Memroy meory = new KVM_Memory(memory_path)
    memory.hashing()
    hashlist = memory.get_hashlist()
    for item in hashlist:
        print item
