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

from nova.compute import api as nova_api
from nova.compute import rpcapi as nova_rpc
from nova.compute import vm_states
from nova import exception
from nova.compute import task_states
from nova.openstack.common import log as logging
from oslo.config import cfg

import nova.openstack.common.rpc.proxy
from nova.openstack.common import jsonutils

LOG = logging.getLogger(__name__)
CONF = cfg.CONF
CONF.import_opt('reclaim_instance_interval', 'nova.compute.cloudlet_manager')


class CloudletAPI(nova.openstack.common.rpc.proxy.RpcProxy):
    """ At the time we implement Cloudlet API in grizzly,
    API has 2.27 BASE_RPC_API_VERSION. 
    """
    BASE_RPC_API_VERSION = '2.28'

    IMAGE_TYPE_BASE_DISK        = "cloudlet_base_disk"
    IMAGE_TYPE_BASE_MEM         = "cloudlet_base_memory"
    IMAGE_TYPE_BASE_DISK_HASH   = "cloudlet_base_disk_hash"
    IMAGE_TYPE_BASE_MEM_HASH    = "cloudlet_base_memory_hash"
    IMAGE_TYPE_OVERLAY_META     = "cloudlet_overlay_meta"
    IMAGE_TYPE_OVERLAY_DATA     = "cloudlet_overlay_data"

    INSTANCE_TYPE_RESUMED_BASE      = "cloudlet_resumed_base_instance"
    INSTANCE_TYPE_SYNTHESIZED_VM    = "cloudlet_synthesized_vm"

    def __init__(self):
        super(CloudletAPI, self).__init__(
                topic=CONF.compute_topic,
                default_version=CloudletAPI.BASE_RPC_API_VERSION)
        self.nova_api = nova_api.API()
        
    def _cloudlet_create_image(self, context, instance, name, image_type,
                      backup_type=None, rotation=None, extra_properties=None):
        """Create new image entry in the image service.  This new image
        will be reserved for the compute manager to upload a snapshot
        or backup.

        :param context: security context
        :param instance: nova.db.sqlalchemy.models.Instance
        :param name: string for name of the snapshot
        :param image_type: snapshot | backup
        :param backup_type: daily | weekly
        :param rotation: int representing how many backups to keep around;
            None if rotation shouldn't be used (as in the case of snapshots)
        :param extra_properties: dict of extra image properties to include

        """
        instance_uuid = instance['uuid']

        properties = {
            'instance_uuid': instance_uuid,
            'user_id': str(context.user_id),
            'image_type': image_type,
        }
        sent_meta = {
            'name': name,
            'is_public': True,
            'properties': properties,
        }

        # Persist base image ref as a Glance image property
        system_meta = self.nova_api.db.instance_system_metadata_get(
                context, instance_uuid)
        base_image_ref = system_meta.get('image_base_image_ref')
        if base_image_ref:
            properties['base_image_ref'] = base_image_ref

        if image_type == 'backup':
            properties['backup_type'] = backup_type

        elif image_type == 'snapshot':
            min_ram, min_disk = self.nova_api._get_minram_mindisk_params(context,
                                                                instance)
            if min_ram is not None:
                sent_meta['min_ram'] = min_ram
            if min_disk is not None:
                sent_meta['min_disk'] = min_disk

        properties.update(extra_properties or {})

        # Now inherit image properties from the base image
        prefix = 'image_'
        for key, value in system_meta.items():
            # Trim off the image_ prefix
            if key.startswith(prefix):
                key = key[len(prefix):]

            # Skip properties that are non-inheritable
            if key in CONF.non_inheritable_image_properties:
                continue

            # By using setdefault, we ensure that the properties set
            # up above will not be overwritten by inherited values
            properties.setdefault(key, value)

        return self.nova_api.image_service.create(context, sent_meta)

    @nova_api.wrap_check_policy
    @nova_api.check_instance_state(vm_state=[vm_states.ACTIVE])
    def cloudlet_create_base(self, context, instance, base_name, extra_properties=None):
        """Getting disk and memory snapshot of the instance"""
        # add network info
        vifs = self.nova_api.network_api.get_vifs_by_instance(context, instance)
        net_info = []
        for vif in vifs:
            vif_info ={'id':vif['uuid'], 'mac_address':vif['address']}
            net_info.append(vif_info)

        disk_properties = {
                'is_cloudlet': True, 
                'cloudlet_type' : CloudletAPI.IMAGE_TYPE_BASE_DISK,
                'network' : net_info, 
                }
        mem_properties = {
                'is_cloudlet': True,
                'cloudlet_type' : CloudletAPI.IMAGE_TYPE_BASE_MEM,
                'network' : net_info, 
                }
        diskhash_properties = {
                'is_cloudlet': True, 
                'cloudlet_type' : CloudletAPI.IMAGE_TYPE_BASE_DISK_HASH,
                'network' : net_info, 
                }
        memhash_properties = {
                'is_cloudlet': True,
                'cloudlet_type' : CloudletAPI.IMAGE_TYPE_BASE_MEM_HASH,
                'network' : net_info, 
                }
        disk_properties.update(extra_properties or {})
        mem_properties.update(extra_properties or {})
        diskhash_properties.update(extra_properties or {})
        memhash_properties.update(extra_properties or {})

        disk_name = base_name+'-disk'
        diskhash_name = base_name+'-disk-meta'
        mem_name = base_name+'-mem'
        memhash_name = base_name+'-mem-meta'
        snapshot = 'snapshot'

        recv_mem_meta = self._cloudlet_create_image(context, instance, mem_name, \
                snapshot, extra_properties = mem_properties)
        recv_diskhash_meta = self._cloudlet_create_image(context, instance, diskhash_name, \
                snapshot, extra_properties = diskhash_properties)
        recv_memhash_meta = self._cloudlet_create_image(context, instance, memhash_name, \
                snapshot, extra_properties = memhash_properties)

        # add reference for the other base vm information to get it later
        disk_properties.update({
            CloudletAPI.IMAGE_TYPE_BASE_MEM: recv_mem_meta['id'],
            CloudletAPI.IMAGE_TYPE_BASE_DISK_HASH: recv_diskhash_meta['id'],
            CloudletAPI.IMAGE_TYPE_BASE_MEM_HASH: recv_memhash_meta['id'],
            })
        recv_disk_meta = self._cloudlet_create_image(context, instance, disk_name, \
                snapshot, extra_properties = disk_properties)

        instance = self.nova_api.update(context, instance,
                               task_state=task_states.IMAGE_SNAPSHOT,
                               expected_task_state=None)

        # api request
        instance_p = jsonutils.to_primitive(instance)
        self.cast(context, 
                self.make_msg(
                        'cloudlet_create_base',
                        instance=instance_p,
                        vm_name=base_name,
                        disk_meta_id=recv_disk_meta['id'], 
                        memory_meta_id=recv_mem_meta['id'],
                        diskhash_meta_id=recv_diskhash_meta['id'], 
                        memoryhash_meta_id=recv_memhash_meta['id']
                        ),
                topic=nova_rpc._compute_topic(self.topic, context, None, instance),
                version=CloudletAPI.BASE_RPC_API_VERSION)
        return recv_disk_meta, recv_mem_meta

    @nova_api.wrap_check_policy
    def cloudlet_create_overlay_start(self, context, instance, basevm_name, extra_properties=None):
        instance_p = jsonutils.to_primitive(instance)

        self.nova_api._cast_compute_message('cloudlet_overlay_start', context, instance)
        self.cast(context, 
                self.make_msg(
                        'cloudlet_overlay_start',
                        instance=instance_p,
                        basevm_name=basevm_name,
                        ),
                topic=nova_rpc._compute_topic(self.topic, context, None, instance),
                version=CloudletAPI.BASE_RPC_API_VERSION)

    @nova_api.check_instance_state(vm_state=[vm_states.ACTIVE])
    def cloudlet_create_overlay_finish(self, context, instance, overlay_name, extra_properties=None):
        meta_properties = {
                'is_cloudlet': True, 
                'cloudlet_type' : CloudletAPI.IMAGE_TYPE_OVERLAY_META,
                }
        blob_properties = {
                'is_cloudlet': True,
                'cloudlet_type' : CloudletAPI.IMAGE_TYPE_OVERLAY_DATA,
                }
        meta_properties.update(extra_properties or {})
        blob_properties.update(extra_properties or {})
        recv_overlay_meta = self._cloudlet_create_image(context, instance, overlay_name+'-meta', \
                'snapshot', extra_properties = meta_properties)
        recv_overlay_blob = self._cloudlet_create_image(context, instance, overlay_name+'-blob', \
                'snapshot', extra_properties = blob_properties)

        instance = self.nova_api.update(context, instance,
                               task_state=task_states.IMAGE_SNAPSHOT,
                               expected_task_state=None)

        # api request
        instance_p = jsonutils.to_primitive(instance)
        self.cast(context, 
                self.make_msg(
                        'cloudlet_overlay_finish',
                        instance=instance_p,
                        overlay_name=overlay_name,
                        overlay_meta_id=recv_overlay_meta['id'], 
                        overlay_blob_id=recv_overlay_blob['id']
                        ),
                topic=nova_rpc._compute_topic(self.topic, context, None, instance),
                version=CloudletAPI.BASE_RPC_API_VERSION)

        return recv_overlay_meta, recv_overlay_blob

