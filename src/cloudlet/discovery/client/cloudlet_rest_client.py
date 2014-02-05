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
from optparse import OptionParser

import urllib
import httplib
import json
from urlparse import urlparse


class CloudletQueryClient(object):
    REST_API_URL        = "/api/v1/resource/"

    def __init__(self, cloudlet_addr, log=None):
        self.cloudlet_addr = cloudlet_addr
        if self.cloudlet_addr.find("http://") != 0:
            self.cloudlet_addr = "http://" + self.cloudlet_addr

        if log:
            self.log = log
        else:
            self.log = open("/dev/null", "w+b")

    def query(self):
        end_point = urlparse("%s%s" % \
                (self.cloudlet_addr, self.REST_API_URL))
        self.cloudlet_info = http_get(end_point)
        return self.cloudlet_info

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
    response_list = json.loads(data)
    conn.close()
    return response_list


def process_command_line(argv):
    USAGE = 'Usage: %prog -s cloudlet_ip:port'

    parser = OptionParser(usage=USAGE)

    parser.add_option(
            '-s', '--server', action='store', dest='cloudlet_addr',
            help='IP address of directory server')
    settings, args = parser.parse_args(argv)
    if not settings.cloudlet_addr:
        parser.error("need Cloudlet addr")
    return settings, args


def main(argv):
    settings, args = process_command_line(sys.argv[1:])
    client = CloudletQueryClient(settings.cloudlet_addr, log=sys.stdout)
    ret_list = client.query()
    print ret_list
    return 0


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
