#!/usr/bin/env python
#
# Elijah: Cloudlet Infrastructure for Mobile Computing
# Copyright (C) 2011-2012 Carnegie Mellon University
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

from optparse import OptionParser
from urlparse import urlparse
import httplib
import json
import sys
import urllib

image_name = "Ubuntu-server"
test_instance_name = image_name + '-test'
base_vm_name = image_name + '-base'
overlay_vm_name = image_name + "-overlay_vm"


class CloudletClientError(Exception):
    pass


def get_list(server_address, token, end_point, request_list):
    if not request_list in ('images', 'flavors', 'extensions', 'servers'):
        sys.stderr.write("Error, Cannot support listing for %s\n" % request_list)
        sys.exit(1)

    params = urllib.urlencode({})
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }
    if request_list == 'extensions':
        end_string = "%s/%s" % (end_point[2], request_list)
    else:
        end_string = "%s/%s/detail" % (end_point[2], request_list)

    # HTTP response
    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", end_string, params, headers)
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    #print json.dumps(dd, indent=2)
    conn.close()
    return dd[request_list]


def request_new_server(server_address, token, end_point, \
        key_name=None, image_name=None, server_name=None):
    # basic data
    image_ref, image_id = get_ref_id(server_address, token, \
            end_point, "images", image_name)
    flavor_ref, flavor_id = get_ref_id(server_address, token, \
            end_point, "flavors", "m1.tiny")
    # other data
    sMetadata = {}
    s = { \
            "server": { \
                "name": server_name, "imageRef": image_id, \
                "flavorRef": flavor_id, "metadata": sMetadata, \
                "min_count":"1", "max_count":"1",
                "key_name": key_name,
                } }
    params = json.dumps(s)
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }
    #print json.dumps(s, indent=4)

    conn = httplib.HTTPConnection(end_point[1])
    conn.request("POST", "%s/servers" % end_point[2], params, headers)
    print "request new server: %s/servers" % (end_point[2])
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    conn.close()
    print json.dumps(dd, indent=2)


def request_synthesis(server_address, token, end_point, key_name=None, image_name=None, \
        server_name=None, overlay_meta_url=None, overlay_blob_url=None):
    # basic data
    image_ref, image_id = get_ref_id(server_address, token, end_point, "images", image_name)
    flavor_ref, flavor_id = get_ref_id(server_address, token, end_point, "flavors", "m1.tiny")
    # other data
    meta_data = {"overlay_meta_url": overlay_meta_url, \
            "overlay_blob_url":overlay_blob_url}
    s = { \
            "server": { \
                "name": server_name, "imageRef": image_id, \
                "flavorRef": flavor_id, "metadata": meta_data, \
                "min_count":"1", "max_count":"1",
                "key_name": key_name,
                } }
    params = json.dumps(s)
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }
    print json.dumps(s, indent=4)
    conn = httplib.HTTPConnection(end_point[1])
    conn.request("POST", "%s/servers" % end_point[2], params, headers)
    print "request new server: %s/servers" % (end_point[2])
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    conn.close()

    print json.dumps(dd, indent=2)
    return dd

def request_start_stop(server_address, token, end_point, server_name, is_request_start):
    server_list = get_list(server_address, token, end_point, "servers")
    server_id = ''
    for server in server_list:
        if server['name'] == server_name:
            server_id = server['id']
            print "server id : " + server_id
    if not server_id:
        return False, "no such VM named : %s" % server_name

    if is_request_start:
        params = json.dumps({"os-start":"null"})
    else:
        params = json.dumps({"os-stop":"null"})
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }

    conn = httplib.HTTPConnection(end_point[1])
    command = "%s/servers/%s/action" % (end_point[2], server_id)
    print command
    conn.request("POST", command, params, headers)
    response = conn.getresponse()
    data = response.read()
    conn.close()
    print data


def request_cloudlet_base(server_address, token, end_point, server_name, cloudlet_base_name):
    server_list = get_list(server_address, token, end_point, "servers")
    server_id = ''
    for server in server_list:
        if server['name'] == server_name:
            server_id = server['id']
            print "server id : " + server_id
    if not server_id:
        raise CloudletClientError("cannot find matching instance with %s", server_name)

    params = json.dumps({"cloudlet-base":{"name": cloudlet_base_name}})
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }

    conn = httplib.HTTPConnection(end_point[1])
    command = "%s/servers/%s/action" % (end_point[2], server_id)
    conn.request("POST", command, params, headers)
    response = conn.getresponse()
    data = response.read()
    conn.close()
    print json.dumps(data, indent=2)
    return data


