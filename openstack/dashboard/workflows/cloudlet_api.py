import logging

from django.conf import settings
from novaclient.v1_1 import client as nova_client
import httplib
import json
from urlparse import urlparse
from openstack_dashboard.api.base import url_for
from novaclient.v1_1 import security_group_rules as nova_rules
from novaclient.v1_1.security_groups import SecurityGroup as NovaSecurityGroup


LOG = logging.getLogger(__name__)

def request_synthesis(request, vm_name, base_disk_id, flavor_id, key_name, security_group_id, overlay_meta_url, overlay_blob_url):
    token = request.user.token.id
    management_url = url_for(request, 'compute')
    end_point = urlparse(management_url)

    # other data
    meta_data = {"overlay_meta_url": overlay_meta_url, 
            "overlay_blob_url":overlay_blob_url}
    s = { \
            "server": { \
                "name": vm_name, "imageRef": base_disk_id, 
                "flavorRef": flavor_id, "metadata": meta_data, 
                "min_count":"1", "max_count":"1",
                "security_group": security_group_id,
                "key_name": key_name,
                } }
    params = json.dumps(s)
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }

    conn = httplib.HTTPConnection(end_point[1])
    conn.request("POST", "%s/servers" % end_point[2], params, headers)
    print "request new server: %s/servers" % (end_point[2])
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    conn.close()

    print json.dumps(dd, indent=2)
    return dd

class test_class(object):
    pass

def novaclient(request):
    insecure = getattr(settings, 'OPENSTACK_SSL_NO_VERIFY', False)
    LOG.debug('novaclient connection created using token "%s" and url "%s"' %
              (request.user.token.id, url_for(request, 'compute')))
    c = nova_client.Client(request.user.username,
                           request.user.token.id,
                           project_id=request.user.tenant_id,
                           auth_url=url_for(request, 'compute'),
                           insecure=insecure,
                           http_log_debug=settings.DEBUG)
    c.client.auth_token = request.user.token.id
    c.client.management_url = url_for(request, 'compute')
    return c
