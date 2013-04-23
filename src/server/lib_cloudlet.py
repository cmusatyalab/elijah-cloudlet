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
import os
import subprocess
import Memory
import Disk
import vmnetfs
import vmnetx
import stat
import delta
import xray
import msgpack
import hashlib
import libvirt
import shutil
from db import api as db_api
from db import table_def as db_table
from Configuration import Const, Options
from delta import DeltaList
from delta import DeltaItem
from xml.etree import ElementTree
from xml.etree.ElementTree import Element
from uuid import uuid4
from tempfile import NamedTemporaryFile
from time import time
from time import sleep
import threading
from optparse import OptionParser
from multiprocessing import Pipe

from tool import comp_lzma
from tool import diff_files
from tool import decomp_overlay


class CloudletGenerationError(Exception):
    pass


class CloudletLog(object):
    def __init__(self, filename=None):
        if filename != None:
            self.logdir = os.path.dirname(filename)
            if os.path.exists(self.logdir) == False:
                os.makedirs(self.logdir)
            self.logfile = open(filename, "w+")
        else:
            self.logfile = open(Const.OVERLAY_LOG, "w+")
        self.mute = open("/dev/null", "w+b")

    def write(self, log):
        self.logfile.write(log)
        sys.stdout.write(log)

    def flush(self):
        self.logfile.flush()
        sys.stdout.flush()


class VM_Overlay(threading.Thread):
    def __init__(self, base_disk, options, qemu_args=None):
        # create user customized overlay.
        # First resume VM, then let user edit its VM
        # Finally, return disk/memory binary as an overlay
        # base_disk: path to base disk
        self.base_disk = base_disk
        self.options = options
        self.qemu_args = qemu_args
        threading.Thread.__init__(self, target=self.create_overlay)

    def create_overlay(self):
        start_time = time()
        log_path = os.path.join(os.path.dirname(self.base_disk), \
                                os.path.basename(self.base_disk) + Const.OVERLAY_LOG)
        Log = CloudletLog(log_path)

        if (self.options == None) or (isinstance(self.options, Options) == False):
            raise CloudletGenerationError("Given option class is invalid: %s" % str(self.options))

        (base_diskmeta, base_mem, base_memmeta) = \
                Const.get_basepath(self.base_disk, check_exist=True)

        # find base vm from DB
        base_hashvalue = None
        dbconn = db_api.DBConnector()
        basevm_list = dbconn.list_item(db_table.BaseVM)
        for basevm_row in basevm_list:
            if basevm_row.disk_path == self.base_disk:
                base_hashvalue = basevm_row.hash_value
        if base_hashvalue == None:
            raise CloudletGenerationError("Cannot find hashvalue for %s" % self.base_disk)

        # filename for overlay VM
        qemu_logfile = NamedTemporaryFile(prefix="cloudlet-qemu-log-", delete=False)

        # make FUSE disk & memory
        fuse = run_fuse(Const.VMNETFS_PATH, Const.CHUNK_SIZE,
                self.base_disk, os.path.getsize(self.base_disk),
                base_mem, os.path.getsize(base_mem), print_out=Log)
        modified_disk = os.path.join(fuse.mountpoint, 'disk', 'image')
        base_mem_fuse = os.path.join(fuse.mountpoint, 'memory', 'image')
        modified_mem = NamedTemporaryFile(prefix="cloudlet-mem-", delete=False)
        # monitor modified chunks
        stream_modified = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_modified')
        stream_access = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_accessed')
        memory_access = os.path.join(fuse.mountpoint, 'memory', 'streams', 'chunks_accessed')
        fuse_stream_monitor = vmnetfs.StreamMonitor()
        fuse_stream_monitor.add_path(stream_modified, vmnetfs.StreamMonitor.DISK_MODIFY)
        fuse_stream_monitor.add_path(stream_access, vmnetfs.StreamMonitor.DISK_ACCESS)
        fuse_stream_monitor.add_path(memory_access, vmnetfs.StreamMonitor.MEMORY_ACCESS)
        fuse_stream_monitor.start()
        qemu_monitor = vmnetfs.FileMonitor(qemu_logfile.name, vmnetfs.FileMonitor.QEMU_LOG)
        qemu_monitor.start()

        # 1. resume & get modified disk
        Log.write("[INFO] * Overlay creation configuration\n")
        Log.write("[INFO]  - %s\n" % str(self.options))
        conn = get_libvirt_connection()
        machine = run_snapshot(conn, modified_disk, base_mem_fuse, qemu_logfile=qemu_logfile.name, qemu_args=self.qemu_args)
        connect_vnc(machine)
        Log.write("[TIME] user interaction time for creating overlay: %f\n" % (time()-start_time))

        # 2. get montoring info
        monitoring_info = _get_monitoring_info(machine, self.options,
                base_memmeta, base_diskmeta,
                fuse_stream_monitor,
                self.base_disk, base_mem,
                modified_disk, modified_mem.name,
                qemu_logfile, Log)

        # 3. get overlay VM
        overlay_deltalist = get_overlay_deltalist(monitoring_info, self.options,
                self.base_disk, base_mem, base_memmeta, 
                modified_disk, modified_mem.name,
                Log)

        # 4. create_overlayfile
        image_name = os.path.basename(self.base_disk).split(".")[0]
        dir_path = os.path.dirname(base_mem)
        overlay_prefix = os.path.join(dir_path, image_name+Const.OVERLAY_FILE_PREFIX)
        overlay_metapath = os.path.join(dir_path, image_name+Const.OVERLAY_META)

        self.overlay_metafile, self.overlay_files = \
                generate_overlayfile(overlay_deltalist, self.options, 
                base_hashvalue, os.path.getsize(modified_disk), os.path.getsize(modified_mem.name),
                overlay_metapath, overlay_prefix, Log)

        # 5. terminting
        fuse.terminate()
        fuse_stream_monitor.terminate()
        qemu_monitor.terminate()
        fuse_stream_monitor.join()
        qemu_monitor.join()
        fuse.join()
        if self.options.MEMORY_SAVE_PATH:
            Log.write("[INFO] moving memory sansphost to %s\n" % self.options.MEMORY_SAVE_PATH)
            shutil.move(modified_mem.name, self.options.MEMORY_SAVE_PATH)
        else:
            os.unlink(modified_mem.name)

        if os.path.exists(qemu_logfile.name):
            os.unlink(qemu_logfile.name)


class _MonitoringInfo(object):
    BASEDISK_HASHLIST       = "basedisk_hashlist"
    BASEMEM_HASHLIST        = "basemem_hashlist"
    DISK_MODIFIED_BLOCKS    = "disk_modified_block" # from fuse monitoring
    DISK_USED_BLOCKS        = "disk_used_block" # from xray support
    DISK_FREE_BLOCKS        = "disk_free_block"
    MEMORY_FREE_BLOCKS      = "memory_free_block"

    def __init__(self, properties):
        for k, v in properties.iteritems():
            setattr(self, k, v)

    def __str__(self):
        ret = ""
        for k, v in self.__dict__.iteritems():
            ret += "%s\t:\t%s\n" % (str(k), str(v))
        return ret

    def __getitem__(self, item):
        return self.__dict__[item]


class ResumedVM(threading.Thread):
    def __init__(self, launch_disk, launch_mem, fuse, disk_only=False, qemu_args=None, **kwargs):
        # kwargs
        # param Log: log out
        self.LOG = kwargs.get("log", None)
        if self.LOG == None:
            self.LOG = open("/dev/null", "w+b")

        # monitor modified chunks
        self.machine = None
        self.disk_only = disk_only
        self.qemu_args = qemu_args
        self.fuse = fuse
        self.launch_disk = launch_disk
        self.launch_mem = launch_mem
        self.qemu_logfile = NamedTemporaryFile(prefix="cloudlet-qemu-log-", delete=False)
        self.resumed_disk = os.path.join(fuse.mountpoint, 'disk', 'image')
        self.resumed_mem = os.path.join(fuse.mountpoint, 'memory', 'image')
        self.stream_modified = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_modified')
        self.stream_disk_access = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_accessed')
        self.stream_memory_access = os.path.join(fuse.mountpoint, 'memory', 'streams', 'chunks_accessed')
        self.monitor = vmnetfs.StreamMonitor()
        self.monitor.add_path(self.stream_modified, vmnetfs.StreamMonitor.DISK_MODIFY)
        self.monitor.add_path(self.stream_disk_access, vmnetfs.StreamMonitor.DISK_ACCESS)
        self.monitor.add_path(self.stream_memory_access, vmnetfs.StreamMonitor.MEMORY_ACCESS)
        self.monitor.start()
        self.qemu_monitor = vmnetfs.FileMonitor(self.qemu_logfile.name, vmnetfs.FileMonitor.QEMU_LOG)
        self.qemu_monitor.start()
        threading.Thread.__init__(self, target=self.resume)

    def resume(self):
        #resume VM
        conn = get_libvirt_connection()
        self.resume_time = {'time':-100}
        try:
            if self.disk_only:
                # edit default XML to have new disk path
                conn = get_libvirt_connection()
                xml = ElementTree.fromstring(open(Const.TEMPLATE_XML, "r").read())
                new_xml_string = _convert_xml(xml, conn, disk_path=self.resumed_disk, \
                        uuid=str(uuid4()), qemu_args=self.qemu_args)
                self.machine = run_vm(conn, new_xml_string, vnc_disable=True)
            else:
                self.machine = run_snapshot(conn, self.resumed_disk, self.resumed_mem,
                        qemu_logfile=self.qemu_logfile.name, resume_time=self.resume_time,
                        qemu_args=self.qemu_args)
        except Exception as e:
            sys.stdout.write(str(e)+"\n")

    def terminate(self):
        try:
            if self.machine:
                self.machine.destroy()
        except libvirt.libvirtError as e:
            pass

        # terminate
        self.fuse.terminate()
        self.monitor.terminate()
        self.qemu_monitor.terminate()
        self.monitor.join()
        self.qemu_monitor.join()

        # delete all temporary file
        if os.path.exists(self.launch_disk):
            os.unlink(self.launch_disk)
        if os.path.exists(self.launch_mem):
            os.unlink(self.launch_mem)
        if os.path.exists(self.qemu_logfile.name):
            os.unlink(self.qemu_logfile.name)


