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
import subprocess
import Memory
import Disk
import vmnetfs
import vmnetx
import stat
import delta
import xray
import hashlib
import msgpack
from Const import Const
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
            self.logfile = open(filename, "w+")
        else:
            self.logfile = open(Const.OVERLAY_LOG, "w+")
        self.mute = open("/dev/null", "wrb")

    def write(self, log):
        self.logfile.write(log)
        sys.stdout.write(log)

    def flush(self):
        sys.stdout.flush()


def copy_disk(in_path, out_path):
    print "[INFO] Copying disk image to %s" % out_path
    cmd = "cp %s %s" % (in_path, out_path)
    cp_proc = subprocess.Popen(cmd, shell=True)
    cp_proc.wait()
    if cp_proc.returncode != 0:
        raise IOError("Copy failed: from %s to %s " % (in_path, out_path))


def convert_xml(xml, conn, vm_name=None, disk_path=None, uuid=None, logfile=None):
    if vm_name:
        name_element = xml.find('name')
        if not name_element:
            raise CloudletGenerationError("Malfomed XML input: %s", Const.TEMPLATE_XML)
        name_element.text = vm_name

    if uuid:
        uuid_element = xml.find('uuid')
        uuid_element.text = str(uuid)

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
    if cdrom_source == None:
        raise CloudletGenerationError("Cannot find cdrom source at VM TEMPLATE: %s", Const.TEMPLATE_XML)
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

    return ElementTree.tostring(xml)


def get_libvirt_connection():
    conn = libvirt.open("qemu:///session")
    return conn


def create_baseVM(disk_image_path):
    # Create Base VM(disk, memory) snapshot using given VM disk image
    # :param disk_image_path : file path of the VM disk image
    # :returns: (generated base VM disk path, generated base VM memory path)

    (base_diskmeta, base_mempath, base_memmeta) = \
            Const.get_basepath(disk_image_path)
    base_hashpath = Const.get_basehash_path(disk_image_path)
    Log = CloudletLog(os.path.basename(base_diskmeta)+Const.OVERLAY_LOG)

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
    if os.path.exists(base_hashpath):
        os.unlink(base_hashpath)

    # edit default XML to have new disk path
    conn = get_libvirt_connection()
    xml = ElementTree.fromstring(open(Const.TEMPLATE_XML, "r").read())
    new_xml_string = convert_xml(xml, conn, disk_path=disk_image_path, uuid=str(uuid4()))

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
        Disk.hashing(disk_image_path, base_diskmeta, print_out=sys.stdout)
        base_hashvalue = hashlib.sha256(open(disk_image_path, "rb").read()).hexdigest()
        open(base_hashpath, "wrb").write(base_hashvalue)
    except Exception as e:
        sys.stderr.write(str(e)+"\n")
        if machine:
            machine.destroy()
        sys.exit(1)

    # write protection
    os.chmod(disk_image_path, stat.S_IRUSR)
    os.chmod(base_diskmeta, stat.S_IRUSR)
    os.chmod(base_mempath, stat.S_IRUSR)
    os.chmod(base_memmeta, stat.S_IRUSR)
    os.chmod(base_hashpath, stat.S_IRUSR)
    return disk_image_path, base_mempath