def request_cloudlet_overlay_start(server_address, token, end_point, image_name, key_name):
    #get right iamge
    image_list = get_list(server_address, token, end_point, "images")
    image_id = ''
    meta = {}
    for image in image_list:
        print "%s == %s" % (image.get('name'), image_name)
        if image.get('name') == image_name:
            metadata = image.get('metadata')
            if metadata and metadata.get('memory_snapshot_id'):
                image_id = image.get('id')
                meta['image_snapshot_id']=metadata.get('memory_snapshot_id')

    if not image_id:
        raise CloudletClientError("cannot find matching image")

    flavor_ref, flavor_id = get_ref_id(server_address, token, end_point, "flavors", "m1.tiny")
    s = { \
            "server": { \
                "name": image_name+"-overlay", "imageRef": image_id, \
                "flavorRef": flavor_id, \
                "metadata": meta, \
                "min_count":"1", "max_count":"1",\
                "key_name": key_name \
                } }
    params = json.dumps(s)
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }
    print json.dumps(s, indent=4)

    conn = httplib.HTTPConnection(end_point[1])
    conn.request("POST", "%s/servers" % end_point[2], params, headers)
    print "request new server: %s/servers" % (end_point[2])
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    conn.close()
    print json.dumps(dd, indent=2)


def request_cloudlet_overlay_stop(server_address, token, end_point, \
        server_name, overlay_name):
    server_list = get_list(server_address, token, end_point, "servers")
    server_id = ''
    for server in server_list:
        print "%s == %s" % (server['name'], server_name)
        if server['name'] == server_name:
            server_id = server['id']
            print "server id : " + server_id
    if not server_id:
        raise CloudletClientError("cannot find matching server name")

    params = json.dumps({"cloudlet-overlay-finish":{"overlay-name": overlay_name}})
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }

    conn = httplib.HTTPConnection(end_point[1])
    command = "%s/servers/%s/action" % (end_point[2], server_id)
    conn.request("POST", command, params, headers)
    response = conn.getresponse()
    data = response.read()
    conn.close()
    print data
   

def request_cloudlet_ipaddress(server_address, token, end_point, server_uuid):
    params = urllib.urlencode({})
    # HTTP response
    conn = httplib.HTTPConnection(end_point[1])
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }
    end_string = "%s/servers/%s" % (end_point[2], server_uuid)
    conn.request("GET", end_string, params, headers)
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    conn.close()

    floating_ip = None
    if dd.get('server', None) == None:
        raise CloudletClientError("cannot find matching server : %s" % \
                server_uuid)
    if dd['server'].get('addresses', None) != None:
        ipinfo_list = dd['server']['addresses'].get('private', None)
        if ipinfo_list != None:
            for each_ipinfo in ipinfo_list:
                if each_ipinfo.has_key("OS-EXT-IPS:type"):
                    floating_ip = str(each_ipinfo['addr'])

    status = dd['server']['status']
    return status, floating_ip


def get_token(server_address, user, password, tenant_name):
    url = "%s:5000" % server_address
    params = {
            "auth":
                {"passwordCredentials":{"username":user, "password":password},
            "tenantName":tenant_name}
            }
    headers = {"Content-Type": "application/json"}

    # HTTP connection
    conn = httplib.HTTPConnection(url)
    #print json.dumps(params, indent=4)
    #print headers
    conn.request("POST", "/v2.0/tokens", json.dumps(params), headers)

    # HTTP response
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    conn.close()

    #print json.dumps(dd, indent=4)
    api_token = dd['access']['token']['id']
    service_list = dd['access']['serviceCatalog']
    nova_endpoint = None
    glance_endpoint = None
    for service in service_list:
        if service['name'] == 'nova':
            nova_endpoint = service['endpoints'][0]['publicURL']
        elif service['name'] == 'glance':
            glance_endpoint = service['endpoints'][0]['publicURL']
    return api_token, nova_endpoint, glance_endpoint


def overlay_download(server_address, uname, password, overlay_name, output_file):
    """ glance API has been changed so the below code does not work
    """
    #from glance import client
    #glance_client = client.get_client(server_address, username=uname, password=password)
    #image_list = glance_client.get_images()
    #image_id = ''
    #for image in image_list:
    #    print "image list : %s" % image.get('name')
    #    if image.get('name') and image['name'] == overlay_name:
    #        image_id = image.get('id')
    #        break
    #if not image_id:
    #    raise CloudletClientError("cannot find matching glance image")
    #
    #meta, raw = glance_client.get_image(image_id)
    #if not meta or not raw:
    #    raise CloudletClientError("cannot download")

    #fout = open(output_file, "wb")
    #for chunk in raw:
    #    fout.write(chunk)
    #fout.close()
    import subprocess
    fout = open(output_file, "wb")
    _PIPE = subprocess.PIPE
    cmd = "glance image-download %s" % (overlay_name)
    proc = subprocess.Popen(cmd.split(" "), stdout=fout, stderr=_PIPE)
    out, err = proc.communicate()
    
    if err:
        print err

def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog" + " [option]",
            version="Cloudlet Synthesys(piping) 0.1")
    parser.add_option(
            '-s', '--server', action='store', type='string',
            dest='server_address', default='rain.elijah.cs.cmu.edu',
            help='set openstack api server address')
    parser.add_option(
            '-u', '--user', action='store', type='string', 
            dest='user_name', default='admin',
            help='set username')
    parser.add_option(
            '-p', '--password', action='store', type='string', 
            dest='password', default='password',
            help='set password')
    parser.add_option(
            '-t', '--tenant', action='store', type='string', 
            dest='tenant_name', default='admin',
            help='set tenant name')
    parser.add_option(
            '-x', '--token', action='store', type='string', dest='token',
            help='set tenant name')
    settings, args = parser.parse_args(argv)
    return settings, args


