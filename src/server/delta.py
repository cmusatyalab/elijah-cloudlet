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

import struct

def create_disk_overlay(overlay_path, meta_path, disk_path, m_chunk_list, chunk_size):
    meta_fd = open(meta_path, "wb")
    overlay_fd = open(overlay_path, "wb")
    disk_fd = open(disk_path, "rb")

    for index, chunk in enumerate(m_chunk_list):
        offset = int(chunk) * chunk_size
        disk_fd.seek(offset)
        data = disk_fd.read(chunk_size)
        meta_format = struct.pack("<QI", long(chunk), int(index*chunk_size))
        meta_fd.write(meta_format)
        overlay_fd.write(data)


def recover_disk_overlay(overlay_path, meta_path, resume_path, chunk_size):
    overlay_fd = open(overlay_path, "rb")
    meta_fd = open(meta_path, "rb")
    resumed_fd = open(resume_path, "wb")

    chunk_list = []
    while True:
        meta_data = meta_fd.read(8+4)
        if not meta_data:
            break;
        chunk, offset = struct.unpack("<QI", meta_data)
        chunk_list.append(chunk)
        overlay_fd.seek(offset)
        overlay_data = overlay_fd.read(chunk_size)
        if len(overlay_data) != chunk_size:
            raise IOError("recovered chunk is wrong")
        #print "write to offset(%ld)" % (chunk*CHUNK_SIZE)
        resumed_fd.seek(chunk*chunk_size)
        resumed_fd.write(overlay_data)

    # overlay chunk format: chunk_1:1,chunk_2:1,...
    overlay_map = ','.join(["%d:1"% chunk for chunk in chunk_list])
    return overlay_map