def _create_overlay_meta(base_hash, overlay_metafile, modified_disksize, modified_memsize,
        blob_info):
    fout = open(overlay_metafile, "wrb")

    meta_dict = dict()
    meta_dict[Const.META_BASE_VM_SHA256] = base_hash
    meta_dict[Const.META_RESUME_VM_DISK_SIZE] = modified_disksize,
    meta_dict[Const.META_RESUME_VM_MEMORY_SIZE] = modified_memsize,
    meta_dict[Const.META_OVERLAY_FILES] = blob_info

    serialized = msgpack.packb(meta_dict)
    fout.write(serialized)
    fout.close()


def _update_overlay_meta(original_meta, new_path, blob_info=None):
    fout = open(new_path, "wrb")

    if blob_info:
        original_meta[Const.META_OVERLAY_FILES] = blob_info
    serialized = msgpack.packb(original_meta)
    fout.write(serialized)
    fout.close()


def _test_dma_accuracy(dma_dict, disk_deltalist, mem_deltalist, Log=sys.stdout):
    dma_start_time = time()
    dma_read_counter = 0
    dma_write_counter = 0
    dma_read_overlay_dedup = 0
    dma_write_overlay_dedup = 0
    dma_read_base_dedup = 0
    dma_write_base_dedup = 0
    disk_delta_dict = dict([(delta.offset/Const.CHUNK_SIZE, delta) for delta in disk_deltalist])
    mem_delta_dict = dict([(delta.offset/Const.CHUNK_SIZE, delta) for delta in mem_deltalist])
    for dma_disk_chunk in dma_dict.keys():
        item = dma_dict.get(dma_disk_chunk)
        is_dma_read = item['read']
        dma_mem_chunk = item['mem_chunk']
        if is_dma_read:
            dma_read_counter += 1
        else:
            dma_write_counter += 1

        disk_delta = disk_delta_dict.get(dma_disk_chunk, None)
        if disk_delta:
            # first search at overlay disk
            if disk_delta.ref_id != DeltaItem.REF_OVERLAY_MEM:
#                print "dma disk chunk is same, but is it not deduped with overlay mem(%d)" \
#                        % (disk_delta.ref_id)
                continue
            delta_mem_chunk = disk_delta.data/Const.CHUNK_SIZE
            if delta_mem_chunk == dma_mem_chunk:
                if is_dma_read:
                    dma_read_overlay_dedup += 1
                else:
                    dma_write_overlay_dedup += 1
        else:
            # search at overlay mem
            mem_delta = mem_delta_dict.get(dma_mem_chunk, None)
            if mem_delta:
                if mem_delta.ref_id != DeltaItem.REF_BASE_DISK:
#                    print "dma memory chunk is same, but is it not deduped with base disk(%d)" \
#                            % (mem_delta.ref_id)
                    continue
                delta_disk_chunk = mem_delta.data/Const.CHUNK_SIZE
                if delta_disk_chunk == dma_disk_chunk:
                    if is_dma_read:
                        dma_read_base_dedup += 1
                    else:
                        dma_write_base_dedup += 1

    dma_end_time = time()
    Log.write("[DEBUG][DMA] Total DMA: %ld\n " % (len(dma_dict)))
    Log.write("[DEBUG][DMA] Total DMA READ: %ld, WRITE: %ld\n " % (dma_read_counter, dma_write_counter))
    Log.write("[DEBUG][DMA] WASTED TIME: %f\n " % (dma_end_time-dma_start_time))
    Log.write("[DEBUG][DMA] 1) DMA READ Overlay Deduplication: %ld(%f %%)\n " % \
            (dma_read_overlay_dedup, 100.0*dma_read_overlay_dedup/dma_read_counter))
    Log.write("[DEBUG][DMA]    DMA READ Base Deduplication: %ld(%f %%)\n " % \
            (dma_read_base_dedup, 100.0*dma_read_base_dedup/dma_read_counter))
    Log.write("[DEBUG][DMA] 2) DMA WRITE Overlay Deduplication: %ld(%f %%)\n " % \
            (dma_write_overlay_dedup, 100.0*dma_write_overlay_dedup/dma_write_counter))
    Log.write("[DEBUG][DMA]    DMA WRITE Base Deduplication: %ld(%f %%)\n " % \
            (dma_write_base_dedup, 100.0*dma_write_base_dedup/dma_write_counter))


def _convert_xml(xml, conn, vm_name=None, disk_path=None, uuid=None, \
        logfile=None, qemu_args=None):
    if vm_name:
        name_element = xml.find('name')
        if not name_element:
            raise CloudletGenerationError("Malfomed XML input: %s", Const.TEMPLATE_XML)
        name_element.text = vm_name

    if uuid:
        uuid_element = xml.find('uuid')
        uuid_element.text = str(uuid)

    # Use custom QEMU
    qemu_emulator = xml.find('devices/emulator')
    if qemu_emulator == None:
        print ElementTree.tostring(xml)
        raise CloudletGenerationError("Cannot find VMM path at XML")
    qemu_emulator.text = Const.QEMU_BIN_PATH

    # disk path is required
    if not disk_path:
        raise CloudletGenerationError("Need disk_path to run new VM")

    # find all disk element(hdd, cdrom)
    disk_elements = xml.findall('devices/disk')
    hdd_source = None
    cdrom_source = None
    for disk_element in disk_elements:
        disk_type = disk_element.attrib['device']
        if disk_type == 'disk':
            hdd_source = disk_element.find('source')
        if disk_type == 'cdrom':
            cdrom_source = disk_element.find('source')

    # hdd path setting
    if hdd_source == None:
        raise CloudletGenerationError("Malfomed XML input: %s", Const.TEMPLATE_XML)
    hdd_source.set("file", os.path.abspath(disk_path))

    # ovf path setting
    if cdrom_source != None:
        cdrom_source.set("file", os.path.abspath(Const.TEMPLATE_OVF))

    # append QEMU-argument
    if logfile:
        qemu_xmlns="http://libvirt.org/schemas/domain/qemu/1.0"
        qemu_element = xml.find("{%s}commandline" % qemu_xmlns)
        if qemu_element == None:
            qemu_element = Element("{%s}commandline" % qemu_xmlns)
            xml.append(qemu_element)
            #msg = "qemu_xmlns is NULL, Malfomed XML input: %s\n%s" % \
            #        (Const.TEMPLATE_XML, ElementTree.tostring(xml))
            #raise CloudletGenerationError(msg)
        qemu_element.append(Element("{%s}arg" % qemu_xmlns, {'value':'-cloudlet'}))
        qemu_element.append(Element("{%s}arg" % qemu_xmlns, {'value':"logfile=%s" % logfile}))

    # append user qemu argument
    if qemu_args:
        qemu_xmlns="http://libvirt.org/schemas/domain/qemu/1.0"
        qemu_element = xml.find("{%s}commandline" % qemu_xmlns)
        if qemu_element == None:
            qemu_element = Element("{%s}commandline" % qemu_xmlns)
            xml.append(qemu_element)
        for each_argument in qemu_args:
            qemu_element.append(Element("{%s}arg" % qemu_xmlns, {'value':each_argument}))


    return ElementTree.tostring(xml)


