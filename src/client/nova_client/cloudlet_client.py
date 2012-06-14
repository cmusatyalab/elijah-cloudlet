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


def get_list(server_address, token, end_point, request_list):
    if not request_list in ('images', 'flavors', 'extensions', 'servers'):
        sys.stderr.write("Error, Cannot support listing for %s\n" % request_list)
        sys.exit(1)

    url = end_point[1]
    params = urllib.urlencode({})
    headers = { "X-Auth-Token":token, "Content-type":"application/json" }
    end_string = "%s/%s" % (end_point[2], request_list)

    # HTTP response
    conn = httplib.HTTPConnection(url)
    conn.request("GET", end_string, params, headers)
    response = conn.getresponse()
    data = response.read()
    dd = json.loads(data)
    print json.dumps(dd, indent=2)
    conn.close()


def get_token(server_address, user, password, tenant_name):
    url = "%s:5000" % server_address
    params = {"auth":{"passwordCredentials":{"username":user, "password":password}, "tenantName":tenant_name}}
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
    for service in service_list:
        if service['name'] == 'nova':
            nova_endpoint = service['endpoints'][0]['publicURL']
    return api_token, nova_endpoint


def process_command_line(argv):
    global operation_mode

    parser = OptionParser(usage="usage: %prog" + " [option]",
            version="Cloudlet Synthesys(piping) 0.1")
    parser.add_option(
            '-s', '--server', action='store', type='string', dest='server_address', default='localhost',
            help='set openstack api server address')
    parser.add_option(
            '-u', '--user', action='store', type='string', dest='user_name', default='admin',
            help='set username')
    parser.add_option(
            '-p', '--password', action='store', type='string', dest='password', default='admin',
            help='set password')
    parser.add_option(
            '-t', '--tenant', action='store', type='string', dest='tenant_name', default='admin',
            help='set tenant name')
    parser.add_option(
            '-x', '--token', action='store', type='string', dest='token',
            help='set tenant name')
    settings, args = parser.parse_args(argv)
    if not len(args) == 0:
        parser.error('program takes no command-line arguments; "%s" ignored.' % (args,))
    return settings, args


def main(argv=None):
    global LOCAL_IPADDRESS
    settings, args = process_command_line(sys.argv[1:])
    print "Connecting to %s for tenant %s" % (settings.server_address, settings.tenant_name)
    token, endpoint = get_token(settings.server_address, settings.user_name, settings.password, settings.tenant_name)
    print "Your token = %s" % token
    get_list(settings.server_address, token, urlparse(endpoint), "images")
    get_list(settings.server_address, token, urlparse(endpoint), "flavors")
    get_list(settings.server_address, token, urlparse(endpoint), "extensions")
    get_list(settings.server_address, token, urlparse(endpoint), "servers")
    get_list(settings.server_address, token, urlparse(endpoint), "cloudlets")


if __name__ == "__main__":
    status = main()
    sys.exit(status)