def create_overlay(base_image, disk_only=False):
    # create user customized overlay.
    # First resume VM, then let user edit its VM
    # Finally, return disk/memory binary as an overlay
    # base_image: path to base disk
    Log = CloudletLog()

    (base_diskmeta, base_mem, base_memmeta) = \
            Const.get_basepath(base_image, check_exist=True)
    base_hash_path = Const.get_basehash_path(base_image)
    base_hash_value = open(base_hash_path, "rb").read()
    
    # filename for overlay VM
    qemu_logfile = NamedTemporaryFile(prefix="cloudlet-qemu-log-", delete=False)
    image_name = os.path.basename(base_image).split(".")[0]
    dir_path = os.path.dirname(base_mem)
    overlay_path = os.path.join(dir_path, image_name+Const.OVERLAY_FILE_PREFIX)
    
    # make FUSE disk & memory
    fuse = run_fuse(Const.VMNETFS_PATH, Const.CHUNK_SIZE, 
            base_image, os.path.getsize(base_image),
            base_mem, os.path.getsize(base_mem))
    modified_disk = os.path.join(fuse.mountpoint, 'disk', 'image')
    base_mem_fuse = os.path.join(fuse.mountpoint, 'memory', 'image')
    modified_mem = NamedTemporaryFile(prefix="cloudlet-mem-", delete=False)
    # monitor modified chunks
    stream_modified = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_modified')
    stream_access = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_accessed')
    memory_access = os.path.join(fuse.mountpoint, 'memory', 'streams', 'chunks_accessed')
    monitor = vmnetfs.StreamMonitor()
    monitor.add_path(stream_modified, vmnetfs.StreamMonitor.DISK_MODIFY)
    monitor.add_path(stream_access, vmnetfs.StreamMonitor.DISK_ACCESS)
    monitor.add_path(memory_access, vmnetfs.StreamMonitor.MEMORY_ACCESS)
    monitor.start()
    qemu_monitor = vmnetfs.FileMonitor(qemu_logfile.name, vmnetfs.FileMonitor.QEMU_LOG)
    qemu_monitor.start()

    # 1-1. resume & get modified disk
    conn = get_libvirt_connection()
    machine = run_snapshot(conn, modified_disk, base_mem_fuse, qemu_logfile=qemu_logfile.name)
    connect_vnc(machine)

    # 1-2. Stop monitoring for memory access (snapshot will create a lot of access)
    #      and get modified memory
    monitor.del_path(vmnetfs.StreamMonitor.MEMORY_ACCESS)
    # TODO: support stream of modified memory rather than tmp file
    if not disk_only:
        save_mem_snapshot(machine, modified_mem.name)

    # 1-3. get hashlist of base memory and disk
    basemem_hashlist = Memory.base_hashlist(base_memmeta)
    basedisk_hashlist = Disk.base_hashlist(base_diskmeta)

    # 1-4. get dma & discard information
    if Const.TRIM_SUPPORT:
        dma_dict, trim_dict = Disk.parse_qemu_log(qemu_logfile.name, Const.CHUNK_SIZE)
        if len(trim_dict) == 0:
            print "[WARNING] No TRIM Discard, Check /etc/fstab configuration"
    else:
        dma_dict = dict()
        trim_dict = dict()
    if Const.FREE_SUPPORT:
        freed_counter_ret = dict()
    else:
        freed_counter_ret = None
    # 1-5. get used sector information from x-ray
    used_blocks_dict = None
    if Const.XRAY_SUPPORT:
        used_blocks_dict = xray.get_used_blocks(modified_disk)

    # 2-1. get memory overlay
    if not disk_only:
        mem_deltalist= Memory.create_memory_deltalist(modified_mem.name, 
                basemem_meta=base_memmeta, basemem_path=base_mem,
                freed_counter_ret = freed_counter_ret,
                print_out=Log)
    else:
        mem_deltalist = list()

    # 2-2. get disk overlay
    m_chunk_dict = monitor.modified_chunk_dict
    disk_statistics = dict()
    disk_deltalist = Disk.create_disk_deltalist(modified_disk,
            m_chunk_dict, Const.CHUNK_SIZE,
            basedisk_hashlist=basedisk_hashlist, basedisk_path=base_image,
            trim_dict=trim_dict,
            dma_dict=dma_dict,
            used_blocks_dict=used_blocks_dict,
            ret_statistics=disk_statistics,
            print_out=Log)

    # 2-3. Merge disk & memory delta_list to generate overlay file
    merged_deltalist = delta.create_overlay(
            mem_deltalist, Memory.Memory.RAM_PAGE_SIZE,
            disk_deltalist, Const.CHUNK_SIZE,
            basedisk_hashlist=basedisk_hashlist,
            basemem_hashlist=basemem_hashlist,
            print_out=Log)

    free_pfn_counter = 0
    if Const.FREE_SUPPORT:
        free_pfn_counter = long(freed_counter_ret.get("freed_counter", 0))

    # TO BE DELETE: DMA performance checking
    # _test_dma_accuracy(dma_dict, disk_deltalist, mem_deltalist)

    if Const.XRAY_SUPPORT == True:
        # 3-1. list-up all the files that is associated with overlay sectors
        xray_start_time = time()
        xray_log = open("./xray_log", "wrb")
        import pprint
        sectors = [item.offset/512 for item in disk_deltalist]
        sec_file_dict = xray.get_files_from_sectors(modified_disk, sectors)
        pprint.pprint(sec_file_dict, xray_log)

        # 3-2. To be deleted
        xray_log.write("-------TRIM VS XRAY\n")
        trim_chunk_set = set(disk_statistics.get('trimed_list', list()))
        xray_chunk_set = set(disk_statistics.get('xrayed_list', list()))
        xray_log.write("trimed - xray:\n%s\n" % str(trim_chunk_set-xray_chunk_set))
        xray_log.write("xray - trimed:\n%s\n" % str(xray_chunk_set-trim_chunk_set))
        diff_list = list(xray_chunk_set-trim_chunk_set)
        diff_sectors = [item/8 for item in diff_list]
        sec_file_dict = xray.get_files_from_sectors(modified_disk, diff_sectors)
        pprint.pprint(sec_file_dict, xray_log)
        xray_log.write("trim(%ld) == xray(%ld)\n" % (disk_statistics.get('trimed', 0), disk_statistics.get('xrayed', 0)))
        xray_log.write("-------END\n")
        xray_end_time = time()
        Log.write("[Debug] WASTED TIME FOR XRAY LOGGING: %f\n" % (xray_end_time-xray_start_time))

    # 3. Reorder transfer order & Compression
    Log.write("[DEBUG][REORDER] change chunk ordering by mem access\n")
    mem_access_list = monitor.mem_access_chunk_list
    delta.reorder_deltalist(mem_access_list, Const.CHUNK_SIZE, merged_deltalist)
    '''
    Log.write("[DEBUG][REORDER] change chunk ordering by offset\n")
    delta.reorder_deltalist_linear(Const.CHUNK_SIZE, merged_deltalist)
    '''    
    Log.write("[DEBUG][LZMA] Compressing overlay blobs\n")
    blob_list = delta.divide_blobs(merged_deltalist, overlay_path, 
            Const.OVERLAY_BLOB_SIZE_KB, Const.CHUNK_SIZE,
            Memory.Memory.RAM_PAGE_SIZE, print_out=Log)
    if Const.XRAY_SUPPORT:
        disk_discarded_count = disk_statistics.get('xrayed', 0)
    else:
        disk_discarded_count = disk_statistics.get('trimed', 0)
    DeltaList.statistics(merged_deltalist, print_out=Log, 
            mem_discarded=free_pfn_counter,
            disk_discarded=disk_discarded_count)

    # 4. create metadata
    overlay_metafile = os.path.join(dir_path, image_name+Const.OVERLAY_META)
    if not disk_only:
        _create_overlay_meta(base_hash_value, overlay_metafile, 
                modified_disk, modified_mem.name, blob_list)
    else:
        _create_overlay_meta(base_hash_value, overlay_metafile, 
                modified_disk, base_mem, blob_list)


    # 4. terminting
    fuse.terminate()
    monitor.terminate()
    qemu_monitor.terminate()
    monitor.join()
    qemu_monitor.join()
    os.unlink(modified_mem.name)
    if os.path.exists(qemu_logfile.name):
        os.unlink(qemu_logfile.name)

    blob_files = [item[Const.META_OVERLAY_FILE_NAME] for item in blob_list]
    return (overlay_metafile, blob_files)