def get_extension(server_address, token, end_point, extension_name):
    ext_list = get_list(server_address, token, end_point, "extensions")
    #print json.dumps(ext_list, indent=4)

    if extension_name:
        for ext in ext_list:
            if ext['name'] == extension_name:
                return ext
    else:
        return ext_list


def get_ref_id(server_address, token, end_point, ref_string, name):
    support_ref_string = ("images", "flavors")
    if not ref_string in support_ref_string:
        sys.stderr.write("We support only %s, but requested reference is %s", " ".join(support_ref_string), ref_string)
        sys.exit(1)

    params = urllib.urlencode({})
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }
    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", "%s/%s" % (end_point[2], ref_string), params, headers)
    print "requesting %s/%s" % (end_point[2], ref_string)
    
    # HTTP response
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    conn.close()
    #print json.dumps(dd, indent=2)

    # Server image URL
    n = len(dd[ref_string])
    for i in range(n):
        if dd[ref_string][i]['name'] == name:
            image_ref = dd[ref_string][i]["links"][0]["href"]
            return image_ref, dd[ref_string][i]['id']


def get_cloudlet_base_list(server_address, uname, password):
    from glance import client
    glance_client = client.get_client(server_address, username=uname, password=password)
    image_list = glance_client.get_images()
    for image in image_list:
        print "image list : %s" % image.get('name')


def main(argv=None):
    global test_instance_name
    global base_vm_name
    global overlay_vm_name
    key_name = "mykey"

    settings, args = process_command_line(sys.argv[1:])
    print "Connecting to %s for tenant %s" % \
            (settings.server_address, settings.tenant_name)
    token, endpoint, glance_endpoint = \
            get_token(settings.server_address, settings.user_name, 
                    settings.password, settings.tenant_name)

    if len(args) < 1:
        print "need command"
        sys.exit(1)

    if args[0] == 'image-list':
        images = get_list(settings.server_address, token, \
                urlparse(endpoint), "images")
        print json.dumps(images, indent=2)
    elif args[0] == 'boot':
        image_name = sys.argv[2]
        images = request_new_server(settings.server_address, \
                token, urlparse(endpoint), key_name=key_name, \
                image_name=image_name, server_name=test_instance_name)
    elif args[0] == 'create-base':
        request_cloudlet_base(settings.server_address, token, \
                urlparse(endpoint), test_instance_name, base_vm_name) 
    elif args[0] == 'create-overlay':
        request_cloudlet_overlay_stop(settings.server_address, token, urlparse(endpoint), \
                test_instance_name, overlay_vm_name)
    elif args[0] == 'download':
        overlay_download(settings.server_address, "admin", "admin", \
                overlay_vm_name + "-meta", "./overlay.meta")
        overlay_download(settings.server_address, "admin", "admin", \
                overlay_vm_name + "-blob", "./overlay.blob")
    elif args[0] == 'synthesis':
        overlay_meta_url = "http://scarlet.aura.cs.cmu.edu:8000/overlay.meta"
        overlay_blob_url = "http://scarlet.aura.cs.cmu.edu:8000/overlay.blob"
        request_synthesis(settings.server_address, token, urlparse(endpoint), \
                key_name=key_name, image_name=base_vm_name+"-disk", server_name='synthesis', \
                overlay_meta_url=overlay_meta_url, overlay_blob_url=overlay_blob_url)
    elif args[0] == 'ip-address':
        server_uuid = args[1]
        ret = request_cloudlet_ipaddress(settings.server_address, token, urlparse(endpoint), \
                server_uuid=server_uuid)
        print str(ret)
    elif args[0] == 'start':
        instance_name = args[1]
        request_start_stop(settings.server_address, token, urlparse(endpoint), instance_name, is_request_start=True)
    elif args[0] == 'stop':
        instance_name = args[1]
        request_start_stop(settings.server_address, token, urlparse(endpoint), instance_name, is_request_start=False)
    elif args[0] == 'ext-list':
        filter_name = None
        if len(args)==2:
            filter_name = args[1]
        ext_info = get_extension(settings.server_address, token, urlparse(endpoint), filter_name)
        print json.dumps(ext_info, indent=2)
    else:
        print "No such command"
        sys.exit(1)
    '''
    elif args[0] == 'flavor-list':
        images = get_list(settings.server_address, token, urlparse(endpoint), "flavors")
        print json.dumps(images, indent=2)
    elif args[0] == 'server-list':
        images = get_list(settings.server_address, token, urlparse(endpoint), "servers")
        print json.dumps(images, indent=2)
    elif args[0] == 'overlay_start':
        request_cloudlet_overlay_start(settings.server_address, token, urlparse(endpoint), image_name=test_base_name, key_name="test")
    elif args[0] == 'cloudlet_list':
        print get_cloudlet_base_list(settings.server_address, "admin", "admin")
    '''


if __name__ == "__main__":
    status = main()
    sys.exit(status)
