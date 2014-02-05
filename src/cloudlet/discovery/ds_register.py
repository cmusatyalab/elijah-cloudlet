#!/usr/bin/env python 
#
# Cloudlet Infrastructure for Mobile Computing
#
#   Author: Kiryong Ha <krha@cmu.edu>
#
#   Copyright (C) 2011-2013 Carnegie Mellon University
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#

import sys
import time
from optparse import OptionParser
import threading

import urllib
import httplib
import json
import socket
from urlparse import urlparse
from cloudlet import log as logging
from cloudlet.discovery.Const import DiscoveryConst as Const


LOG = logging.getLogger(__name__)


class RegisterError(Exception):
    pass


class RegisterThread(threading.Thread):

    def __init__(self, register_server, resource_monitor, update_period=60, 
            cloudlet_ip=None, cloudlet_rest_port=None,
            latitude=None, longitude=None):
        self.register_server = register_server
        if self.register_server.find("http://") != 0:
            self.register_server = "http://" + self.register_server
        if self.register_server.endswith("/") == True:
            self.register_server = self.register_server[:-1]
        self.REGISTER_PERIOD_SEC = update_period
        self.stop = threading.Event()
        self.resource_uri = None
        self.resource_monitor = resource_monitor

        # custom argument
        self.cloudlet_ip = cloudlet_ip
        if self.cloudlet_ip is None:
            self.cloudlet_ip = get_local_ipaddress()
        self.cloudlet_rest_port = cloudlet_rest_port
        if self.cloudlet_rest_port is None:
            self.cloudlet_rest_port = Const.REST_API_PORT
        self.latitude = latitude
        self.longitude = longitude
        threading.Thread.__init__(self, target=self.register)

    def register(self):
        LOG.info("[REGISTER] start register to %s" % (self.register_server))
        while (self.resource_uri == None):
            if self.stop.wait(0.001):
                # finish thread without deregister since it hasn't done register
                return

            # first resource creation until successfully connected
            try:
                self.resource_uri = self._initial_register(self.register_server)
                LOG.info("[REGISTER] success to initial register")
            except (socket.error, ValueError) as e:
                LOG.info("[REGISTER] waiting for directory server ready")
            finally:
                self.stop.wait(self.REGISTER_PERIOD_SEC)

        # regular update
        while(not self.stop.wait(0.001)):
            try:
                self._update_status(self.register_server)
                LOG.info("[REGISTER] updating status")
            except (socket.error, ValueError) as e:
                pass
                LOG.info("[REGISTER] waiting for directory server ready")
            finally:
                self.stop.wait(self.REGISTER_PERIOD_SEC)

        # send termination message
        try:
            self._deregister(self.register_server)
            LOG.info("[REGISTER] Deregister")
        except (socket.error, ValueError) as e:
            LOG.info("[REGISTER] Failed to deregister due to server error")

    def terminate(self):
        self.stop.set()

    def _initial_register(self, register_server):
        resource_meta = {
                }
        # check existing
        end_point = urlparse("%s%s?ip_address=%s" % \
                (register_server, Const.REGISTER_URL,  self.cloudlet_ip))
        response_list = http_get(end_point)

        resource_meta.update(self.resource_monitor.get_static_resource())
        ret_uri = None
        json_string = {
                "status":"RUN",
                Const.KEY_REST_URL: Const.REST_API_URL,
                Const.KEY_CLOUDLET_IP: self.cloudlet_ip,
                Const.KEY_REST_PORT: self.cloudlet_rest_port,
                'meta': resource_meta,
                }
        if self.latitude is not None:
            json_string.update({Const.KEY_LATITUDE: self.latitude})
        if self.longitude is not None:
            json_string.update({Const.KEY_LONGITUDE: self.longitude})

        if response_list is None or len(response_list) == 0:
            # POST
            end_point = urlparse("%s%s" % \
                (register_server, Const.REGISTER_URL))
            ret_msg = http_post(end_point, json_string=json_string)
            ret_uri = ret_msg.get('resource_uri', None)
            LOG.info("POST information: %s" % json_string)
        else:
            # PUT
            ret_uri = response_list[0].get('resource_uri', None)
            end_point = urlparse("%s%s" % (register_server, ret_uri))
            http_put(end_point, json_string=json_string)
            LOG.info("PUT information: %s" % json_string)

        return ret_uri


    def _update_status(self, register_server):
        resource_meta = {}
        resource_meta.update(self.resource_monitor.get_static_resource())
        #resource_meta.update(self.resource_monitor.get_dynamic_resource())
        end_point = urlparse("%s%s" % (register_server, self.resource_uri))
        json_string = {
                "status":"RUN",
                'meta': resource_meta,
                }
        ret_msg = http_put(end_point, json_string=json_string)
        return ret_msg


    def _deregister(self, register_server):
        end_point = urlparse("%s%s" % (register_server, self.resource_uri))
        json_string = {"status":"TER"}
        ret_msg = http_put(end_point, json_string=json_string)
        return ret_msg


def http_get(end_point):
    #sys.stdout.write("Connecting to %s\n" % (''.join(end_point)))
    params = urllib.urlencode({})
    headers = {"Content-type":"application/json"}
    end_string = "%s?%s" % (end_point[2], end_point[4])

    conn = httplib.HTTPConnection(end_point.hostname, end_point.port, timeout=1)
    conn.request("GET", end_string, params, headers)
    data = conn.getresponse().read()
    response_list = json.loads(data).get('objects', list())
    conn.close()
    return response_list


def http_post(end_point, json_string=None):
    #sys.stdout.write("Connecting to %s\n" % (''.join(end_point)))
    params = json.dumps(json_string)
    headers = {"Content-type":"application/json" }

    conn = httplib.HTTPConnection(end_point[1])
    conn.request("POST", "%s" % end_point[2], params, headers)
    response = conn.getresponse()
    data = response.read()
    conn.close()
    if data:
        return json.loads(data)
    return None


def http_put(end_point, json_string=None):
    #sys.stdout.write("Connecting to %s\n" % (''.join(end_point)))
    params = json.dumps(json_string)
    headers = {"Content-type":"application/json" }

    conn = httplib.HTTPConnection(end_point[1])
    conn.request("PUT", "%s" % end_point[2], params, headers)
    response = conn.getresponse()
    data = response.read()
    conn.close()
    return json.loads(data)


def get_local_ipaddress():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("gmail.com",80))
    ipaddress = (s.getsockname()[0])
    s.close()
    return ipaddress


def process_command_line(argv):
    USAGE = 'Usage: %prog -s register_server'
    DESCRIPTION = 'Cloudlet register thread'

    parser = OptionParser(usage=USAGE, description=DESCRIPTION)

    parser.add_option(
            '-s', '--server', action='store', dest='register_server',
            help='IP address of directory server')
    settings, args = parser.parse_args(argv)
    if not settings.register_server:
        parser.error("need server dns")
    return settings, args


def main(argv):
    settings, args = process_command_line(sys.argv[1:])
    registerThread = RegisterThread(settings.register_server, update_period=60)
    try:
        registerThread.start()
        time.sleep(60*60*60*60)
    except KeyboardInterrupt as e:
        LOG.info("User interrupt")
    finally:
        registerThread.terminate()
    return 0


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