def _get_monitoring_info(machine, options,
        base_memmeta, base_diskmeta,
        fuse_stream_monitor,
        base_disk, base_mem,
        modified_disk, modified_mem,
        qemu_logfile, print_out):
    ''' return montioring information including
        1) base vm hash list
        2) used/freed disk block list
        3) freed memory page
    '''

    # 1-2. Stop monitoring for memory access (snapshot will create a lot of access)
    fuse_stream_monitor.del_path(vmnetfs.StreamMonitor.MEMORY_ACCESS)
    # TODO: support stream of modified memory rather than tmp file
    if not options.DISK_ONLY:
        save_mem_snapshot(machine, modified_mem)

    # 1-3. get hashlist of base memory and disk
    basemem_hashlist = Memory.base_hashlist(base_memmeta)
    basedisk_hashlist = Disk.base_hashlist(base_diskmeta)

    # 1-4. get dma & discard information
    if options.TRIM_SUPPORT:
        dma_dict, trim_dict = Disk.parse_qemu_log(qemu_logfile.name, Const.CHUNK_SIZE, print_out=print_out)
        if len(trim_dict) == 0:
            print_out.write("[WARNING] No TRIM Discard, Check /etc/fstab configuration\n")
    else:
        trim_dict = dict()
    free_memory_dict = dict()

    # 1-5. get used sector information from x-ray
    used_blocks_dict = None
    if options.XRAY_SUPPORT:
        used_blocks_dict = xray.get_used_blocks(modified_disk)

    m_chunk_dict = fuse_stream_monitor.modified_chunk_dict

    info_dict = dict()
    info_dict[_MonitoringInfo.BASEDISK_HASHLIST] = basedisk_hashlist
    info_dict[_MonitoringInfo.BASEMEM_HASHLIST] = basemem_hashlist
    info_dict[_MonitoringInfo.DISK_USED_BLOCKS] = used_blocks_dict
    info_dict[_MonitoringInfo.DISK_MODIFIED_BLOCKS] = m_chunk_dict
    info_dict[_MonitoringInfo.DISK_FREE_BLOCKS] = trim_dict
    info_dict[_MonitoringInfo.MEMORY_FREE_BLOCKS] = free_memory_dict
    monitoring_info = _MonitoringInfo(info_dict)
    return monitoring_info


def copy_disk(in_path, out_path):
    print "[INFO] Copying disk image to %s" % out_path
    cmd = "cp %s %s" % (in_path, out_path)
    cp_proc = subprocess.Popen(cmd, shell=True)
    cp_proc.wait()
    if cp_proc.returncode != 0:
        raise IOError("Copy failed: from %s to %s " % (in_path, out_path))


def get_libvirt_connection():
    conn = libvirt.open("qemu:///session")
    return conn


def get_overlay_deltalist(monitoring_info, options,
        base_image, base_mem, base_memmeta, 
        modified_disk, modified_mem,
        print_out, old_deltalist=None):
    '''return overlay deltalist
    Get difference between base vm (base_image, base_mem) and 
    launch vm (modified_disk, modified_mem) using monitoring information

    Args:
        prev_mem_deltalist : Option only for creating_residue.
            Different from disk, we create whole memory snapshot even for residue.
            So, to get the precise difference between previous memory overlay,
            we need previous memory deltalist
    '''

    INFO = _MonitoringInfo
    basedisk_hashlist = getattr(monitoring_info, INFO.BASEDISK_HASHLIST, None)
    basemem_hashlist = getattr(monitoring_info, INFO.BASEMEM_HASHLIST, None)
    free_memory_dict = getattr(monitoring_info, INFO.MEMORY_FREE_BLOCKS, None)
    m_chunk_dict = getattr(monitoring_info, INFO.DISK_MODIFIED_BLOCKS, None)
    trim_dict = getattr(monitoring_info, INFO.DISK_FREE_BLOCKS, None)
    used_blocks_dict = getattr(monitoring_info, INFO.DISK_USED_BLOCKS, None)
    dma_dict = dict()

    # 2-1. get memory overlay
    if options.DISK_ONLY:
        mem_deltalist = list()
    else:
        mem_deltalist= Memory.create_memory_deltalist(modified_mem,
                basemem_meta=base_memmeta, basemem_path=base_mem,
                apply_free_memory=options.FREE_SUPPORT,
                free_memory_info=free_memory_dict,
                print_out=print_out)
        if old_deltalist and len(old_deltalist) > 0:
            diff_deltalist = delta.residue_diff_deltalists(old_deltalist,
                    mem_deltalist, base_mem, print_out)
            mem_deltalist = diff_deltalist

    # 2-2. get disk overlay
    disk_statistics = dict()
    disk_deltalist = Disk.create_disk_deltalist(modified_disk,
        m_chunk_dict, Const.CHUNK_SIZE,
        basedisk_hashlist=basedisk_hashlist, basedisk_path=base_image,
        trim_dict=trim_dict,
        apply_discard = True,
        dma_dict=dma_dict,
        used_blocks_dict=used_blocks_dict,
        ret_statistics=disk_statistics,
        print_out=print_out)


    # 2-3. Merge disk & memory delta_list to generate overlay file
    # deduplication
    merged_deltalist = delta.create_overlay(
            mem_deltalist, Memory.Memory.RAM_PAGE_SIZE,
            disk_deltalist, Const.CHUNK_SIZE,
            basedisk_hashlist=basedisk_hashlist,
            basemem_hashlist=basemem_hashlist,
            print_out=print_out)

    free_memory_dict = getattr(monitoring_info, _MonitoringInfo.MEMORY_FREE_BLOCKS, None)
    free_pfn_counter = long(free_memory_dict.get("freed_counter", 0))
    disk_discarded_count = disk_statistics.get('trimed', 0)
    DeltaList.statistics(merged_deltalist, print_out=print_out,
            mem_discarded=free_pfn_counter,
            disk_discarded=disk_discarded_count)

    return merged_deltalist


def generate_overlayfile(overlay_deltalist, options, 
        base_hashvalue, launchdisk_size, launchmem_size,
        overlay_metapath, overlayfile_prefix, print_out):
    ''' generate overlay metafile and file
    Return:
        [overlay_metapath, [overlayfilepath1, overlayfilepath2]]
    '''

    # Compression
    print_out.write("[DEBUG][LZMA] Compressing overlay blobs\n")
    blob_list = delta.divide_blobs(overlay_deltalist, overlayfile_prefix,
            Const.OVERLAY_BLOB_SIZE_KB, Const.CHUNK_SIZE,
            Memory.Memory.RAM_PAGE_SIZE, print_out=print_out)

    # create metadata
    if not options.DISK_ONLY:
        _create_overlay_meta(base_hashvalue, overlay_metapath,
                launchdisk_size, launchmem_size, blob_list)
    else:
        _create_overlay_meta(base_hashvalue, overlay_metapath,
                launchdisk_size, launchmem_size, blob_list)

    overlay_files = [item[Const.META_OVERLAY_FILE_NAME] for item in blob_list]
    dirpath = os.path.dirname(overlayfile_prefix)
    overlay_files = [os.path.join(dirpath, item) for item in overlay_files]

    return overlay_metapath, overlay_files


def run_delta_compression(output_list, **kwargs):
    # kwargs
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess
    #           will be returned without finishing their work
    # custom_delta
    log = kwargs.get('log', None)
    nova_util = kwargs.get('nova_util', None)
    custom_delta = kwargs.get('custom_delta', False)

    # xdelta and compression
    ret_files = []
    for (base, modified, overlay) in output_list:
        start_time = time()

        # xdelta
        if custom_delta:
            diff_files(base, modified, overlay, nova_util=nova_util)
        else:
            diff_files(base, modified, overlay, nova_util=nova_util)
        print '[TIME] time for creating overlay : ', str(time()-start_time)
        print '[INFO] (%d)-(%d)=(%d): ' % (os.path.getsize(base), os.path.getsize(modified), os.path.getsize(overlay))

        # compression
        comp = overlay + '.lzma'
        comp, time1 = comp_lzma(overlay, comp, nova_util=nova_util)
        ret_files.append(comp)

        # remove temporary files
        os.remove(modified)
        os.remove(overlay)

    return ret_files


