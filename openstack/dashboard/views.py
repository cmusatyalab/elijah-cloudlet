"""
Views for managing Images and Snapshots.
"""

import logging

from django.utils.translation import ugettext_lazy as _

import glanceclient.exc as glance_exceptions
from openstack_dashboard.api import glance

from horizon import exceptions
from horizon import tables
from horizon import tabs

from openstack_dashboard import api
from openstack_dashboard.api.base import is_service_enabled
from django.utils.datastructures import SortedDict
from .images.tables import BaseVMsTable 
from .snapshots.tables import SnapshotsTable
from .instances.tables import InstancesTable
from .volume_snapshots.tables import VolumeSnapshotsTable
from .volume_snapshots.tabs import SnapshotDetailTabs

from horizon import workflows
from .workflows import LaunchInstance
from .workflows import CLOUDLET_TYPE

LOG = logging.getLogger(__name__)


class IndexView(tables.MultiTableView):

    table_classes = (BaseVMsTable, SnapshotsTable, InstancesTable, VolumeSnapshotsTable)
    template_name = 'project/cloudlet/index.html'

    def has_more_data(self, table):
        return getattr(self, "_more_%s" % table.name, False)

    def get_images_data(self):
        marker = self.request.GET.get(BaseVMsTable._meta.pagination_param, None)
        try:
            # FIXME(gabriel): The paging is going to be strange here due to
            # our filtering after the fact.
            (all_images,
             self._more_images) = api.glance.image_list_detailed(self.request,
                                                                 marker=marker)
            images = [im for im in all_images
                      if im.properties.get("cloudlet_type", None) == CLOUDLET_TYPE.IMAGE_TYPE_BASE_DISK]
        except:
            images = []
            exceptions.handle(self.request, _("Unable to retrieve images."))
        return images

    def get_overlays_data(self):
        req = self.request
        marker = req.GET.get(SnapshotsTable._meta.pagination_param, None)
        try:
            all_snaps, self._more_snapshots = api.glance.snapshot_list_detailed(
                req, marker=marker)
            snaps = [im for im in all_snaps
                      if im.properties.get("cloudlet_type", None) == CLOUDLET_TYPE.IMAGE_TYPE_OVERLAY_META]
        except:
            snaps = []
            exceptions.handle(req, _("Unable to retrieve snapshots."))
        return snaps

    def get_instances_data(self):
        def _cloudlet_type(instance):
            request = instance.request
            image_id = instance.image['id']
            metadata = instance.metadata
            try:
                image = glance.image_get(request, image_id)
                if hasattr(image, 'properties') != True:
                    return None
                properties = getattr(image, 'properties')
                if properties == None or properties.get('is_cloudlet') == None:
                    return None

                # now it's either resumed base instance or synthesized instance
                # synthesized instance has meta that for overlay URL
                if metadata.get('overlay_meta_url') != None:
                    return CLOUDLET_TYPE.IMAGE_TYPE_OVERLAY_META
                else:
                    return CLOUDLET_TYPE.IMAGE_TYPE_BASE_DISK
            except glance_exceptions.ClientException:
                return None

        # Gather synthesized instances
        try:
            instances = api.nova.server_list(self.request)
        except:
            instances = []
            exceptions.handle(self.request,
                              _('Unable to retrieve instances.'))

        # Gather our flavors and correlate our instances to them
        filtered_instances = list()
        if instances:
            try:
                flavors = api.nova.flavor_list(self.request)
            except:
                flavors = []
                exceptions.handle(self.request, ignore=True)

            full_flavors = SortedDict([(str(flavor.id), flavor)
                                        for flavor in flavors])
            # Loop through instances to get flavor info.
            for instance in instances:
                try:
                    flavor_id = instance.flavor["id"]
                    if flavor_id in full_flavors:
                        instance.full_flavor = full_flavors[flavor_id]
                    else:
                        # If the flavor_id is not in full_flavors list,
                        # get it via nova api.
                        instance.full_flavor = api.nova.flavor_get(
                            self.request, flavor_id)
                except:
                    msg = _('Unable to retrieve instance size information.')
                    exceptions.handle(self.request, msg)

            for instance in instances:
                instance_type = _cloudlet_type(instance)
                if instance_type == CLOUDLET_TYPE.IMAGE_TYPE_BASE_DISK:
                    filtered_instances.append(instance)
                    setattr(instance, 'cloudlet_type', "Resumed Base VM")
                if instance_type == CLOUDLET_TYPE.IMAGE_TYPE_OVERLAY_META:
                    filtered_instances.append(instance)
                    setattr(instance, 'cloudlet_type', "Synthesized VM")

        return filtered_instances


    def get_volume_snapshots_data(self):
        if is_service_enabled(self.request, 'volume'):
            try:
                snapshots = api.cinder.volume_snapshot_list(self.request)
            except:
                snapshots = []
                exceptions.handle(self.request, _("Unable to retrieve "
                                                  "volume snapshots."))
        else:
            snapshots = []
        return snapshots


class LaunchInstanceView(workflows.WorkflowView):
    workflow_class = LaunchInstance
    template_name = "project/cloudlet/instances/launch.html"

    def get_initial(self):
        initial = super(LaunchInstanceView, self).get_initial()
        initial['project_id'] = self.request.user.tenant_id
        initial['user_id'] = self.request.user.id
        return initial


class DetailView(tabs.TabView):
    tab_group_class = SnapshotDetailTabs
    template_name = 'project/cloudlet/snapshots/detail.html'
