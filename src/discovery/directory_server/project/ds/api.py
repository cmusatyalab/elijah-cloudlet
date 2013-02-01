# ds/api.py
from django.utils.timezone import utc
import datetime
import heapq
from operator import itemgetter
from tastypie.authorization import Authorization
from ds.models import Cloudlet
from tastypie.resources import ModelResource, ALL, ALL_WITH_RELATIONS
from django.contrib.auth.models import User
from django.conf.urls.defaults import *
from tastypie.utils import trailing_slash

from django.core.serializers import json
from django.utils import simplejson
from tastypie.serializers import Serializer
from network import ip_location


now = datetime.datetime.utcnow().replace(tzinfo=utc)
cost = ip_location.IPLocation()


class PrettyJSONSerializer(Serializer):
    json_indent = 2
    def to_json(self, data, options=None):
        options = options or {}
        data = self.to_simple(data, options)
        return simplejson.dumps(data, cls=json.DjangoJSONEncoder,
                sort_keys=True, ensure_ascii=False, indent=self.json_indent)


class CloudletResource(ModelResource):
    class Meta:
        serializer = PrettyJSONSerializer()
        authorization = Authorization()
        always_return_data = True
        queryset = Cloudlet.objects.all()
        resource_name = 'Cloudlet'
        list_allowed_methods = ['get', 'post', 'put', 'delete']
        excludes = ['pub_date', 'mod_time', 'id']
        filtering = {"mod_time":ALL, "status":ALL}

        search_result = ['latitude', 'longitude', 'ip_address']

    def obj_create(self, bundle, request=None, **kwargs):
        '''
        called for POST
        '''
        #import pdb;pdb.set_trace()
        return super(CloudletResource, self).obj_create(bundle, request, **kwargs)

    def hydrate(self, bundle):
        '''
        called for POST, UPDATE
        '''
        #import pdb;pdb.set_trace()
        cloudlet_ip = bundle.request.META.get("REMOTE_ADDR")
        if cloudlet_ip == '127.0.0.1':
            import socket
            cloudlet_ip = socket.gethostbyname(socket.gethostname())

        # record Cloudlet's ip address
        bundle.obj.ip_address = cloudlet_ip
        # find location of cloudlet
        location = cost.ip2location(cloudlet_ip)
        bundle.obj.longitude = location.longitude
        bundle.obj.latitude = location.latitude
        # set status as running
        if not bundle.obj.status:
            bundle.obj.status = 'RUN'
        # update latest update time
        self.mod_time = datetime.datetime.now()
        return bundle

    def dehydrate(self, bundle):
        '''
        called for POST, UPDATE, GET
        '''
        #import pdb;pdb.set_trace()
        bundle.data['longitude'] = "%9.6f" % bundle.data['longitude']
        bundle.data['latitude'] = "%9.6f" % bundle.data['latitude']
        return bundle 
    
    def override_urls(self):
        return [url(r"^(?P<resource_name>%s)/search%s$" % (self._meta.resource_name, trailing_slash()),\
                self.wrap_view('get_search'), name="api_get_search"), ] 
        
    def get_search(self, request, **kwargs):
        self.method_check(request, allowed=['get'])
        self.is_authenticated(request)
        self.throttle_check(request)

        SEARCH_COUNT = int(request.GET.get('n', 5))
        cloudlet_ip = request.META.get("REMOTE_ADDR")
        client_location = cost.ip2location(cloudlet_ip)
        lat1, lon1 = client_location.latitude, client_location.longitude
        cloudlet_list = list()
        for cloudlet in self.Meta.queryset:
            if cloudlet.status != Cloudlet.CLOUDLET_STATUS_RUNNING:
                continue

            lat2, lon2 = float(cloudlet.latitude), float(cloudlet.longitude)
            geo_distance = ip_location.geo_distance(lat1, lon1, lat2, lon2)
            cloudlet.cost = geo_distance
            cloudlet_list.append(cloudlet)

        top_cloudlets = heapq.nlargest(SEARCH_COUNT, cloudlet_list, key=itemgetter('cost'))
        top_cloudlet_list = [item.search_out() for item in top_cloudlets]
        object_list = {
            'cloudlet' : top_cloudlet_list
        }
        self.log_throttled_access(request)
        return self.create_response(request, object_list)

        
    '''
    def build_filters(self, filters=None):
        if filters is None:
            filters = {}
            orm_filters = super(CloudletResource, self).build_filters(filters)
            if "q" in filters:
                sqs = SearchQuerySet().auto_query(filters['q'])
                orm_filters["pk__in"] = [i.pk for i in sqs]
            return orm_filters
    '''