def recover_launchVM(base_image, meta_info, overlay_file, **kwargs):
    # kwargs
    # skip_validation   :   skip sha1 validation
    # LOG = log object for nova
    # nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess
    #           will be returned without finishing their work
    log = kwargs.get('log', open("/dev/null", "w+b"))
    nova_util = kwargs.get('nova_util', None)

    (base_diskmeta, base_mem, base_memmeta) = \
            Const.get_basepath(base_image, check_exist=True)
    launch_mem = NamedTemporaryFile(prefix="cloudlet-launch-mem-", delete=False)
    launch_disk = NamedTemporaryFile(prefix="cloudlet-launch-disk-", delete=False)

    # Get modified list from overlay_meta
    vm_disk_size = meta_info[Const.META_RESUME_VM_DISK_SIZE]
    vm_memory_size = meta_info[Const.META_RESUME_VM_MEMORY_SIZE]
    memory_chunk_list = list()
    disk_chunk_list = list()
    for each_file in meta_info[Const.META_OVERLAY_FILES]:
        memory_chunks = each_file[Const.META_OVERLAY_FILE_MEMORY_CHUNKS]
        disk_chunks = each_file[Const.META_OVERLAY_FILE_DISK_CHUNKS]
        memory_chunk_list.extend(["%ld:0" % item for item in memory_chunks])
        disk_chunk_list.extend(["%ld:0" % item for item in disk_chunks])
    disk_overlay_map = ','.join(disk_chunk_list)
    memory_overlay_map = ','.join(memory_chunk_list)

    # make FUSE disk & memory
    kwargs['meta_info'] = meta_info
    kwargs['print_out'] = log
    fuse = run_fuse(Const.VMNETFS_PATH, Const.CHUNK_SIZE,
            base_image, vm_disk_size, base_mem, vm_memory_size,
            resumed_disk=launch_disk.name,  disk_overlay_map=disk_overlay_map,
            resumed_memory=launch_mem.name, memory_overlay_map=memory_overlay_map,
            **kwargs)
    log.write("[INFO] Start FUSE\n")

    # Recover Modified Memory
    pipe_parent, pipe_child = Pipe()
    delta_proc = delta.Recovered_delta(base_image, base_mem, overlay_file, \
            launch_mem.name, vm_memory_size,
            launch_disk.name, vm_disk_size, Const.CHUNK_SIZE,
            out_pipe=pipe_child)
    fuse_thread = vmnetfs.FuseFeedingThread(fuse,
            pipe_parent, delta.Recovered_delta.END_OF_PIPE, print_out=log)
    return [launch_disk.name, launch_mem.name, fuse, delta_proc, fuse_thread]


def run_fuse(bin_path, chunk_size, original_disk, fuse_disk_size,
        original_memory, fuse_memory_size,
        resumed_disk=None, disk_overlay_map=None,
        resumed_memory=None, memory_overlay_map=None,
        **kwargs):
    if fuse_disk_size <= 0:
        raise CloudletGenerationError("FUSE disk size should be bigger than 0")
    if original_memory != None and fuse_memory_size <= 0:
        raise CloudletGenerationError("FUSE memory size should be bigger than 0")

    # run fuse file system
    resumed_disk = os.path.abspath(resumed_disk) if resumed_disk else ""
    resumed_memory = os.path.abspath(resumed_memory) if resumed_memory else ""
    disk_overlay_map = str(disk_overlay_map) if disk_overlay_map else ""
    memory_overlay_map = str(memory_overlay_map) if memory_overlay_map else ""

    # launch fuse
    execute_args = ['', '', \
            # disk parameter
            '%s' % vmnetfs.VMNetFS.FUSE_TYPE_DISK,
            "%s" % os.path.abspath(original_disk),  # base path
            "%s" % resumed_disk,                    # overlay path
            "%s" % disk_overlay_map,                # overlay map
            '%d' % fuse_disk_size,                       # size of base
            '0',                                    # segment size
            "%d" % chunk_size]
    if original_memory:
        for parameter in [
                # memory parameter
                '%s' % vmnetfs.VMNetFS.FUSE_TYPE_MEMORY,
                "%s" % os.path.abspath(original_memory),
                "%s" % resumed_memory,
                "%s" % memory_overlay_map,
                '%d' % fuse_memory_size,
                '0',\
                "%d" % chunk_size
                ]:
            execute_args.append(parameter)

    fuse_process = vmnetfs.VMNetFS(bin_path, execute_args, **kwargs)
    fuse_process.launch()
    fuse_process.start()
    return fuse_process


def run_vm(conn, domain_xml, **kwargs):
    # kwargs
    # vnc_disable       :   do not show vnc console
    # wait_vnc          :   wait until vnc finishes if vnc_enabled
    machine = conn.createXML(domain_xml, 0)

    # Run VNC and wait until user finishes working
    if kwargs.get('vnc_disable'):
        return machine

    # Get VNC port
    vnc_port = 5900
    try:
        running_xml_string = machine.XMLDesc(libvirt.VIR_DOMAIN_XML_SECURE)
        running_xml = ElementTree.fromstring(running_xml_string)
        vnc_port = running_xml.find("devices/graphics").get("port")
        vnc_port = int(vnc_port)-5900
    except AttributeError as e:
        sys.stderr.write("Warning, Possible VNC port error:%s\n" % str(e))

    _PIPE = subprocess.PIPE
    vnc_process = subprocess.Popen("gvncviewer localhost:%d" % vnc_port,
            shell=True, stdin=_PIPE, stdout=_PIPE, stderr=_PIPE)
    if kwargs.get('wait_vnc'):
        try:
            vnc_process.wait()
        except KeyboardInterrupt as e:
            print "[INFO] keyboard interrupt while waiting VNC"
            if machine:
                machine.destroy()
    return machine


def save_mem_snapshot(machine, fout_path, **kwargs):
    #kwargs
    #LOG = log object for nova
    #nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess
    #           will be returned without finishing their work
    log = kwargs.get('log', )
    nova_util = kwargs.get('nova_util', None)

    #Set migration speed
    ret = machine.migrateSetMaxSpeed(1000000, 0)   # 1000 Gbps, unlimited
    if ret != 0:
        raise CloudletGenerationError("Cannot set migration speed : %s", machine.name())

    #Pause VM
    ret = machine.suspend()
    if ret != 0:
        raise CloudletGenerationError("Cannot pause VM : %s", machine.name())

    #Save memory state
    print "[INFO] save VM memory state at %s" % fout_path
    print "[INFO] (This could take up to minute depend on VM's memory size)"
    print "[INFO] (Check file at %s)" % fout_path
    try:
        ret = machine.save(fout_path)
    except libvirt.libvirtError, e:
        raise CloudletGenerationError("libvirt memory save : " + str(e))
    if ret != 0:
        raise CloudletGenerationError("libvirt: Cannot save memory state")


def run_snapshot(conn, disk_image, mem_snapshot, **kwargs):
    # kwargs
    # qemu_logfile      :   log file for QEMU-KVM
    # resume_time       :   write back the resumed_time
    resume_time = kwargs.get('resume_time', None)
    logfile = kwargs.get('qemu_logfile', None)
    qemu_args = kwargs.get('qemu_args', None)
    if resume_time != None:
        start_resume_time = time()

    # read embedded XML at memory snapshot to change disk path
    hdr = vmnetx._QemuMemoryHeader(open(mem_snapshot))
    xml = ElementTree.fromstring(hdr.xml)
    new_xml_string = _convert_xml(xml, conn, disk_path=disk_image,
            uuid=uuid4(), logfile=logfile, qemu_args=qemu_args)

    overwrite_xml(mem_snapshot, new_xml_string)

    #temp_mem = NamedTemporaryFile(prefix="cloudlet-mem-")
    #copy_with_xml(mem_snapshot, temp_mem.name, new_xml_string)

    # resume
    restore_with_config(conn, mem_snapshot, new_xml_string)
    if resume_time != None:
        resume_time['start_time'] = start_resume_time
        resume_time['end_time'] = time()
        print "[RESUME] : QEMU resume time (%f)~(%f)=(%f)" % \
                (resume_time['start_time'], resume_time['end_time'], \
                resume_time['end_time']-resume_time['start_time'])


    # get machine
    domxml = ElementTree.fromstring(new_xml_string)
    uuid_element = domxml.find('uuid')
    uuid = str(uuid_element.text)
    machine = conn.lookupByUUIDString(uuid)

    return machine


def connect_vnc(machine, no_wait=False):
    # Get VNC port
    vnc_port = 5900
    try:
        running_xml_string = machine.XMLDesc(libvirt.VIR_DOMAIN_XML_SECURE)
        running_xml = ElementTree.fromstring(running_xml_string)
        vnc_port = running_xml.find("devices/graphics").get("port")
        vnc_port = int(vnc_port)-5900
    except AttributeError as e:
        sys.stderr.write("Warning, Possible VNC port error:%s\n" % str(e))

    # Run VNC
    vnc_process = subprocess.Popen("gvncviewer localhost:%d" % vnc_port,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            shell=True)
    if no_wait == True:
        return

    print "[INFO] waiting for finishing VNC interaction"
    try:
        vnc_process.wait()
    except KeyboardInterrupt as e:
        print "keyboard interrupt while waiting VNC"
        vnc_process.terminate()


