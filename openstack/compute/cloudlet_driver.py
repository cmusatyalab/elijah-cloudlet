# Elijah: Cloudlet Infrastructure for Mobile Computing
# Copyright (C) 2011-2013 Carnegie Mellon University
# Author: Kiryong Ha (krha@cmu.edu)
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
import uuid
import hashlib
import urllib2

from nova.virt.libvirt import blockinfo
from nova.compute import power_state
from nova import exception
from nova import utils
from nova.virt import driver
from nova.virt.libvirt import utils as libvirt_utils
from nova.image import glance
from nova.compute import task_states
from nova.openstack.common import fileutils
from nova.openstack.common import log as openstack_logging

from nova.virt.libvirt import driver as libvirt_driver
from nova.compute.cloudlet_api import CloudletAPI

from lzma import LZMADecompressor
from xml.etree import ElementTree
from cloudlet import synthesis
from cloudlet import msgpack
from cloudlet.Configuration import Const as Cloudlet_Const


LOG = openstack_logging.getLogger(__name__)
synthesis.LOG = LOG  # overwrite cloudlet's own log


class CloudletDriver(libvirt_driver.LibvirtDriver):

    def __init__(self, read_only=False):
        super(CloudletDriver, self).__init__(read_only)

        # manage VM overlay list
        self.vm_overlay_dict = dict()
        # manage synthesized VM list
        self.synthesized_vm_dics = dict()

    def _get_snapshot_metadata(self, virt_dom, context, instance, snapshot_id):
        _image_service = glance.get_remote_image_service(context, snapshot_id)
        snapshot_image_service, snapshot_image_id = _image_service
        snapshot = snapshot_image_service.show(context, snapshot_image_id)
        metadata = {'is_public': False,
                    'status': 'active',
                    'name': snapshot['name'],
                    'properties': {
                                'kernel_id': instance['kernel_id'],
                                'image_location': 'snapshot',
                                'image_state': 'available',
                                'owner_id': instance['project_id'],
                                'ramdisk_id': instance['ramdisk_id'],
                                }
                    }

        (image_service, image_id) = glance.get_remote_image_service(
            context, instance['image_ref'])
        try:
            base = image_service.show(context, image_id)
        except exception.ImageNotFound:
            base = {}

        if 'architecture' in base.get('properties', {}):
            arch = base['properties']['architecture']
            metadata['properties']['architecture'] = arch

        metadata['disk_format'] = 'raw'
        metadata['container_format'] = base.get('container_format', 'bare')
        return metadata

    def _update_to_glance(self, context, image_service, filepath, 
            meta_id, metadata):
        with libvirt_utils.file_open(filepath) as image_file:
            image_service.update(context,
                                meta_id,
                                metadata,
                                image_file)

    @exception.wrap_exception()
    def cloudlet_base(self, context, instance, vm_name,
            disk_meta_id, memory_meta_id, 
            diskhash_meta_id, memoryhash_meta_id, update_task_state):
        """create base vm and save it to glance
        """ 
        try:
            virt_dom = self._lookup_by_name(instance['name'])
        except exception.InstanceNotFound:
            raise exception.InstanceNotRunning(instance_id=instance['uuid'])

        # pause VM
        self.pause(instance)

        (image_service, image_id) = glance.get_remote_image_service(
            context, instance['image_ref'])

        disk_metadata = self._get_snapshot_metadata(virt_dom, context, 
                instance, disk_meta_id)
        mem_metadata = self._get_snapshot_metadata(virt_dom, context, 
                instance, memory_meta_id)
        diskhash_metadata = self._get_snapshot_metadata(virt_dom, context, 
                instance, diskhash_meta_id)
        memhash_metadata = self._get_snapshot_metadata(virt_dom, context, 
                instance, memoryhash_meta_id)

        disk_path = libvirt_utils.find_disk(virt_dom)
        source_format = libvirt_utils.get_disk_type(disk_path)
        snapshot_name = uuid.uuid4().hex
        (state, _max_mem, _mem, _cpus, _t) = virt_dom.info()
        state = libvirt_driver.LIBVIRT_POWER_STATE[state]

        # creating base vm requires cold snapshotting
        snapshot_backend = self.image_backend.snapshot(disk_path,
                snapshot_name,
                image_type=source_format)

        LOG.info(_("Beginning cold snapshot process"),
                    instance=instance)
        snapshot_backend.snapshot_create()

        update_task_state(task_state=task_states.IMAGE_PENDING_UPLOAD)
        snapshot_directory = libvirt_driver.CONF.libvirt_snapshots_directory
        fileutils.ensure_tree(snapshot_directory)
        with utils.tempdir(dir=snapshot_directory) as tmpdir:
            try:
                out_path = os.path.join(tmpdir, snapshot_name)
                # At this point, base vm should be "raw" format
                snapshot_backend.snapshot_extract(out_path, "raw")
            finally:
                snapshot_backend.snapshot_delete()
                LOG.info(_("Snapshot extracted, beginning image upload"),
                         instance=instance)

            # generate memory snapshop and hashlist
            basemem_path = os.path.join(tmpdir, snapshot_name+"-mem")
            diskhash_path = os.path.join(tmpdir, snapshot_name+"-disk_hash")
            memhash_path = os.path.join(tmpdir, snapshot_name+"-mem_hash")

            update_task_state(task_state=task_states.IMAGE_UPLOADING,
                     expected_state=task_states.IMAGE_PENDING_UPLOAD) 
            
            synthesis._create_baseVM(self._conn, virt_dom, out_path, basemem_path, 
                    diskhash_path, memhash_path, nova_util=libvirt_utils)

            self._update_to_glance(context, image_service, out_path, 
                    disk_meta_id, disk_metadata)
            LOG.info(_("Base disk upload complete"), instance=instance)
            self._update_to_glance(context, image_service, basemem_path, 
                    memory_meta_id, mem_metadata)
            LOG.info(_("Base memory image upload complete"), instance=instance)
            self._update_to_glance(context, image_service, diskhash_path, 
                    diskhash_meta_id, diskhash_metadata)
            LOG.info(_("Base disk upload complete"), instance=instance)
            self._update_to_glance(context, image_service, memhash_path, \
                    memoryhash_meta_id, memhash_metadata)
            LOG.info(_("Base memory image upload complete"), instance=instance)
            
            # restore vm to gracefully terminate using openstack logic
            #self._conn.restore(basemem_path)


    # sperate creating domain and creating network
    def _create_network_only(self, xml, instance, network_info,
                                   block_device_info=None):
        """Do required network setup and skip create domain part."""
        block_device_mapping = driver.block_device_info_get_mapping(
            block_device_info)

        for vol in block_device_mapping:
            connection_info = vol['connection_info']
            disk_dev = vol['mount_device'].rpartition("/")[2]
            disk_info = {
                'dev': disk_dev,
                'bus': blockinfo.get_disk_bus_for_disk_dev(
                    libvirt_driver.CONF.libvirt_type, disk_dev
                    ),
                'type': 'disk',
                }
            self.volume_driver_method('connect_volume',
                                      connection_info,
                                      disk_info)

        self.plug_vifs(instance, network_info)
        self.firewall_driver.setup_basic_filtering(instance, network_info)
        self.firewall_driver.prepare_instance_filter(instance, network_info)
        self.firewall_driver.apply_instance_filter(instance, network_info)

    def create_overlay_vm(self, context, instance, 
            overlay_name, overlay_meta_id, overlay_blob_id, update_task_state):
        try:
            virt_dom = self._lookup_by_name(instance['name'])
        except exception.InstanceNotFound:
            raise exception.InstanceNotRunning(instance_id=instance['uuid'])

        # pause VM
        self.pause(instance)

        # create VM overlay
        (image_service, image_id) = glance.get_remote_image_service(
            context, instance['image_ref'])
        meta_metadata = self._get_snapshot_metadata(virt_dom, context, \
                instance, overlay_meta_id)
        blob_metadata = self._get_snapshot_metadata(virt_dom, context, \
                instance, overlay_blob_id)
        update_task_state(task_state=task_states.IMAGE_PENDING_UPLOAD)

        vm_overlay = self.vm_overlay_dict.get(instance['uuid'], None)
        if vm_overlay == None:
            raise exception.InstanceNotRunning(instance_id=instance['uuid'])
        del self.vm_overlay_dict[instance['uuid']]
        vm_overlay.create_overlay()
        meta_filepath = vm_overlay.overlay_metafile
        blob_files = vm_overlay.overlay_files[0]
        print "[INFO] overlay : %s" % str(vm_overlay.overlay_files[0])

        update_task_state(task_state=task_states.IMAGE_UPLOADING,
                    expected_state=task_states.IMAGE_PENDING_UPLOAD)

        # export to glance
        self._update_to_glance(context, image_service, meta_filepath, \
                overlay_meta_id, meta_metadata)
        LOG.info(_("overlay_vm metafile upload complete"), instance=instance)
        self._update_to_glance(context, image_service, blob_files, \
                overlay_blob_id, blob_metadata)
        LOG.info(_("overlay_vm blobfile upload complete"), instance=instance)


    def _get_cache_image(self, context, instance, snapshot_id, suffix=''):
        def basepath(fname='', suffix=suffix):
            return os.path.join(libvirt_utils.get_instance_path(instance),
                                fname + suffix)
        def raw(fname, image_type='raw'):
            return self.image_backend.image(instance, fname, image_type)

        # ensure directories exist and are writable
        fileutils.ensure_tree(basepath(suffix=''))
        fname = hashlib.sha1(snapshot_id).hexdigest()
        LOG.debug(_("cloudlet, caching file at %s" % fname))
        size = instance['root_gb'] * 1024 * 1024 * 1024
        if size == 0:
            size = None

        raw('disk').cache(fetch_func=libvirt_utils.fetch_image,
                            context=context,
                            filename=fname,
                            size=size,
                            image_id=snapshot_id,
                            user_id=instance['user_id'],
                            project_id=instance['project_id'])

        # from cache method at virt/libvirt/imagebackend.py 
        abspath = os.path.join(libvirt_driver.CONF.instances_path,
                libvirt_driver.CONF.base_dir_name, fname)
        return abspath

    def _polish_VM_configuration(self, xml):
        # remove cpu element
        cpu_element = xml.find("cpu")
        if cpu_element != None:
            xml.remove(cpu_element)

        # TODO: Handle console/serial element properly
        device_element = xml.find("devices")
        console_elements = device_element.findall("console")
        for console_element in console_elements:
            device_element.remove(console_element)
        serial_elements = device_element.findall("serial")
        for serial_element in serial_elements:
            device_element.remove(serial_element)

        # remove O_DIRECT option since FUSE does not support it
        disk_elements = xml.findall('devices/disk')
        for disk_element in disk_elements:
            disk_type = disk_element.attrib['device']
            if disk_type == 'disk':
                hdd_driver = disk_element.find("driver")
                if hdd_driver != None and hdd_driver.get("cache", None) != None:
                    del hdd_driver.attrib['cache']

        xml_str = ElementTree.tostring(xml)
        return xml_str

    def _get_basevm_meta_info(self, image_meta):
        # get memory_snapshot_id for resume case
        memory_snap_id = None
        diskhash_snap_id = None
        memhash_snap_id = None

        meta_data = image_meta.get('properties', None)
        if meta_data and meta_data.get(CloudletAPI.IMAGE_TYPE_BASE_MEM):
            memory_snap_id = str(meta_data.get(CloudletAPI.IMAGE_TYPE_BASE_MEM))
            diskhash_snap_id = str(meta_data.get(CloudletAPI.IMAGE_TYPE_BASE_DISK_HASH))
            memhash_snap_id = str(meta_data.get(CloudletAPI.IMAGE_TYPE_BASE_MEM_HASH))
            LOG.debug(_("cloudlet, get memory_snapshot_id: %s" % str(memory_snap_id)))
            LOG.debug(_("cloudlet, get disk_hash_snapshot_id: %s" % str(diskhash_snap_id)))
            LOG.debug(_("cloudlet, get memory_hash_snapshot_id: %s" % str(memhash_snap_id)))
        return memory_snap_id, diskhash_snap_id, memhash_snap_id

    def _get_VM_overlay_meta(self, instance):
        # get overlay from instance metadata for synthesis case
        overlay_meta_url = overlay_data_url = None
        instance_meta = instance.get('metadata', None)
        LOG.debug(_("instance meta data : %s" % instance_meta))
        overlay_meta_url = overlay_data_url = None
        if instance_meta != None:
            for instance_meta_item in instance_meta:
                if instance_meta_item.get("key") == "overlay_meta_url":
                    overlay_meta_url = instance_meta_item.get("value")
                if instance_meta_item.get("key") == "overlay_blob_url":
                    overlay_data_url = instance_meta_item.get("value")
        return overlay_meta_url, overlay_data_url

    # overwrite original libvirt_driver's spawn method
    def spawn(self, context, instance, image_meta, injected_files,
              admin_password, network_info=None, block_device_info=None):

        # add metadata to the instance
        def _append_metadata(target_instance, metadata_dict):
            original_meta = target_instance.get('metadata', None) or list()
            original_meta.append(metadata_dict)
            target_instance['metadata'] = original_meta


        # get meta info related to VM synthesis
        memory_snap_id, diskhash_snap_id, memhash_snap_id = \
                self._get_basevm_meta_info(image_meta)
        overlay_meta_url, overlay_data_url = self._get_VM_overlay_meta(instance)

        # original openstack logic
        disk_info = blockinfo.get_disk_info(libvirt_driver.CONF.libvirt_type,
                                            instance,
                                            block_device_info,
                                            image_meta)
        xml = self.to_xml(instance, network_info,
                          disk_info, image_meta,
                          block_device_info=block_device_info)

        # handle xml configuration to make a portable VM
        xml_obj = ElementTree.fromstring(xml)
        xml = self._polish_VM_configuration(xml_obj)

        # avoid injecting key, password, and metadata since we're resuming the VM
        original_inject_password = libvirt_driver.CONF.libvirt_inject_password
        original_inject_key = libvirt_driver.CONF.libvirt_inject_key
        original_metadata = instance.get('metadata')
        libvirt_driver.CONF.libvirt_inject_password = None
        libvirt_driver.CONF.libvirt_inject_key = None
        instance['metadata'] = None

        self._create_image(context, instance,
                           disk_info['mapping'],
                           network_info=network_info,
                           block_device_info=block_device_info,
                           files=injected_files,
                           admin_pass=admin_password)

        # revert back the configuration
        libvirt_driver.CONF.libvirt_inject_password = original_inject_password
        libvirt_driver.CONF.libvirt_inject_key = original_inject_key
        instance['metadata'] = original_metadata


        if overlay_meta_url != None and overlay_data_url != None:
            # synthesis from overlay
            LOG.debug(_('cloudlet, synthesis start'))
            # append metadata to the instance
            self._create_network_only(xml, instance, network_info, block_device_info)
            synthesized_vm = self.create_new_using_synthesis(context, instance, 
                    xml, image_meta, overlay_meta_url, overlay_data_url)
            instance_uuid = str(instance.get('uuid', ''))
            self.synthesized_vm_dics[instance_uuid] = synthesized_vm
        elif memory_snap_id != None:
            # resume from memory snapshot
            LOG.debug(_('cloudlet, resume from memory snapshot'))
            # append metadata to the instance
            basedisk_path = self._get_cache_image(context, instance, image_meta['id'])
            basemem_path = self._get_cache_image(context, instance, memory_snap_id)
            diskhash_path = self._get_cache_image(context, instance, diskhash_snap_id)
            memhash_path = self._get_cache_image(context, instance, memhash_snap_id)

            self._create_network_only(xml, instance, network_info, block_device_info)
            self.resume_basevm(instance, xml, basedisk_path, basemem_path, 
                    diskhash_path, memhash_path, image_meta['id'])
        else:
            self._create_domain_and_network(xml, instance, network_info,
                                            block_device_info)

        LOG.debug(_("Instance is running"), instance=instance)

        def _wait_for_boot():
            """Called at an interval until the VM is running."""
            state = self.get_info(instance)['state']

            if state == power_state.RUNNING:
                LOG.info(_("Instance spawned successfully."),
                         instance=instance)
                raise utils.LoopingCallDone()

        timer = utils.FixedIntervalLoopingCall(_wait_for_boot)
        timer.start(interval=0.5).wait()

    # overwrite original libvirt_driver's _destroy method
    def _destroy(self, instance):
        super(CloudletDriver, self)._destroy(instance)

        # get meta info related to VM synthesis
        instance_uuid = str(instance.get('uuid', ''))
        overlay_meta_url, overlay_data_url = self._get_VM_overlay_meta(instance)

        if overlay_meta_url != None and overlay_data_url != None:
            # synthesized VM
            synthesized_VM = self.synthesized_vm_dics.get(instance_uuid)
            if synthesized_VM == None:
                msg = "Synthesized VM, but can't find matching uuid of %s" % \
                        (instance_uuid)
                LOG.info(msg)
            else:
                LOG.info(_("Deallocate all resources of synthesized VM"), \
                        instance=instance)
                if hasattr(synthesized_VM, 'machine') == True:
                    # intentionally avoid terminating VM at synthesis code
                    # since OpenStack will do that
                    synthesized_VM.machine = None
                synthesized_VM.terminate()
                del self.synthesized_vm_dics[instance_uuid]

    def resume_basevm(self, instance, xml,
            base_disk, base_memory, base_diskmeta, base_memmeta, base_hashvalue):
        """ resume base vm to create overlay vm
        """
        options = synthesis.Options()
        options.TRIM_SUPPORT = True
        options.FREE_SUPPORT = False
        options.XRAY_SUPPORT = False
        options.DISK_ONLY = False
        vm_overlay = synthesis.VM_Overlay(base_disk, options, 
                base_mem=base_memory, 
                base_diskmeta=base_diskmeta, 
                base_memmeta=base_memmeta,
                base_hashvalue=base_hashvalue,
                nova_xml=xml,
                nova_util=libvirt_utils,
                nova_conn=self._conn)
        virt_dom = vm_overlay.resume_basevm()
        self.vm_overlay_dict[instance['uuid']] = vm_overlay

        synthesis.rettach_nic(virt_dom, vm_overlay.old_xml_str, xml)

    def create_new_using_synthesis(self, context, instance, xml, 
            image_meta, overlay_meta_url, overlay_data_url):
        # download meta file and get hash value
        u = urllib2.urlopen(overlay_meta_url)
        metadata = u.read()
        overlay_meta = msgpack.unpackb(metadata)
        basevm_id = overlay_meta.get(Cloudlet_Const.META_BASE_VM_SHA256, None)

        # check basevm
        basedisk_snap_id = image_meta['id']
        if basedisk_snap_id != basevm_id:
            msg = "requested base vm is not compatible with openstack base disk %s != %s" \
                    % (basedisk_snap_id, basevm_id)
            raise exception.ImageNotFound(msg)

        meta_data = image_meta.get('properties', None)
        if meta_data.get(CloudletAPI.IMAGE_TYPE_BASE_MEM, None) == None:
            msg = "requested base disk does not have enought %s" \
                    % (basedisk_snap_id )
            raise exception.ImageNotFound(msg)
        memory_snap_id = str(meta_data.get(CloudletAPI.IMAGE_TYPE_BASE_MEM))
        diskhash_snap_id = str(meta_data.get(CloudletAPI.IMAGE_TYPE_BASE_DISK_HASH))
        memhash_snap_id = str(meta_data.get(CloudletAPI.IMAGE_TYPE_BASE_MEM_HASH))
        basedisk_path = self._get_cache_image(context, instance, basedisk_snap_id)
        basemem_path = self._get_cache_image(context, instance, memory_snap_id)
        diskhash_path = self._get_cache_image(context, instance, diskhash_snap_id)
        memhash_path = self._get_cache_image(context, instance, memhash_snap_id)

        # download blob
        u = urllib2.urlopen(overlay_data_url)
        comp_overlay_blob = u.read()
        fileutils.ensure_tree(libvirt_utils.get_instance_path(instance))
        decomp_overlay = os.path.join(libvirt_utils.get_instance_path(instance),
                'decomp_overlay')

        # decompress blob
        decomp_overlay_file = open(decomp_overlay, "w+b")
        decompressor = LZMADecompressor()
        decomp_data = decompressor.decompress(comp_overlay_blob)
        decomp_data += decompressor.flush()
        decomp_overlay_file.write(decomp_data)
        decomp_overlay_file.close()

        # recover VM
        launch_disk, launch_mem, fuse, delta_proc, fuse_proc = \
                synthesis.recover_launchVM(basedisk_path, overlay_meta,
                        decomp_overlay,
                        base_mem=basemem_path,
                        base_diskmeta=diskhash_path,
                        base_memmeta=memhash_path)

        # resume VM
        LOG.info(_("Starting VM synthesis"), instance=instance)
        synthesized_vm = synthesis.SynthesizedVM(launch_disk, launch_mem, fuse,
                disk_only=False, qemu_args=False, nova_xml=xml, nova_conn=self._conn)

        # testing non-thread resume
        delta_proc.start()
        fuse_proc.start()
        delta_proc.join()
        fuse_proc.join() 
        LOG.info(_("Finish VM synthesis"), instance=instance)

        synthesized_vm.resume()

        # rettach nic card
        synthesis.rettach_nic(synthesized_vm.machine, 
                synthesized_vm.old_xml_str, xml)

        return synthesized_vm


