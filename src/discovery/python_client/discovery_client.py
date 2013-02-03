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

import sys
import time
from optparse import OptionParser

import urllib
import httplib
import json
import socket
from urlparse import urlparse


class CloudletDiscoveryClient(object):
    API_URL             =   "/api/v1/Cloudlet/search/"

    def __init__(self, server_dns, log=None):
        self.server_dns = server_dns
        if self.server_dns.find("http://") != 0:
            self.server_dns = "http://" + self.server_dns

        if log:
            self.log = log
        else:
            self.log = open("/dev/null", "w+b")

    def search(self, n_max=5):
        # get cloudlet list matching server_dns
        end_point = urlparse("%s%s?n=%d" % \
                (self.server_dns, CloudletDiscoveryClient.API_URL, n_max))
        self.cloudlet_list = http_get(end_point)
        return self.cloudlet_list

    def terminate(self):
        pass


def http_get(end_point):
    sys.stdout.write("Connecting to %s\n" % (''.join(end_point)))
    params = urllib.urlencode({})
    headers = {"Content-type":"application/json"}
    end_string = "%s?%s" % (end_point[2], end_point[4])

    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", end_string, params, headers)
    data = conn.getresponse().read()
    import pdb; pdb.set_trace()
    response_list = json.loads(data).get('cloudlet', list())
    conn.close()
    return response_list


def process_command_line(argv):
    USAGE = 'Usage: %prog -s server_domain'
    DESCRIPTION = 'Cloudlet register thread'

    parser = OptionParser(usage=USAGE, description=DESCRIPTION)

    parser.add_option(
            '-s', '--server', action='store', dest='server_dns',
            help='IP address of directory server')
    settings, args = parser.parse_args(argv)
    if not settings.server_dns:
        parser.error("need server dns")
    return settings, args


def main(argv):
    settings, args = process_command_line(sys.argv[1:])
    client = CloudletDiscoveryClient(settings.server_dns, log=sys.stdout)
    ret_list = client.search()
    print ret_list
    return 0


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