def rettach_nic(conn, xml, **kwargs):
    #kwargs
    #LOG = log object for nova
    #nova_util = nova_util is executioin wrapper for nova framework
    #           You should use nova_util in OpenStack, or subprocess
    #           will be returned without finishing their work
    log = kwargs.get('log', None)

    # get machine
    domxml = ElementTree.fromstring(xml)
    uuid = domxml.find('uuid').text
    machine = conn.lookupByUUIDString(uuid)

    # get xml info of running xml
    running_xml = machine.XMLDesc(libvirt.VIR_DOMAIN_XML_SECURE)
    machinexml = ElementTree.fromstring(running_xml)
    nic = machinexml.find('devices/interface')
    nic_xml = ElementTree.tostring(nic)

    if log:
        log.debug(_("Rettaching device : %s" % str(nic_xml)))
        log.debug(_("memory xml"))
        log.debug(_("%s" % xml))
        log.debug(_("running xml"))
        log.debug(_("%s" % running_xml))
    else:
        print "[Debug] Rettaching device : %s" % str(nic_xml)

    #detach
    machine.detachDevice(nic_xml)
    sleep(3)
    if log:
        log.debug(_("dettached xml"))
        dettached_xml = machine.XMLDesc(libvirt.VIR_DOMAIN_XML_SECURE)
        log.debug(_("%s" % str(dettached_xml)))

    #attach
    machine.attachDevice(nic_xml)
    if log:
        log.debug(_("rettached xml"))
        rettached_xml = machine.XMLDesc(libvirt.VIR_DOMAIN_XML_SECURE)
        log.debug(_("%s" % str(rettached_xml)))


def restore_with_config(conn, mem_snapshot, xml):
    try:
        print "[INFO] restoring VM..."
        conn.restoreFlags(mem_snapshot, xml, libvirt.VIR_DOMAIN_SAVE_RUNNING)
        #conn.restoreFlags(mem_snapshot, xml, libvirt.VIR_DOMAIN_SAVE_PAUSED)
        print "[INFO] VM is restored..."
    except libvirt.libvirtError, e:
        message = "%s\nXML: %s\nError, Check you QEMU_ARGUMENT" % (xml, str(e))
        raise CloudletGenerationError(message)


def overwrite_xml(in_path, new_xml):
    fin = open(in_path, "rb")
    hdr = vmnetx._QemuMemoryHeader(fin)
    fin.close()

    # Write header
    fin = open(in_path, "r+b")
    hdr.overwrite(fin, new_xml)
    fin.close()


def copy_with_xml(in_path, out_path, xml):
    fin = open(in_path)
    fout = open(out_path, 'wrb')
    hdr = vmnetx._QemuMemoryneader(fin)

    # Write header
    hdr.xml = xml
    hdr.write(fout)
    fout.flush()

    # move to the content
    hdr.seek_body(fin)
    fout.write(fin.read())


def create_residue(base_disk, base_hashvalue, 
        resumed_vm, options, 
        original_deltalist, print_out):
    '''Get residue
    Return : residue_metafile_path, residue_filelist
    '''
    # 1 sanity check
    if (options == None) or (isinstance(options, Options) == False):
        raise CloudletGenerationError("Given option class is invalid: %s" % str(options))
    (base_diskmeta, base_mem, base_memmeta) = \
            Const.get_basepath(base_disk, check_exist=True)
    qemu_logfile = resumed_vm.qemu_logfile
    residue_mem = NamedTemporaryFile(prefix="cloudlet-residue-mem-", delete=False)

    # 2. suspend VM and get monitoring information
    monitoring_info = _get_monitoring_info(resumed_vm.machine, options,
            base_memmeta, base_diskmeta,
            resumed_vm.monitor,
            base_disk, base_mem,
            resumed_vm.resumed_disk, residue_mem.name,
            qemu_logfile, print_out)

    # 3. get overlay VM
    residue_deltalist = get_overlay_deltalist(monitoring_info, options,
            base_disk, base_mem, base_memmeta, 
            resumed_vm.resumed_disk, residue_mem.name,
            print_out, old_deltalist=original_deltalist)

    # 3-1. save residue overlay only for measurement
    # Free to delete
    image_name = os.path.basename(base_disk).split(".")[0]
    dir_path = os.path.abspath(".")
    residue_prefix = os.path.join(dir_path, "residue")
    residue_metapath = os.path.join(dir_path, "residue"+Const.OVERLAY_META)
    residue_metafile, residue_files = \
            generate_overlayfile(residue_deltalist, options, 
            base_hashvalue, os.path.getsize(resumed_vm.resumed_disk), 
            os.path.getsize(residue_mem.name),
            residue_metapath, residue_prefix, print_out)

    # 4. merge with previous deltalist
    merged_list = delta.residue_merge_deltalist(original_deltalist, \
            residue_deltalist, print_out)

    # 5. create_overlayfile
    image_name = os.path.basename(base_disk).split(".")[0]
    dir_path = os.path.dirname(base_mem)
    overlay_prefix = os.path.join(dir_path, image_name+Const.OVERLAY_FILE_PREFIX)
    overlay_metapath = os.path.join(dir_path, image_name+Const.OVERLAY_META)

    overlay_metafile, overlay_files = \
            generate_overlayfile(merged_list, options, 
            base_hashvalue, os.path.getsize(resumed_vm.resumed_disk), 
            os.path.getsize(residue_mem.name),
            overlay_metapath, overlay_prefix, print_out)

    # 6. terminting
    resumed_vm.machine = None   # protecting malaccess to machine 
    if os.path.exists(residue_mem.name):
        os.unlink(residue_mem.name);

    return overlay_metafile, overlay_files
    

def synthesis_statistics(meta_info, decomp_overlay_file,
        mem_access_list, disk_access_list, print_out=sys.stdout):
    start_time = time()

    delta_list = DeltaList.fromfile(decomp_overlay_file)
    total_overlay_size = os.path.getsize(decomp_overlay_file)
    delta_dic = dict()
    for delta_item in delta_list:
        delta_dic[delta_item.index] = delta_item

    overlay_mem_chunks = dict()
    overlay_disk_chunks = dict()
    access_per_blobs = dict()
    total_overlay_mem_chunks = 0
    total_overlay_disk_chunks = 0

    # get all overlay chunks from meta info
    for each_file in meta_info[Const.META_OVERLAY_FILES]:
        memory_chunks = each_file[Const.META_OVERLAY_FILE_MEMORY_CHUNKS]
        disk_chunks = each_file[Const.META_OVERLAY_FILE_DISK_CHUNKS]
        blob_name = each_file[Const.META_OVERLAY_FILE_NAME]
        for mem_chunk in memory_chunks:
            index = DeltaItem.get_index(DeltaItem.DELTA_MEMORY, mem_chunk*Memory.Memory.RAM_PAGE_SIZE)
            chunk_size = len(delta_dic[index].get_serialized())
            overlay_mem_chunks[mem_chunk] = {"blob_name":blob_name, 'chunk_size':chunk_size}
        for disk_chunk in disk_chunks:
            index = DeltaItem.get_index(DeltaItem.DELTA_DISK, disk_chunk*Const.CHUNK_SIZE)
            chunk_size = len(delta_dic[index].get_serialized())
            overlay_disk_chunks[disk_chunk] = {"blob_name":blob_name, 'chunk_size':chunk_size}
        # (memory, memory_total, disk, disk_total)
        access_per_blobs[blob_name] = {
                'mem_access':0, 'mem_access_size':0, 'mem_total':len(memory_chunks),
                'disk_access':0, 'disk_access_size':0, 'disk_total':len(disk_chunks),
                'blob_size':each_file[Const.META_OVERLAY_FILE_SIZE]}
        total_overlay_mem_chunks += len(memory_chunks)
        total_overlay_disk_chunks += len(disk_chunks)

    # compare real accessed chunks with overlay chunk list
    overlay_mem_access_count = 0
    overlay_disk_access_count = 0
    overlay_mem_access_size = 0
    overlay_disk_access_size = 0
    for access_chunk in mem_access_list:
        if overlay_mem_chunks.get(access_chunk, None) != None:
            index = DeltaItem.get_index(DeltaItem.DELTA_MEMORY, access_chunk*Memory.Memory.RAM_PAGE_SIZE)
            chunk_size = len(delta_dic[index].get_serialized())
            blob_name = overlay_mem_chunks.get(access_chunk)['blob_name']
            chunk_size = overlay_mem_chunks.get(access_chunk)['chunk_size']
            access_per_blobs[blob_name]['mem_access'] += 1 # 0: memory
            access_per_blobs[blob_name]['mem_access_size'] += chunk_size
            overlay_mem_access_count += 1
            overlay_mem_access_size += chunk_size
    for access_chunk in disk_access_list:
        if overlay_disk_chunks.get(access_chunk, None) != None:
            index = DeltaItem.get_index(DeltaItem.DELTA_DISK, access_chunk*Const.CHUNK_SIZE)
            chunk_size = len(delta_dic[index].get_serialized())
            blob_name = overlay_disk_chunks.get(access_chunk)['blob_name']
            chunk_size = overlay_disk_chunks.get(access_chunk)['chunk_size']
            access_per_blobs[blob_name]['disk_access'] += 1
            access_per_blobs[blob_name]['disk_access_size'] += chunk_size
            overlay_disk_access_count += 1
            overlay_disk_access_size += chunk_size

    print_out.write("-------------------------------------------------\n")
    print_out.write("## Synthesis Statistics (took %f seconds) ##\n" % (time()-start_time))
    print_out.write("Overlay acccess count / total overlay count\t: %d / %d = %05.2f %%\n" % \
            (overlay_mem_access_count+overlay_disk_access_count,\
            total_overlay_mem_chunks+total_overlay_disk_chunks, \
            100.0 * (overlay_mem_access_count+overlay_disk_access_count)/ (total_overlay_mem_chunks+total_overlay_disk_chunks)))
    print_out.write("Overlay acccess size / total overlay size\t: %10.3d MB/ %10.3f MB= %05.2f %%\n" % \
            ((overlay_mem_access_size+overlay_disk_access_size)/1024.0/1024, \
            (total_overlay_size/1024.0/1024),\
            100.0 * (overlay_mem_access_size+overlay_disk_access_size)/total_overlay_size))
    try:
        print_out.write("  Memory Count: Overlay memory acccess / total memory overlay\t: %d / %d = %05.2f %%\n" % \
                (overlay_mem_access_count, total_overlay_mem_chunks,\
                100.0 * overlay_mem_access_count/total_overlay_mem_chunks))
        print_out.write("  Memory Size: Overlay memory acccess / total overlay\t: %d / %d = %05.2f %%\n" % \
                (overlay_mem_access_size, total_overlay_size,\
                100.0 * overlay_mem_access_size/total_overlay_size))
        print_out.write("  Disk Count: Overlay acccess / total disk overlay\t: %d / %d = %05.2f %%\n" % \
                (overlay_disk_access_count, total_overlay_disk_chunks, \
                100.0 * overlay_disk_access_count/total_overlay_disk_chunks))
        print_out.write("  Disk Size: Overlay acccess / total overlay\t: %d / %d = %05.2f %%\n" % \
                (overlay_disk_access_size, total_overlay_size, \
                100.0 * overlay_disk_access_size/total_overlay_size))
        print_out.write("  EXTRA (count): Overlay memory acccess / VM memory access\t: %d / %d = %05.2f %%\n" % \
                (overlay_mem_access_count, len(mem_access_list), \
                100.0 * overlay_mem_access_count/len(mem_access_list)))
        print_out.write("  EXTRA (count): Overlay disk acccess / VM disk access\t: %d / %d = %05.2f %%\n" % \
                (overlay_disk_access_count, len(disk_access_list), \
                100.0 * overlay_disk_access_count/len(disk_access_list)))
    except ZeroDivisionError as e:
        pass
    used_blob_count = 0
    used_blob_size = 0
    for blob_name in access_per_blobs.keys():
        mem_access = access_per_blobs[blob_name]['mem_access']
        mem_access_size = access_per_blobs[blob_name]['mem_access_size']
        total_mem_chunks = access_per_blobs[blob_name]['mem_total']
        disk_access = access_per_blobs[blob_name]['disk_access']
        disk_access_size = access_per_blobs[blob_name]['disk_access_size']
        total_disk_chunks = access_per_blobs[blob_name]['disk_total']
        if mem_access > 0:
            used_blob_count += 1
            used_blob_size += access_per_blobs[blob_name]['blob_size']
        if total_mem_chunks != 0:
            pass
        '''
            print_out.write("    %s\t:\t%d/%d\t=\t%5.2f is used (%d bytes uncompressed)\n" % \
                    (blob_name, mem_access+disk_access, \
                    total_mem_chunks+total_disk_chunks, \
                    (mem_access+disk_access)*100.0/(total_mem_chunks+total_disk_chunks),
                    (mem_access_size+disk_access_size)))
                    '''
    print_out.write("%d blobs (%f MB) are required out of %d (%05.2f %%)\n" % \
            (used_blob_count, used_blob_size/1024.0/1024, len(access_per_blobs.keys()), \
            used_blob_count*100.0/len(access_per_blobs.keys())))
    print_out.write("-------------------------------------------------\n")



