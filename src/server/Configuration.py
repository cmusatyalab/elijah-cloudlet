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

class ConfigurationError(Exception):
    pass


class Options(object):
    TRIM_SUPPORT                        = True
    FREE_SUPPORT                        = True
    XRAY_SUPPORT                        = False
    DISK_ONLY                           = False
    DATA_SOURCE_URI                     = None

    # see the effect of dedup and reducing semantic by generating two indenpendent overlay
    SEPERATE_DEDUP_REDUCING_SEMANTICS   = False     
    # for test purposes, we can optionally save modified memory snapshot
    MEMORY_SAVE_PATH                    = None

    def __str__(self):
        import pprint
        return pprint.pformat(self.__dict__)


class Const(object):
    BASE_DISK               = ".base-img"
    BASE_MEM                = ".base-mem"
    BASE_DISK_META          = ".base-img-meta"
    BASE_MEM_META           = ".base-mem-meta"
    OVERLAY_META            = ".overlay-meta"
    OVERLAY_URIs            = ".overlay-URIs"
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

    MODULE_DIR = os.path.dirname(os.path.abspath(__file__))
    VMNETFS_PATH            = os.path.abspath(os.path.join(MODULE_DIR, "../../lib/bin/x86_64/vmnetfs"))
    VMNETFS_PATH            = os.path.abspath(os.path.join(MODULE_DIR, "../../lib/bin/x86_64/vmnetfs"))
    QEMU_BIN_PATH           = os.path.abspath(os.path.join(MODULE_DIR, "../../lib/bin/x86_64/qemu-system-x86_64"))
    FREE_MEMORY_BIN_PATH    = os.path.abspath(os.path.join(MODULE_DIR, "../../lib/bin/x86_64/free_page_scan"))
    XRAY_BIN_PATH           = os.path.abspath(os.path.join(MODULE_DIR, "../../lib/x86_64/disk_analyzer"))

    CLOUDLET_DB             = os.path.abspath(os.path.join(MODULE_DIR, "./config/cloudlet.db"))
    CLOUDLET_DB_SCHEMA      = os.path.abspath(os.path.join(MODULE_DIR, "./config/schema.sql"))
    TEMPLATE_XML            = os.path.abspath(os.path.join(MODULE_DIR, "./config/VM_TEMPLATE.xml"))
    TEMPLATE_OVF            = os.path.abspath(os.path.join(MODULE_DIR, "./config/ovftransport.iso"))
    UPnP_SERVER             = os.path.abspath(os.path.join(MODULE_DIR, "../../lib/bin/upnp_server.jar"))
    REST_SERVER_BIN         = os.path.abspath(os.path.join(MODULE_DIR, "./RESTServer"))
    CHUNK_SIZE=4096

    @staticmethod
    def _check_path(name, path):
        if not os.path.exists(path):
            message = "Cannot find name at %s" % (path)
            raise ConfigurationError(message)

    @staticmethod
    def get_basepath(base_disk_path, check_exist=False):
        Const._check_path('base disk', base_disk_path)

        image_name = os.path.splitext(os.path.basename(base_disk_path))[0]
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


class Synthesis_Const(object):
    # PIPLINING
    TRANSFER_SIZE           = 1024*16
    END_OF_FILE             = "!!Overlay Transfer End Marker"
    SHOW_VNC                = False
    IS_EARLY_START          = False
    IS_PRINT_STATISTICS     = False

    # Discovery
    DIRECTORY_UPDATE_PERIOD = 60*10 # 10 min

    # Synthesis Server
    LOCAL_IPADDRESS = 'localhost'
    SERVER_PORT_NUMBER = 8021


class Discovery_Const(object):
    HOST_SAMBA_DIR = "/var/samba/"
