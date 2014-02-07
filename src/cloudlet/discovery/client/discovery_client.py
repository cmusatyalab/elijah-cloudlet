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
import socket
import pprint
from optparse import OptionParser

import urllib
import httplib
import json
import ast
import threading
from urlparse import urlparse

class CloudletDiscoveryClientError(Exception):
    pass

class CloudletQueryingThread(threading.Thread):
    def __init__(self, cloudlet_info):
        self.cloudlet_info = cloudlet_info
        self.url = "http://%s:%d%s" % (
                cloudlet_info['ip_address'],
                cloudlet_info['rest_api_port'],
                cloudlet_info['rest_api_url'])
        threading.Thread.__init__(self, target=self.get_info)

    def get_info(self):
        end_point = urlparse(self.url)
        params = urllib.urlencode({})
        end_string = "%s" % (end_point[2])
        conn = httplib.HTTPConnection(end_point.hostname, end_point.port, timeout=10)
        conn.request("GET", end_string, params)
        print "Connecting to cloudlet at %s" % self.url
        data = conn.getresponse().read()
        json_data = json.loads(data)
        recv_info = ast.literal_eval(json_data)
        self.cloudlet_info.update(recv_info)


class CloudletDiscoveryClient(object):
    API_URL             =   "/api/v1/Cloudlet/search/"

    def __init__(self, register_server, log=None):
        self.register_server = register_server
        if self.register_server.find("http://") != 0:
            self.register_server = "http://" + self.register_server
        if self.register_server.endswith("/") != 0:
            self.register_server =self.register_server[:-1]
        if log:
            self.log = log
        else:
            self.log = open("/dev/null", "w+b")

    def find_by_ip(self, app_id, client_ip):
        cloudlet_list = self._search_by_proximity(client_ip=client_ip)
        self._get_cloudlet_infos(cloudlet_list, app_id)
        cloudlet = self._find_best_cloudlet(cloudlet_list)
        return cloudlet

    def find_by_gps(self, app_id, latitude, longitude):
        cloudlet_list = self._search_by_proximity(latitude=latitude, 
                longitude=longitude)
        self._get_cloudlet_infos(cloudlet_list, app_id)
        cloudlet = self._find_best_cloudlet(cloudlet_list)
        return cloudlet

    def _search_by_proximity(self, client_ip=None, \
            latitude=None, longitude=None, n_max=5):
        # get cloudlet list
        if latitude is not None and longitude is not None:
            end_point = urlparse("%s%s?n=%d&latitude=%s&longitude=%s" % \
                    (self.register_server, CloudletDiscoveryClient.API_URL, \
                    n_max, latitude, longitude))
        elif client_ip is not None:
            # search by IP address
            end_point = urlparse("%s%s?n=%d&client_ip=%s" % \
                    (self.register_server, CloudletDiscoveryClient.API_URL, \
                    n_max, str(client_ip)))
        else:
            end_point = urlparse("%s%s?n=%d" % \
                    (self.register_server, CloudletDiscoveryClient.API_URL, \
                    n_max))
        try:
            self.cloudlet_list = http_get(end_point)
        except socket.error as e:
            CloudletDiscoveryClient("Cannot connect to ")
        return self.cloudlet_list

    def _get_cloudlet_infos(self, cloudlet_list, app_id):
        thread_list = list()
        for cloudlet in cloudlet_list:
            new_thread = CloudletQueryingThread(cloudlet)
            thread_list.append(new_thread)
        for th in thread_list:
            th.start()
        for th in thread_list:
            th.join()


    def _find_best_cloudlet(self, cloudlet_list):
        if len(cloudlet_list) == 0:
            raise CloudletDiscoveryClientError("No available cloudlet at the list")
        return cloudlet_list[0]

    def terminate(self):
        pass


def http_get(end_point):
    sys.stdout.write("Connecting to %s\n" % (end_point.geturl()))
    params = urllib.urlencode({})
    headers = {"Content-type":"application/json"}
    end_string = "%s?%s" % (end_point[2], end_point[4])

    conn = httplib.HTTPConnection(end_point[1])
    conn.request("GET", end_string, params, headers)
    data = conn.getresponse().read()
    response_list = json.loads(data).get('cloudlet', list())
    conn.close()
    return response_list


def process_command_line(argv):
    USAGE = 'Usage: %prog [-d dns_server|-s register_server]'
    DESCRIPTION = 'Cloudlet register thread'

    parser = OptionParser(usage=USAGE, description=DESCRIPTION)

    parser.add_option(
            '-d', '--dns_server', action='store', dest='dns_server',
            default=None, help='IP address of DNS server')
    parser.add_option(
            '-s', '--register_server', action='store', dest='register_server',
            default=None, help='IP address of cloudlet register server')
    parser.add_option(
            '-a', '--latitude', action='store', type='string', dest='latitude', \
            default=None, help="Manually set cloudlet's latitude")
    parser.add_option(
            '-o', '--longitude', action='store', type='string', dest='longitude',
            default=None, help="Manually set cloudlet's longitude")
    settings, args = parser.parse_args(argv)
    if settings.dns_server == None and settings.register_server == None:
        parser.error("need either dns or register server")
    if settings.dns_server is not None and settings.register_server is not None:
        parser.error("need either dns or register server")
    return settings, args


def main(argv):
    settings, args = process_command_line(sys.argv[1:])
    if settings.register_server is not None:
        client = CloudletDiscoveryClient(settings.register_server, log=sys.stdout)
        #cloudlet = client.find_by_gps("test_app",
        #        latitude=settings.latitude, longitude=settings.longitude)
        cloudlet = client.find_by_ip("test_app", client_ip="128.2.210.197")
        pprint.pprint(cloudlet)
    return 0


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