'''External API Start
'''
def validate_congifuration():
    cmd = "%s --version" % Const.QEMU_BIN_PATH
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    out, err = proc.communicate()
    if len(err) > 0:
        print "KVM validation Error: %s" % (err)
        return False
    if out.find("Cloudlet") < 0:
        print "KVM validation Error, Incorrect Version:\n%s" % (out)
        return False
    return True


def create_baseVM(disk_image_path):
    # Create Base VM(disk, memory) snapshot using given VM disk image
    # :param disk_image_path : file path of the VM disk image
    # :returns: (generated base VM disk path, generated base VM memory path)

    # Check DB
    (base_diskmeta, base_mempath, base_memmeta) = \
            Const.get_basepath(disk_image_path)

    # check sanity
    if not os.path.exists(Const.TEMPLATE_XML):
        raise CloudletGenerationError("Cannot find Base VM default XML at %s\n" \
                % Const.TEMPLATE_XML)
    if not os.path.exists(Const.TEMPLATE_OVF):
        raise CloudletGenerationError("Cannot find ovf file for AMIt %s\n" \
                % Const.TEMPLATE_OVF)
    if os.path.exists(base_mempath):
        warning_msg = "Warning: (%s) exist.\nAre you sure to overwrite? (y/N) " \
                % (base_mempath)
        ret = raw_input(warning_msg)
        if str(ret).lower() != 'y':
            sys.exit(1)

    # allow write permission to base disk and delete all previous files
    os.chmod(disk_image_path, stat.S_IRUSR | stat.S_IWUSR | stat.S_IXUSR)
    if os.path.exists(base_diskmeta):
        os.unlink(base_diskmeta)
    if os.path.exists(base_mempath):
        os.unlink(base_mempath)
    if os.path.exists(base_memmeta):
        os.unlink(base_memmeta)

    # edit default XML to have new disk path
    conn = get_libvirt_connection()
    xml = ElementTree.fromstring(open(Const.TEMPLATE_XML, "r").read())
    new_xml_string = _convert_xml(xml, conn, disk_path=disk_image_path, uuid=str(uuid4()))

    # launch VM & wait for end of vnc
    machine = None
    try:
        machine = run_vm(conn, new_xml_string, wait_vnc=True)
        # make memory snapshot
        # VM has to be paused first to perform stable disk hashing
        save_mem_snapshot(machine, base_mempath)
        base_mem = Memory.hashing(base_mempath)
        base_mem.export_to_file(base_memmeta)

        # generate disk hashing
        # TODO: need more efficient implementation, e.g. bisect
        base_hashvalue = Disk.hashing(disk_image_path, base_diskmeta, print_out = sys.stdout)
    except Exception as e:
        sys.stderr.write(str(e) + "\n")
        if machine:
            machine.destroy()
        sys.exit(1)

    # save the result to DB
    dbconn = db_api.DBConnector()
    new_basevm = db_table.BaseVM(disk_image_path, base_hashvalue)
    dbconn.add_item(new_basevm)

    # write protection
    os.chmod(disk_image_path, stat.S_IRUSR)
    os.chmod(base_diskmeta, stat.S_IRUSR)
    os.chmod(base_mempath, stat.S_IRUSR)
    os.chmod(base_memmeta, stat.S_IRUSR)

    return disk_image_path, base_mempath

def _reconstruct_mem_deltalist(base_disk, base_mem, overlay_filepath):
    ret_deltalist = list()
    deltalist = DeltaList.fromfile(overlay_filepath)

    #const
    import struct
    import tool
    import mmap

    # initialize reference data to use mmap
    base_disk_fd = open(base_disk, "rb")
    raw_disk = mmap.mmap(base_disk_fd.fileno(), 0, prot=mmap.PROT_READ)
    base_mem_fd = open(base_mem, "rb")
    raw_mem = mmap.mmap(base_mem_fd.fileno(), 0, prot=mmap.PROT_READ)
    ZERO_DATA = struct.pack("!s", chr(0x00)) * Const.CHUNK_SIZE
    chunk_size = Const.CHUNK_SIZE
    recovered_data_dict = dict()

    for delta_item in deltalist:
        if type(delta_item) != DeltaItem:
            raise CloudletGenerationError("Failed to reconstruct deltalist")

        #print "recovering %ld/%ld" % (index, len(delta_list))
        if (delta_item.ref_id == DeltaItem.REF_RAW):
            recover_data = delta_item.data
        elif (delta_item.ref_id == DeltaItem.REF_ZEROS):
            recover_data = ZERO_DATA
        elif (delta_item.ref_id == DeltaItem.REF_BASE_MEM):
            offset = delta_item.data
            recover_data = raw_mem[offset:offset+chunk_size]
        elif (delta_item.ref_id == DeltaItem.REF_BASE_DISK):
            offset = delta_item.data
            recover_data = raw_disk[offset:offset+chunk_size]
        elif delta_item.ref_id == DeltaItem.REF_SELF:
            ref_index = delta_item.data
            self_ref_data = recovered_data_dict.get(ref_index, None)
            if self_ref_data == None:
                msg = "Cannot find self reference: type(%ld), offset(%ld), \
                        index(%ld), ref_index(%ld)" % \
                        (delta_item.delta_type, delta_item.offset, \
                        delta_item.index, ref_index)
                raise MemoryError(msg)
            recover_data = self_ref_data
        elif delta_item.ref_id == DeltaItem.REF_XDELTA:
            patch_data = delta_item.data
            patch_original_size = delta_item.offset_len
            if delta_item.delta_type == DeltaItem.DELTA_MEMORY:
                base_data = raw_mem[delta_item.offset:delta_item.offset+patch_original_size]
            elif delta_item.delta_type == DeltaItem.DELTA_DISK:
                base_data = raw_disk[delta_item.offset:delta_item.offset+patch_original_size]
            else:
                msg = "Delta should be either disk or memory"
                raise CloudletGenerationError(msg)
            recover_data = tool.merge_data(base_data, patch_data, len(base_data)*5)
        else:
            msg ="Cannot recover: invalid referce id %d" % delta_item.ref_id
            raise MemoryError(msg)

        if len(recover_data) != delta_item.offset_len:
            msg = "Recovered Size Error: %d, ref_id: %s, %ld %ld" % \
                    (len(recover_data), delta_item.ref_id, \
                    delta_item.data_len, delta_item.offset)
            raise CloudletGenerationError(msg)

        # recover
        #delta_item.ref_id = DeltaItem.REF_RAW
        #delta_item.data = recover_data
        #delta_item.data_len = len(recover_data)
        delta_item.hash_value = hashlib.sha256(recover_data).digest()
        recovered_data_dict[delta_item.index] = recover_data
        ret_deltalist.append(delta_item)

    base_disk_fd.close()
    base_mem_fd.close()
    return ret_deltalist


