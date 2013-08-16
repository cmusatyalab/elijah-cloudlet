#!/usr/bin/env python 
#
# Cloudlet Infrastructure for Mobile Computing
#
#   Author: Kiryong Ha <krha@cmu.edu>
#
#   Copyright (C) 2011-2013 Carnegie Mellon University
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

from __future__ import division
import os
import io
import struct
from cloudlet import log as logging

LOG = logging.getLogger(__name__)
import subprocess


class MachineGenerationError(Exception):
    pass


# File handle arguments don't need more than two letters
# pylint: disable=C0103
class _QemuMemoryHeader(object):
    HEADER_MAGIC = 'LibvirtQemudSave'
    HEADER_VERSION = 2
    # Header values are stored "native-endian".  We only support x86, so
    # assume we don't need to byteswap.
    HEADER_FORMAT = str(len(HEADER_MAGIC)) + 's19I'
    HEADER_LENGTH = struct.calcsize(HEADER_FORMAT)
    HEADER_UNUSED_VALUES = 15

    COMPRESS_RAW = 0
    COMPRESS_XZ = 3
    COMPRESS_CLOUDLET = 4

    EXPECTED_HEADER_LENGTH = 4096*2

    def __init__(self, f):
        # Read header struct
        f.seek(0)
        buf = f.read(self.HEADER_LENGTH)
        header = list(struct.unpack(self.HEADER_FORMAT, buf))
        magic = header.pop(0)
        version = header.pop(0)
        self._xml_len = header.pop(0)
        self.was_running = header.pop(0)
        self.compressed = header.pop(0)

        # Check header
        if magic != self.HEADER_MAGIC:
            raise MachineGenerationError('Invalid memory image magic')
        if version != self.HEADER_VERSION:
            raise MachineGenerationError('Unknown memory image version %d' %
                    version)
        if header != [0] * self.HEADER_UNUSED_VALUES:
            raise MachineGenerationError('Unused header values not 0')

        libvirt_header_len = self._xml_len + self.HEADER_LENGTH
        if libvirt_header_len != self.EXPECTED_HEADER_LENGTH:
            LOG.warning("libvirt header length is not aligned with 8KB")

        # Read XML, drop trailing NUL padding
        self.xml = f.read(self._xml_len - 1).rstrip('\0')
        if f.read(1) != '\0':
            raise MachineGenerationError('Missing NUL byte after XML')

    def seek_body(self, f):
        f.seek(self.HEADER_LENGTH + self._xml_len)

    def write(self, f):
        # Calculate header
        header = [self.HEADER_MAGIC,
                self.HEADER_VERSION,
                self._xml_len,
                self.was_running,
                self.compressed]
        header.extend([0] * self.HEADER_UNUSED_VALUES)

        # Write data
        f.seek(0)
        f.write(struct.pack(self.HEADER_FORMAT, *header))
        f.write(struct.pack('%ds' % self._xml_len, self.xml))
# pylint: enable=C0103

    def overwrite(self, f, new_xml):
        # Calculate header
        if len(new_xml) > self._xml_len - 1:
            # If this becomes a problem, we could write out a larger xml_len,
            # though this must be page-aligned.
            raise MachineGenerationError('self.xml is too large')
        header = [self.HEADER_MAGIC,
                self.HEADER_VERSION,
                self._xml_len,
                self.was_running,
                self.compressed]
        header.extend([0] * self.HEADER_UNUSED_VALUES)

        # Write data
        f.seek(0)
        f.write(struct.pack(self.HEADER_FORMAT, *header))
        f.write(struct.pack('%ds' % self._xml_len, new_xml))
# pylint: enable=C0103


class _QemuMemoryHeaderData(_QemuMemoryHeader):
    HEADER_MAGIC_PARTIAL = 'LibvirtQemudPart'

    def __init__(self, data):
        # Read header struct
        buf = data[:self.HEADER_LENGTH]
        header = list(struct.unpack(self.HEADER_FORMAT, buf))
        magic = header.pop(0)
        version = header.pop(0)
        self._xml_len = header.pop(0)
        self.was_running = header.pop(0)
        self.compressed = header.pop(0)

        # Check header
        if magic != self.HEADER_MAGIC and magic != self.HEADER_MAGIC_PARTIAL:
            # libvirt replace magic_partial to magic after finishing saving
            msg = 'Invalid memory image magic'
            LOG.error(msg)
            raise MachineGenerationError(msg)
        if version != self.HEADER_VERSION:
            msg = 'Unknown memory image version %d' % version
            LOG.error(msg)
            raise MachineGenerationError(msg)
        if header != [0] * self.HEADER_UNUSED_VALUES:
            msg = 'Unused header values not 0'
            LOG.error(msg)
            raise MachineGenerationError(msg)

        # Read XML, drop trailing NUL padding
        self.xml = data[self.HEADER_LENGTH:self.HEADER_LENGTH+self._xml_len]
        if self.xml[-1] != '\0':
            raise MachineGenerationError('Missing NUL byte after XML')

    def get_aligned_header(self, expected_header_size):
        current_size = self.HEADER_LENGTH + len(self.xml)
        padding_size = expected_header_size - current_size
        if padding_size < 0:
            msg = "WE FIXED LIBVIRT HEADER SIZE TO 2*4096\n" + \
                    "But given xml size is bigger than 2*4096"
            raise MachineGenerationError(msg)
        elif padding_size > 0:
            new_xml = self.xml + ("\0" * padding_size)
            self._xml_len = len(new_xml)
            self.xml = new_xml
        return self.get_header()

    def get_header(self):
        # Calculate header
        header = [self.HEADER_MAGIC,
                self.HEADER_VERSION,
                self._xml_len,
                self.was_running,
                self.compressed]
        header.extend([0] * self.HEADER_UNUSED_VALUES)

        # Write data
        header_binary = struct.pack(self.HEADER_FORMAT, *header)
        header_binary += struct.pack('%ds' % self._xml_len, self.xml)
        return header_binary


def copy_memory(in_path, out_path, xml):
    # Recompress if possible
    fin = open(in_path)
    fout = open(out_path, 'w')

    # Write header to output
    hdr = _QemuMemoryHeader(fin)
    hdr._xml_len = len(xml)
    hdr.xml = xml
    hdr.write(fout)

    # move fin position to data
    hdr = _QemuMemoryHeader(fin)
    hdr.seek_body(fin)

    # Write body
    while True:
        data = fin.read(1000*1000*10)
        if len(data) == 0:
            break
        fout.write(data)
    fout.flush()


def copy_disk(in_path, out_path):
    print 'Copying and compressing disk image...'
    if subprocess.call(['qemu-img', 'convert', '-cp', '-O', 'qcow2',
            in_path, out_path]) != 0:
        raise MachineGenerationError('qemu-img failed')

