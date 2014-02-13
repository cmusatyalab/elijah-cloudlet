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

import os


class ConfigurationError(Exception):
    pass


def which(program):
    def is_exe(fpath):
        return os.path.isfile(fpath) and os.access(fpath, os.X_OK) 
    fpath, fname = os.path.split(program)
    if fpath:
        if is_exe(program):
            return program
    else:
        for path in os.environ["PATH"].split(os.pathsep):
            path = path.strip('"')
            exe_file = os.path.join(path, program)
            if is_exe(exe_file):
                return exe_file
    return exe_file


class Options(object):
    TRIM_SUPPORT                        = True
    FREE_SUPPORT                        = False
    XRAY_SUPPORT                        = False
    DISK_ONLY                           = False
    ZIP_CONTAINER                       = False
    DATA_SOURCE_URI                     = None

    # see the effect of dedup and reducing semantic by generating two indenpendent overlay
    SEPERATE_DEDUP_REDUCING_SEMANTICS   = False     
    # for test purposes, we can optionally save modified memory snapshot
    MEMORY_SAVE_PATH                    = None

    def __str__(self):
        import pprint
        return pprint.pformat(self.__dict__)


class Const(object):
    VERSION = str("0.8.5")
    HOME_DIR = os.path.abspath(os.path.expanduser("~"))
    CONFIGURATION_DIR = os.path.join('/', 'var', 'lib', 'cloudlet', 'conf')

    BASE_DISK               = ".base-img"
    BASE_MEM                = ".base-mem"
    BASE_DISK_META          = ".base-img-meta"
    BASE_MEM_META           = ".base-mem-meta"
    BASE_HASH_VALUE         = ".base-hash"
    OVERLAY_URIs            = ".overlay-URIs"
    OVERLAY_META            = "overlay-meta"
    OVERLAY_FILE_PREFIX     = "overlay-blob"
    OVERLAY_ZIP             = "overlay.zip"
    OVERLAY_LOG             = ".overlay-log"
    LOG_PATH                = "/var/tmp/cloudlet/log-synthesis"
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
    QEMU_BIN_PATH           = which("cloudlet_qemu-system-x86_64")
    FREE_MEMORY_BIN_PATH    = which("cloudlet_free_page_scan")
    VMNETFS_PATH            = which("cloudlet_vmnetfs")
    XRAY_BIN_PATH           = which("cloudlet_disk_analyzer")
    UPnP_SERVER             = which("upnp_server.jar")

    # personal information
    CLOUDLET_DB             = os.path.abspath(os.path.join(HOME_DIR, ".cloudlet/config/cloudlet.db"))
    BASE_VM_DIR             = os.path.abspath(os.path.join(HOME_DIR, ".cloudlet", "baseVM"))

    # global configuration files
    CLOUDLET_DB_SCHEMA      = os.path.join(CONFIGURATION_DIR, "schema.sql")
    BASEVM_PACKAGE_SCHEMA   = os.path.join(CONFIGURATION_DIR, "package.xsd")
    TEMPLATE_XML            = os.path.join(CONFIGURATION_DIR, "VM_TEMPLATE.xml")
    TEMPLATE_OVF            = os.path.join(CONFIGURATION_DIR, "ovftransport.iso")
    CHUNK_SIZE=4096

    @staticmethod
    def _check_path(name, path):
        if not os.path.exists(path):
            message = "Cannot find name at %s" % (path)
            raise ConfigurationError(message)
        if not os.access(path, os.R_OK):
            message = "File exists but cannot read the file at %s" % (path)
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

    @staticmethod
    def get_base_hashpath(base_disk_path):
        image_name = os.path.splitext(os.path.basename(base_disk_path))[0]
        dir_path = os.path.dirname(base_disk_path)
        return os.path.join(dir_path, image_name+Const.BASE_HASH_VALUE)


class Synthesis_Const(object):
    # PIPLINING CONSTANT
    TRANSFER_SIZE           = 1024*16
    END_OF_FILE             = "!!Overlay Transfer End Marker"
    ERROR_OCCURED           = "!!Overlay Transfer Error Marker"

    # Synthesis Server
    LOCAL_IPADDRESS = 'localhost'
    SERVER_PORT_NUMBER = 8021


class Caching_Const(object):
    MODULE_DIR          = os.path.dirname(os.path.abspath(__file__))
    CACHE_FUSE_BINPATH  = os.path.abspath(os.path.join(MODULE_DIR, "./caching/fuse/cachefs"))
    HOST_SAMBA_DIR      = "/var/samba/"
    CACHE_ROOT          = '/tmp/cloudlet_cache/'
    REDIS_ADDR          = ('localhost', 6379)
    REDIS_REQ_CHANNEL   = "fuse_request"
    REDIS_RES_CHANNEL   = "fuse_response"