def _create_overlay_meta(base_hash, overlay_metafile, modified_disk, modified_mem, 
        blob_info):
    fout = open(overlay_metafile, "wrb")

    meta_dict = dict()
    meta_dict[Const.META_BASE_VM_SHA256] = base_hash
    meta_dict[Const.META_RESUME_VM_DISK_SIZE] = os.path.getsize(modified_disk)
    meta_dict[Const.META_RESUME_VM_MEMORY_SIZE] = os.path.getsize(modified_mem)
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
    log = kwargs.get('log', open("/dev/null", "wrb"))
    nova_util = kwargs.get('nova_util', None)

    (base_diskmeta, base_mem, base_memmeta) = \
            Const.get_basepath(base_image, check_exist=True)
    modified_mem = NamedTemporaryFile(prefix="cloudlet-recoverd-mem-", delete=False)
    modified_img = NamedTemporaryFile(prefix="cloudlet-recoverd-img-", delete=False)

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
    fuse = run_fuse(Const.VMNETFS_PATH, Const.CHUNK_SIZE, 
            base_image, vm_disk_size, base_mem, vm_memory_size,
            modified_img.name,  disk_overlay_map,
            modified_mem.name, memory_overlay_map, **kwargs)
    print "[INFO] Start FUSE"

    # Recover Modified Memory
    pipe_parent, pipe_child = Pipe()
    delta_proc = delta.Recovered_delta(base_image, base_mem, overlay_file, \
            modified_mem.name, vm_memory_size, 
            modified_img.name, vm_disk_size, Const.CHUNK_SIZE, 
            out_pipe=pipe_child)
    fuse_thread = vmnetfs.FuseFeedingThread(fuse, 
            pipe_parent, delta.Recovered_delta.END_OF_PIPE)
    return [modified_img.name, modified_mem.name, fuse, delta_proc, fuse_thread]


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
    try:
        ret = machine.save(fout_path)
    except libvirt.libvirtError, e:
        raise CloudletGenerationError(str(e))
    if ret != 0:
        raise CloudletGenerationError("libvirt: Cannot save memory state")