def synthesis(base_disk, meta, disk_only=False, return_residue=False, qemu_args=None):
    # VM Synthesis and run recoverd VM
    # param base_disk : path to base disk
    # param meta : path to meta file for overlay
    # param disk_only: synthesis size VM with only disk image
    # param return_residue: return residue of changed portion
    Log = CloudletLog()

    # decomp
    overlay_filename = NamedTemporaryFile(prefix="cloudlet-overlay-file-")
    meta_info = decomp_overlay(meta, overlay_filename.name, print_out=Log)

    # recover VM
    Log.write("[Debug] recover launch VM\n")
    launch_disk, launch_mem, fuse, delta_proc, fuse_thread = \
            recover_launchVM(base_disk, meta_info, overlay_filename.name, \
            log=Log)

    # resume VM
    resumed_VM = ResumedVM(launch_disk, launch_mem, fuse,
            disk_only=disk_only, qemu_args=qemu_args, log=Log)
    resumed_VM.start()

    delta_proc.start()
    fuse_thread.start()

    delta_proc.join()
    fuse_thread.join()

    # prevent resume VM when modified_mem is not exist
    if os.path.getsize(launch_mem) == 0 and disk_only == False:
        # terminate
        resumed_VM.join()
        resumed_VM.terminate()
        fuse.terminate()
        print "[Error] NO memory overlay file exist. Check disk_only parameter"
        return

    print "[INFO] VM Disk is Fully recovered at %s" % launch_disk
    print "[INFO] VM Memory is Fully recoverd at %s" % launch_mem

    resumed_VM.join()
    connect_vnc(resumed_VM.machine)

    # statistics
    mem_access_list = resumed_VM.monitor.mem_access_chunk_list
    disk_access_list = resumed_VM.monitor.disk_access_chunk_list
    synthesis_statistics(meta_info, overlay_filename.name, \
            mem_access_list, disk_access_list, print_out=Log)

    if return_residue == True:        
        options = Options()
        options.TRIM_SUPPORT = True
        options.FREE_SUPPORT = True
        options.DISK_ONLY = False
        try:
            # FIX: here we revisit all overlay to reconstruct hash information
            # we can leverage Recovered_delta class reconstruction process,
            # but that does not generate hash value
            (base_diskmeta, base_mem, base_memmeta) = \
                    Const.get_basepath(base_disk, check_exist=True)
            prev_mem_deltalist = _reconstruct_mem_deltalist( \
                    base_disk, base_mem, overlay_filename.name)
            residue_meta, residue_files = create_residue(base_disk, \
                    meta_info[Const.META_BASE_VM_SHA256],
                    resumed_VM, options, 
                    prev_mem_deltalist, Log)
            Log.write("[RESULT] Residue\n")
            Log.write("[RESULT]   Metafile : %s\n" % \
                    (os.path.abspath(residue_meta)))
            Log.write("[RESULT]   Files : %s\n" % str(residue_files))
        except CloudletGenerationError, e:
            sys.stderr.write("Cannot create residue : %s" % (str(e)))

    # terminate
    resumed_VM.terminate()
    fuse.terminate()

    if os.path.exists(launch_disk):
        os.unlink(launch_disk)
    if os.path.exists(launch_mem):
        os.unlink(launch_mem)

'''External API End
'''


