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

from nova.compute import task_states
from nova.compute import vm_states
from nova.openstack.common import log as logging
from nova import utils
from nova.openstack.common import lockutils
from nova import exception
from nova.compute import manager as compute_manager
from nova.openstack.common.notifier import api as notifier
from nova.virt import driver


LOG = logging.getLogger(__name__)


class CloudletComputeManager(compute_manager.ComputeManager):
    """Manages the running instances from creation to destruction."""
    RPC_API_VERSION = '2.28'

    def __init__(self, compute_driver=None, *args, **kwargs):
        super(CloudletComputeManager, self).__init__(*args, **kwargs)

        # make sure to load cloudlet Driver which inherit libvirt driver
        # change at /etc/nova/nova-compute.conf
        self.driver = driver.load_compute_driver(self.virtapi, compute_driver)

    @compute_manager.exception.wrap_exception(notifier=notifier, \
            publisher_id=compute_manager.publisher_id())
    @compute_manager.reverts_task_state
    @compute_manager.wrap_instance_fault
    def cloudlet_create_base(self, context, instance, vm_name, 
            disk_meta_id, memory_meta_id, 
            diskhash_meta_id, memoryhash_meta_id):
        """Cloudlet base creation
        and terminate the instance
        """
        context = context.elevated()
        current_power_state = self._get_power_state(context, instance)
        LOG.info(_("Generating cloudlet base"), instance=instance)

        self._notify_about_instance_usage(context, instance, "snapshot.start")

        def update_task_state(task_state, expected_state=task_states.IMAGE_SNAPSHOT):
            return self._instance_update(context, instance['uuid'],
                    task_state=task_state,
                    expected_task_state=expected_state)

        self.driver.cloudlet_base(context, instance, vm_name, 
                disk_meta_id, memory_meta_id, 
                diskhash_meta_id, memoryhash_meta_id, update_task_state)

        instance = self._instance_update(context, instance['uuid'],
                task_state=None,
                expected_task_state=task_states.IMAGE_UPLOADING)

        # notify will raise exception since instance is already deleted
        self._notify_about_instance_usage( context, instance, "snapshot.end")
        self.cloudlet_terminate_instance(context, instance)

    @compute_manager.exception.wrap_exception(notifier=notifier, \
            publisher_id=compute_manager.publisher_id())
    @compute_manager.reverts_task_state
    @compute_manager.wrap_instance_fault
    def cloudlet_overlay_finish(self, context, instance, overlay_name,
            overlay_meta_id, overlay_blob_id):
        """Generate VM overlay with given instance,
        and save it as a snapshot
        """
        context = context.elevated()
        LOG.info(_("Generating VM overlay"), instance=instance)

        def update_task_state(task_state, expected_state=task_states.IMAGE_SNAPSHOT):
            return self._instance_update(context, instance['uuid'],
                    task_state=task_state,
                    expected_task_state=expected_state)

        self.driver.create_overlay_vm(context, instance, overlay_name, 
                overlay_meta_id, overlay_blob_id, update_task_state)

        instance = self._instance_update(context, instance['uuid'],
                task_state=None,
                expected_task_state=task_states.IMAGE_UPLOADING)
        self.cloudlet_terminate_instance(context, instance)

    # almost identical to terminate_instance method
    def cloudlet_terminate_instance(self, context, instance):
        bdms = self._get_instance_volume_bdms(context, instance)

        @lockutils.synchronized(instance['uuid'], 'nova-')
        def do_terminate_instance(instance, bdms):
            try:
                self._delete_instance(context, instance, bdms,
                                      reservations=None)
            except exception.InstanceTerminationFailure as error:
                msg = _('%s. Setting instance vm_state to ERROR')
                LOG.error(msg % error, instance=instance)
                self._set_instance_error_state(context, instance['uuid'])
            except exception.InstanceNotFound as e:
                LOG.warn(e, instance=instance)

        do_terminate_instance(instance, bdms)
