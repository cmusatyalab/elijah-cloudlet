from django.utils.timezone import utc
import datetime
import heapq
import socket 
from operator import itemgetter
from tastypie.authorization import Authorization
from .models import Cloudlet
from tastypie.resources import ModelResource, ALL, ALL_WITH_RELATIONS
from django.conf.urls.defaults import *
from tastypie.utils import trailing_slash

from django.core.serializers import json
from django.utils import simplejson
from tastypie.serializers import Serializer
from ..network import ip_location
from django.db.models.signals import post_save

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
        filtering = {"mod_time":ALL, "status":ALL, "ip_address":ALL,
                "latitude":ALL, "longitude":ALL, "rest_api_port":ALL}

    def obj_create(self, bundle, **kwargs):
        '''
        called for POST
        '''
        return super(CloudletResource, self).obj_create(bundle, **kwargs)

    def hydrate(self, bundle):
        '''
        called for POST, UPDATE
        '''
        cloudlet_ip = bundle.request.META.get("REMOTE_ADDR")
        if cloudlet_ip == "127.0.0.1":
            import socket
            cloudlet_ip = socket.gethostbyname(socket.gethostname())

        # find location of cloudlet
        location = cost.ip2location(cloudlet_ip)
        # in python 2.6, you cannot directly convert float to Decimal
        if bundle.obj.longitude is None:
            bundle.obj.longitude = str(location.longitude).strip()
        if bundle.obj.latitude is None:
            bundle.obj.latitude = str(location.latitude).strip()

        # record Cloudlet's ip address
        bundle.obj.mod_time = datetime.datetime.now()
        return bundle

    def dehydrate(self, bundle):
        '''
        called for POST, UPDATE, GET
        '''
        #bundle.data['longitude'] = "%9.6f" % bundle.data['longitude']
        #bundle.data['latitude'] = "%9.6f" % bundle.data['latitude']
        return bundle

    def prepend_urls(self):
        return [url(r"^(?P<resource_name>%s)/search%s$" %
                (self._meta.resource_name, trailing_slash()),
                self.wrap_view('get_search'), name="api_get_search"), ]

    def get_search(self, request, **kwargs):
        self.method_check(request, allowed=['get'])
        self.is_authenticated(request)
        self.throttle_check(request)

        SEARCH_COUNT = int(request.GET.get('n', 5))
        client_ip = request.GET.get('client_ip', None)
        latitude = request.GET.get('latitude', None)
        longitude = request.GET.get('longitude', None)
        if client_ip is not None and self._is_ip(client_ip) is True:
            client_location = cost.ip2location(str(client_ip))
            latitude = client_location.latitude
            longitude = client_location.longitude
        elif latitude is not None and longitude is not None:
            latitude, longitude = float(latitude), float(longitude)
        else:
            client_ip = request.META.get("REMOTE_ADDR")
            client_location = cost.ip2location(client_ip)
            latitude = client_location.latitude
            longitude = client_location.longitude
        cloudlet_list = list()
        for cloudlet in self.Meta.queryset:
            if cloudlet.status != Cloudlet.CLOUDLET_STATUS_RUNNING:
                continue
            cloudlet_lat = float(cloudlet.latitude)
            cloudlet_long = float(cloudlet.longitude)
            geo_distance = ip_location.geo_distance(latitude, longitude, \
                    cloudlet_lat, cloudlet_long)
            cloudlet.cost = geo_distance
            cloudlet_list.append(cloudlet)

        top_cloudlets = heapq.nsmallest(SEARCH_COUNT, cloudlet_list, key=itemgetter('cost'))
        top_cloudlet_list = [item.search_out() for item in top_cloudlets]
        object_list = {
            'cloudlet' : top_cloudlet_list
        }
        self.log_throttled_access(request)
        return self.create_response(request, object_list)

    def _is_ip(self, ip_address):
        try:
            socket.inet_aton(ip_address)
            return True
        except socket.error:
            return False


def post_save_signal(sender, **kwargs):
    pass
    '''
    cloudlet = kwargs.get('instance', None)
    if (not cloudlet) or (not redis):
        return
    redis.set(cloudlet.ip_address, (cloudlet.latitude, cloudlet.longitude))
    '''

post_save.connect(post_save_signal, sender=Cloudlet)