def main(argv):
    MODE = ('base', 'overlay', 'synthesis', "test")
    USAGE = 'Usage: %prog ' + ("[%s]" % "|".join(MODE)) + " [paths..]"
    VERSION = '%prog 0.1'
    DESCRIPTION = 'Cloudlet Overlay Generation & Synthesis'
    if not validate_congifuration():
        sys.stderr.write("failed to validate configuration\n")
        sys.exit(1)

    parser = OptionParser(usage=USAGE, version=VERSION, description=DESCRIPTION)
    opts, args = parser.parse_args()
    if len(args) == 0:
        parser.error("Incorrect mode among %s" % "|".join(MODE))
    mode = str(args[0]).lower()

    if mode == MODE[0]: #base VM generation
        if len(args) != 2:
            parser.error("Generating base VM requires 1 arguements\n1) VM disk path")
            sys.exit(1)
        # creat base VM
        disk_image_path = args[1]
        disk_path, mem_path = create_baseVM(disk_image_path)
        print "Base VM is created from %s" % disk_image_path
        print "Disk: %s" % disk_path
        print "Mem: %s" % mem_path
    elif mode == MODE[2]:   #synthesis
        if len(args) < 3:
            parser.error("Synthesis requires 2 arguments\n \
                    1) base-disk path\n \
                    2) overlay meta path\n \
                    3) disk if disk only")
            sys.exit(1)
        base_disk_path = args[1]
        meta = args[2]
        if len(args) == 4 and args[3] == 'disk':
            disk_only = True
        else:
            disk_only = False
        synthesis(base_disk_path, meta, disk_only=disk_only, return_residue=False)
    elif mode == 'seperate_overlay':
        if len(args) != 3:
            parser.error("seperating disk and memory overlay need 3 arguments\n \
                    1)meta file\n \
                    2)output directory\n")
            sys.exit(1)
        meta = args[1]
        output_dir = args[2]
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        overlay_path = NamedTemporaryFile(prefix="cloudlet-qemu-log-")
        meta_info = decomp_overlay(meta, overlay_path.name)

        comp_overlay_files = meta_info[Const.META_OVERLAY_FILES]
        comp_overlay_files = [item[Const.META_OVERLAY_FILE_NAME] for item in comp_overlay_files]
        comp_overlay_files = [os.path.join(os.path.dirname(meta), item) for item in comp_overlay_files]
        import shutil
        for comp_file in comp_overlay_files:
            filename = os.path.join(output_dir, os.path.basename(comp_file))
            shutil.copyfile(comp_file, filename)
        delta_list = DeltaList.fromfile(overlay_path.name)
        memory_delta_list = list()
        disk_delta_list = list()
        for delta_item in delta_list:
            if delta_item.delta_type == DeltaItem.DELTA_MEMORY:
                memory_delta_list.append(delta_item)
            elif delta_item.delta_type == DeltaItem.DELTA_DISK:
                disk_delta_list.append(delta_item)
            else:
                raise CloudletGenerationError("No delta type exist")

        disk_uncomp_data = ''
        memory_uncomp_data = ''
        for disk_delta_item in disk_delta_list:
            disk_uncomp_data += (disk_delta_item.get_serialized())
        for mem_delta_item in memory_delta_list:
            memory_uncomp_data += (mem_delta_item.get_serialized())

        total_size = len(disk_delta_list)+len(memory_delta_list)

        print "Delta Item #\tDisk: %d / %d = %f, Memory: %d / %d = %f" % \
                (len(disk_delta_list), total_size, 100.0*len(disk_delta_list)/total_size, \
                len(memory_delta_list), total_size, 100.0*len(memory_delta_list)/total_size)

        disk_uncomp_size = len(disk_uncomp_data)
        memory_uncomp_size = len(memory_uncomp_data)
        total_size = disk_uncomp_size+memory_uncomp_size
        print "Uncomp Size\tDisk: %d / %d = %f, Memory: %d / %d = %f" % \
                (disk_uncomp_size, total_size, 100.0*disk_uncomp_size/total_size, \
                memory_uncomp_size, total_size, 100.0*memory_uncomp_size/total_size)

        from lzma import LZMACompressor
        disk_comp_option = {'format':'xz', 'level':9}
        mem_comp_option = {'format':'xz', 'level':9}
        disk_comp = LZMACompressor(options=disk_comp_option)
        mem_comp = LZMACompressor(options=mem_comp_option)
        disk_comp_data = disk_comp.compress(disk_uncomp_data)
        disk_comp_data += disk_comp.flush()
        mem_comp_data = mem_comp.compress(memory_uncomp_data)
        mem_comp_data += mem_comp.flush()

        disk_comp_size = len(disk_comp_data)
        memory_comp_size = len(mem_comp_data)
        total_size = disk_comp_size+memory_comp_size
        print "Comp Size\tDisk: %d / %d = %f, Memory: %d / %d = %f" % \
                (disk_comp_size, total_size, 100.0*disk_comp_size/total_size, \
                memory_comp_size, total_size, 100.0*memory_comp_size/total_size)
        '''
        disk_overlay_path = os.path.join(output_dir, "disk_overlay")
        memory_overlay_path = os.path.join(output_dir, "memory_overlay")
        disk_blob_list = delta.divide_blobs(disk_delta_list, disk_overlay_path,
                Const.OVERLAY_BLOB_SIZE_KB, Const.CHUNK_SIZE,
                Memory.Memory.RAM_PAGE_SIZE, print_out=sys.stdout)
        memory_blob_list = delta.divide_blobs(memory_delta_list, memory_overlay_path,
                Const.OVERLAY_BLOB_SIZE_KB, Const.CHUNK_SIZE,
                Memory.Memory.RAM_PAGE_SIZE, print_out=sys.stdout)
        '''
    elif mode == "first_run":   #overlay VM creation
        import socket
        import struct

        start_time = time()
        if len(args) != 2:
            parser.error("Resume VM and wait for first run\n \
                    1) Base disk path\n")
            sys.exit(1)
        # create overlay
        disk_path = args[1]

        # waiting for socket command
        port = 10111
        serversock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        serversock.bind(("0.0.0.0", port))
        serversock.listen(1)
        serversock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        print "Waiting for client connection at %d.." % (port)
        (client_socket, address) = serversock.accept()
        app_name_size = struct.unpack("!I", client_socket.recv(4))[0]
        app_name = struct.unpack("!%ds" % app_name_size, client_socket.recv(app_name_size))[0]

        print "start VM resume for application: %s" % app_name
        # start application & VM
        options = Options()
        options.DISK_ONLY = False
        overlay = VM_Overlay(disk_path, options)
        overlay.start()
        overlay.join()
        print "[INFO] overlay metafile : %s" % overlay.overlay_metafile
        print "[INFO] overlay : %s" % str(overlay.overlay_files[0])
        print "[INFO] overlay creation time: %f" % (time()-start_time())

    elif mode == 'dedup_source':
        if len(args) != 4:
            parser.error("analyzing deduplication source need 3 arguments\n \
                    1)meta file\n \
                    2)raw disk\n \
                    3)output logfile\n")
            sys.exit(1)
        meta = args[1]
        raw_disk = args[2]
        output_path = args[3]
        output_log = open(output_path, "w")

        overlay_path = NamedTemporaryFile(prefix="cloudlet-qemu-log-")
        meta_info = decomp_overlay(meta, overlay_path.name)
        delta_list = DeltaList.fromfile(overlay_path.name)

        memory_delta_list = list()
        disk_delta_list = list()
        for delta_item in delta_list:
            if delta_item.ref_id == DeltaItem.REF_BASE_DISK:
                if delta_item.delta_type == DeltaItem.DELTA_MEMORY:
                    memory_delta_list.append(long(delta_item.data))
                elif delta_item.delta_type == DeltaItem.DELTA_DISK:
                    disk_delta_list.append(long(delta_item.data))

        from pprint import pprint
        sectors_overlay_disk = [item/512 for item in disk_delta_list]
        sectors_overlay_memory = [item/512 for item in memory_delta_list]
        sec_file_overlay_disk = xray.get_files_from_sectors(raw_disk, sectors_overlay_disk)
        sec_file_overlay_memory = xray.get_files_from_sectors(raw_disk, sectors_overlay_memory)
        pprint("deduped file at overlay memory", output_log)
        pprint(sec_file_overlay_memory.keys(), output_log)
        pprint("deduped file at overlay disk", output_log)
        pprint(sec_file_overlay_disk.keys(), output_log)

    elif mode == 'compress':
        Log = CloudletLog()
        if len(args) != 3:
            parser.error("recompress requires 2 arguments\n \
                    1)meta file\n \
                    2)output directory\n")
            sys.exit(1)
        meta = args[1]
        output_dir = args[2]
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        overlay_path = os.path.join(output_dir, "overlay")
        meta_info = decomp_overlay(meta, overlay_path)

        blob_size_list = [32, 64, 1024, 1024*1024]
        for order_type in ("access", "linear"):
            delta_list = DeltaList.fromfile(overlay_path)
            sub_dir1 = os.path.join(output_dir, order_type)
            if order_type == "access":
                pass
            elif order_type == "linear":
                delta.reorder_deltalist_linear(Memory.Memory.RAM_PAGE_SIZE, delta_list)
            elif order_type == "random":
                delta.reorder_deltalist_random(Memory.Memory.RAM_PAGE_SIZE, delta_list)

            for blob_size in blob_size_list:
                sub_dir = os.path.join(sub_dir1, "%d" % (blob_size))
                if not os.path.exists(sub_dir):
                    os.makedirs(sub_dir)
                meta_path = os.path.join(sub_dir, "overlay-meta")
                overlay_prefix = os.path.join(sub_dir, "overlay-blob")
                print "Creating %d KB overlays" % blob_size
                blob_list = delta.divide_blobs(delta_list, overlay_prefix,
                        blob_size, Const.CHUNK_SIZE,
                        Memory.Memory.RAM_PAGE_SIZE, print_out=Log)
                _update_overlay_meta(meta_info, meta_path, blob_info=blob_list)
                DeltaList.statistics(delta_list, print_out=sys.stdout)
    elif mode == 'reorder':
        Log = CloudletLog()
        if len(args) != 5:
            print args
            parser.error("Reordering requires 4 arguments\n \
                    1)[linear | access-pattern file path]\n \
                    2)meta file\n \
                    3)blob size in kb\n \
                    4)output directory\n")
            sys.exit(1)

        access_pattern_file = args[1]
        meta = args[2]
        blob_size_kb = int(args[3])
        output_dir = args[4]
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)

        # decomp
        overlay_path = os.path.join(output_dir, "precise.overlay")
        new_meta_path = os.path.join(output_dir, "precise.overlay-meta")
        meta_info = decomp_overlay(meta, overlay_path)
        delta_list = DeltaList.fromfile(overlay_path)
        # reorder
        if access_pattern_file == "linear":
            delta.reorder_deltalist_linear(Memory.Memory.RAM_PAGE_SIZE, delta_list)
        else:
            delta.reorder_deltalist_file(access_pattern_file,
                    Memory.Memory.RAM_PAGE_SIZE, delta_list)
        DeltaList.statistics(delta_list, print_out=sys.stdout)
        blob_list = delta.divide_blobs(delta_list, overlay_path,
                blob_size_kb, Const.CHUNK_SIZE,
                Memory.Memory.RAM_PAGE_SIZE, print_out=Log)
        _update_overlay_meta(meta_info, new_meta_path, blob_info=blob_list)

    elif mode == 'test_overlay_download':    # To be delete
        base_disk_path = "/home/krha/cloudlet/image/nova/base_disk"
        base_mem_path = "/home/krha/cloudlet/image/nova/base_memory"
        overlay_disk_url = "http://dagama.isr.cs.cmu.edu/overlay/nova_overlay_disk.lzma"
        overlay_mem_url = "http://dagama.isr.cs.cmu.edu/overlay/nova_overlay_mem.lzma"
        launch_disk, launch_mem = recover_launchVM_from_URL(base_disk_path, base_mem_path, overlay_disk_url, overlay_mem_url)
        conn = get_libvirt_connection()
        machine=run_snapshot(conn, launch_disk, launch_mem)
        connect_vnc(machine)

    elif mode == 'test_new_xml':    # To be delete
        in_path = args[1]
        out_path = in_path + ".new"
        hdr = vmnetx._QemuMemoryHeader(open(in_path))
        domxml = ElementTree.fromstring(hdr.xml)
        domxml.find('uuid').text = "new-uuid"
        new_xml = ElementTree.tostring(domxml)
        copy_with_xml(in_path, out_path, new_xml)

        hdr = vmnetx._QemuMemoryHeader(open(out_path))
        domxml = ElementTree.fromstring(hdr.xml)
        print "new xml is changed uuid to " + domxml.find('uuid').text
    elif mode == 'nic':
        mem_path = args[1]
        conn = get_libvirt_connection()
        hdr = vmnetx._QemuMemoryHeader(open(mem_path))
        rettach_nic(conn, hdr.xml)
    else:
        print "Invalid command"
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
