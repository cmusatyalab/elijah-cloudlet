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

import os

class ConstError(Exception):
    pass


class Const(object):
    TRIM_SUPPORT            = True
    FREE_SUPPORT            = True
    XRAY_SUPPORT            = False

    BASE_DISK               = ".base-img"
    BASE_MEM                = ".base-mem"
    BASE_DISK_HASH          = ".base-img-hash"
    BASE_DISK_META          = ".base-img-meta"
    BASE_MEM_META           = ".base-mem-meta"
    OVERLAY_META            = ".overlay-meta"
    OVERLAY_FILE_PREFIX     = ".overlay"
    OVERLAY_LOG             = ".overlay-log"
    OVERLAY_BLOB_SIZE_KB    = 1024*1024 # 1G

    META_BASE_VM_SHA256                 = "base_vm_sha256"
    META_RESUME_VM_DISK_SIZE            = "resumed_vm_disk_size"
    META_RESUME_VM_MEMORY_SIZE          = "resumed_vm_memory_size"
    META_OVERLAY_FILES                  = "overlay_files"
    META_OVERLAY_FILE_NAME              = "overlay_name"
    META_OVERLAY_FILE_SIZE              = "overlay_size"
    META_OVERLAY_FILE_DISK_CHUNKS       = "disk_chunk"
    META_OVERLAY_FILE_MEMORY_CHUNKS     = "memory_chunk"

    TEMPLATE_XML = "./config/VM_TEMPLATE.xml"
    TEMPLATE_OVF = "./config/ovftransport.iso"
    VMNETFS_PATH = "../vmnetx/vmnetfs/vmnetfs"
    CHUNK_SIZE=4096

    @staticmethod
    def _check_path(name, path):
        if not os.path.exists(path):
            message = "Cannot find name at %s" % (path)
            raise ConstError(message)

    @staticmethod
    def get_basepath(base_disk_path, check_exist=False):
        Const._check_path('base disk', base_disk_path)

        image_name = os.path.splitext(base_disk_path)[0]
        dir_path = os.path.dirname(base_disk_path)
        diskmeta = os.path.join(dir_path, image_name+Const.BASE_DISK_META)
        mempath = os.path.join(dir_path, image_name+Const.BASE_MEM)
        memmeta = os.path.join(dir_path, image_name+Const.BASE_MEM_META)

        #check sanity
        if check_exist==True:
            Const._check_path('base memory', mempath)
            Const._check_path('base disk-hash', diskmeta)
            Const._check_path('base memory-hash', memmeta)

        return diskmeta, mempath, memmeta

    @staticmethod
    def get_basehash_path(base_disk_path, check_exist=False):
        Const._check_path('base disk', base_disk_path)
        image_name = os.path.splitext(base_disk_path)[0]
        dir_path = os.path.dirname(base_disk_path)
        diskhash = os.path.join(dir_path, image_name+Const.BASE_DISK_HASH)
        if check_exist == True:
            Const._check_path(diskhash)
        return diskhash