def run_snapshot(conn, disk_image, mem_snapshot, **kwargs):
    # kwargs
    # qemu_logfile      :   log file for QEMU-KVM
    # resume_time       :   write back the resumed_time
    resume_time = kwargs.get('resume_time', None)
    logfile = kwargs.get('qemu_logfile', None)
    if resume_time != None:
        start_resume_time = time()

    # read embedded XML at memory snapshot to change disk path
    hdr = vmnetx._QemuMemoryHeader(open(mem_snapshot))
    xml = ElementTree.fromstring(hdr.xml)
    new_xml_string = convert_xml(xml, conn, disk_path=disk_image, 
            uuid=uuid4(), logfile=logfile)

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


def connect_vnc(machine):
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
        message = "%s\nXML: %s" % (str(e), xml)
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


def synthesis(base_disk, meta, disk_only=False):
    # VM Synthesis and run recoverd VM
    # param base_disk : path to base disk
    # param meta : path to meta file for overlay
    # param overlay_disk : path to overlay disk file
    # param overlay_mem : path to overlay memory file
    Log = CloudletLog()

    # decomp
    overlay_filename = NamedTemporaryFile(prefix="cloudlet-overlay-file-")
    meta_info = decomp_overlay(meta, overlay_filename.name, print_out=Log)

    # recover VM
    Log.write("[Debug] recover launch VM\n")
    modified_img, modified_mem, fuse, delta_proc, fuse_thread = \
            recover_launchVM(base_disk, meta_info, overlay_filename.name, log=Log)

    # resume VM
    resumed_VM = ResumedVM(modified_img, modified_mem, fuse, disk_only=disk_only)
    resumed_VM.start()

    delta_proc.start()
    fuse_thread.start()

    delta_proc.join()
    fuse_thread.join()

    # prevent resume VM when modified_mem is not exist
    if os.path.getsize(modified_mem) == 0 and disk_only == False:
        # terminate
        resumed_VM.join()
        resumed_VM.terminate()
        fuse.terminate()
        print "[Error] NO memory overlay file exist. Check disk_only parameter"
        return

    print "[INFO] VM Disk is Fully recovered at %s" % modified_img
    print "[INFO] VM Memory is Fully recoverd at %s" % modified_mem
    #raw_input("waiting key input")

    resumed_VM.join()
    connect_vnc(resumed_VM.machine)

    # statistics
    mem_access_list = resumed_VM.monitor.mem_access_chunk_list
    disk_access_list = resumed_VM.monitor.disk_access_chunk_list
    synthesis_statistics(meta_info, overlay_filename.name, \
            mem_access_list, disk_access_list, print_out=Log)

    # terminate
    resumed_VM.terminate()
    fuse.terminate()


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



