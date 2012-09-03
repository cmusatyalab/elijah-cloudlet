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
import struct
from operator import itemgetter


DELTA_FILE_MAGIC = 0x1145511b
DELTA_FILE_VERSION = 0x00000001


class DeltaItem(object):
    REF_RAW = 0x00
    REF_XDELTA = 0x01
    REF_SELF = 0x02
    REF_BASE_DISK = 0x03
    REF_BASE_MEM = 0x04
    REF_OVERLAY_DISK = 0x05
    REF_OVERLAY_MEM =0x06

    # data exist only when ref_id is not xdelta
    def __init__(self, offset, offset_len, hash_value, ref_id, data_len=0, data=None):
        self.offset = offset
        self.offset_len = offset_len
        self.hash_value = hash_value
        self.ref_id = ref_id
        self.data_len = data_len
        self.data = data

    def __getitem__(self, item):
        return self.__dict__[item]

    def get_serialized(self):
        # offset        : unsigned long long
        # offset_length : unsigned int
        # ref_id        : unsigned char
        data = struct.pack("<QIc", \
                self.offset,
                self.offset_len,
                chr(self.ref_id))

        if self.ref_id == DeltaItem.REF_RAW or \
               self.ref_id == DeltaItem.REF_XDELTA:
            data += struct.pack("<Q", self.data_len)
            data += struct.pack("<%ds" % self.data_len, self.data)
        elif self.ref_id == DeltaItem.REF_SELF:
            data += struct.pack("<Q", self.data)
        elif self.ref_id == DeltaItem.REF_BASE_DISK or \
                self.ref_id == DeltaItem.REF_BASE_MEM or \
                self.ref_id == DeltaItem.REF_OVERLAY_DISK or \
                self.ref_id == DeltaItem.REF_OVERLAY_MEM:
            data += struct.pack("<Q", self.data)
        return data

    @staticmethod
    def unpack_stream(stream):
        data = stream.read(8 + 4 + 1)
        data_len = 0
        if not data:
            return None

        (offset, offset_len, ref_id) = \
                struct.unpack("<QIc", data)
        ref_id = ord(ref_id)
        if ref_id == DeltaItem.REF_RAW or \
                ref_id == DeltaItem.REF_XDELTA:
            data_len = struct.unpack("<Q", stream.read(8))[0]
            data = stream.read(data_len)
        elif ref_id == DeltaItem.REF_SELF:
            data = struct.unpack("<Q", stream.read(8))[0]
        elif ref_id == DeltaItem.REF_BASE_DISK or \
                ref_id == DeltaItem.REF_BASE_MEM or \
                ref_id == DeltaItem.REF_OVERLAY_DISK or \
                ref_id == DeltaItem.REF_OVERLAY_MEM:
            data = struct.unpack("<Q", stream.read(8))[0]

        # hash value does not exist when recovered
        item = DeltaItem(offset, offset_len, None, ref_id, data_len, data)
        return item


class DeltaList(object):
    @staticmethod
    def tofile(header_delta, footer_delta, delta_list, f_path):
        if (not header_delta)or (not footer_delta):
            raise MemoryError("header/footer delta is invalid")
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        fd = open(f_path, "wb")
        # Write MAGIC & VERSION
        fd.write(struct.pack("<q", DELTA_FILE_MAGIC))
        fd.write(struct.pack("<q", DELTA_FILE_VERSION))

        # Write Header & Footer delta
        fd.write(struct.pack("<q", len(header_delta)))
        fd.write(header_delta)
        fd.write(struct.pack("<q", len(footer_delta)))
        fd.write(footer_delta)

        # Write list if delta item
        for item in delta_list:
            # save it as little endian format
            fd.write(item.get_serialized())
        fd.close()

    @staticmethod
    def fromfile(f_path):
        delta_list = []
        # MAGIC & VERSION
        fd = open(f_path, "rb")
        magic, version = struct.unpack("<qq", fd.read(8+8))
        if magic != DELTA_FILE_MAGIC or version != DELTA_FILE_VERSION:
            msg = "delta magic number(%x != %x), version(%ld != %ld) does not match" \
                    % (DELTA_FILE_MAGIC, magic, \
                    DELTA_FILE_VERSION, version)
            raise IOError(msg)

        # Read Header & Footer delta
        header_len = struct.unpack("<q", fd.read(8))[0]
        header_delta = fd.read(header_len)
        footer_len = struct.unpack("<q", fd.read(8))[0]
        footer_delta = fd.read(footer_len)
        while True:
            new_item = DeltaItem.unpack_stream(fd)
            if not new_item:
                break
            delta_list.append(new_item)
        fd.close()
        return header_delta, footer_delta, delta_list 

    @staticmethod
    def get_self_delta(delta_list):
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        # delta_list : list of (start, end, ref_id, hash/data)
        delta_list.sort(key=itemgetter('hash_value', 'offset')) # sort by (hash/start offset)

        pivot = delta_list[0]
        matching = 0
        for delta_item in delta_list[1:]:
            if delta_item.hash_value == pivot.hash_value:
                if delta_item.ref_id == DeltaItem.REF_XDELTA or delta_item.ref_id == DeltaItem.REF_RAW:
                    # same data/hash
                    # save reference start offset
                    delta_item.ref_id = DeltaItem.REF_SELF
                    delta_item.data_len = 8
                    delta_item.data = long(pivot.offset)
                    matching += 1
                    continue
            pivot=delta_item

        print "[Debug] self delta : %ld/%ld" % (matching, len(delta_list))
        delta_list.sort(key=itemgetter('offset'))

    @staticmethod
    def statistics(delta_list, print_out=sys.stdout):
        if len(delta_list) == 0 or type(delta_list[0]) != DeltaItem:
            raise MemoryError("Need list of DeltaItem")

        from_self = 0
        from_raw = 0
        from_base_disk = 0
        from_base_mem = 0
        from_xdelta = 0
        from_overlay_disk = 0
        from_overlay_mem = 0
        for delta_item in delta_list:
            if delta_item.ref_id == DeltaItem.REF_SELF:
                from_self += 1
            elif delta_item.ref_id == DeltaItem.REF_BASE_DISK:
                from_base_disk += 1
            elif delta_item.ref_id == DeltaItem.REF_BASE_MEM:
                from_base_mem += 1
            elif delta_item.ref_id == DeltaItem.REF_OVERLAY_DISK:
                from_overlay_disk += 1
            elif delta_item.ref_id == DeltaItem.REF_OVERLAY_MEM:
                from_overlay_mem += 1
            elif delta_item.ref_id == DeltaItem.REF_XDELTA:
                from_xdelta += 1
            elif delta_item.ref_id == DeltaItem.REF_RAW:
                from_raw += 1

        print_out.write("[INFO] Total Modified page #\t:%ld\n" % len(delta_list))
        print_out.write("[INFO] Saved as RAW\t\t:%ld\n" % from_raw)
        print_out.write("[INFO] Saved by xdelta3\t\t:%ld\n" % from_xdelta)
        print_out.write("[INFO] Shared within Self\t:%ld\n" % from_self)
        print_out.write("[INFO] Shared with Base Disk\t:%ld\n" % from_base_disk)
        print_out.write("[INFO] Shared with Base Mem\t:%ld\n" % from_base_mem)
        print_out.write("[INFO] Shared with Overlay Disk\t:%ld\n" % from_overlay_disk)
        print_out.write("[INFO] Shared with Overlay Mem\t:%ld\n" % from_overlay_mem)