class ResumedVM(threading.Thread):
    def __init__(self, modified_img, modified_mem, fuse, disk_only=False, **kwargs):
        # kwargs
        # qemu_logfile      :   log file for QEMU-KVM

        # monitor modified chunks
        self.machine = None
        self.disk_only = disk_only
        self.fuse = fuse
        self.modified_img = modified_img
        self.modified_mem = modified_mem
        self.qemu_logfile = NamedTemporaryFile(prefix="cloudlet-qemu-log-", delete=False)
        self.residue_img = os.path.join(fuse.mountpoint, 'disk', 'image')
        self.residue_mem = os.path.join(fuse.mountpoint, 'memory', 'image')
        stream_modified = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_modified')
        stream_disk_access = os.path.join(fuse.mountpoint, 'disk', 'streams', 'chunks_accessed')
        stream_memory_access = os.path.join(fuse.mountpoint, 'memory', 'streams', 'chunks_accessed')
        self.monitor = vmnetfs.StreamMonitor()
        self.monitor.add_path(stream_modified, vmnetfs.StreamMonitor.DISK_MODIFY)
        self.monitor.add_path(stream_disk_access, vmnetfs.StreamMonitor.DISK_ACCESS)
        self.monitor.add_path(stream_memory_access, vmnetfs.StreamMonitor.MEMORY_ACCESS)
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
                new_xml_string = convert_xml(xml, conn, disk_path=self.residue_img, uuid=str(uuid4()))
                self.machine = run_vm(conn, new_xml_string, vnc_disable=True)
            else:
                self.machine=run_snapshot(conn, self.residue_img, self.residue_mem, 
                        qemu_logfile=self.qemu_logfile.name, resume_time=self.resume_time)
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
        if os.path.exists(self.modified_img):
            os.unlink(self.modified_img)
        if os.path.exists(self.modified_mem):
            os.unlink(self.modified_mem)
        if os.path.exists(self.qemu_logfile.name):
            os.unlink(self.qemu_logfile.name)


def validate_congifuration():
    cmd = "kvm --version"
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    out, err = proc.communicate()
    if len(err) > 0:
        print "KVM validation Error: %s" % (err)
        return False
    if out.find("Cloudlet") < 0:
        print "KVM validation Error, Incorrect Version:\n%s" % (out)
        return False
    return True


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
    elif mode == MODE[1]:   #overlay VM creation
        if len(args) < 2:
            parser.error("Overlay Creation requires 1 arguments\n \
                    1) Base disk path\n \
                    2) disk if disk only")
            sys.exit(1)
        # create overlay
        disk_path = args[1]
        if len(args) == 3 and args[2] == 'disk':
            disk_only = True
        else:
            disk_only = False
        overlay_files = create_overlay(disk_path, disk_only)
        print "[INFO] overlay metafile : %s" % overlay_files[0]
        print "[INFO] overlay : %s" % str(overlay_files[1])
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
        synthesis(base_disk_path, meta, disk_only)
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

        blob_size_list = [4, 16, 128, 512, 1024, 1024*8, 1024*16, 1024*64, 1024*1024]
        #blob_size_list = [16]
        for order_type in ("access", "linear", "random"):
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
