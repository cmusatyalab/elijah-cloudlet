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

from cloudlet import log as logging


LOG = logging.getLogger(__name__)

class DiscoveryConst(object):
    REGISTER_URL        = "/api/v1/Cloudlet/"

    KEY_REST_PORT   = "rest_api_port"
    KEY_REST_URL    = "rest_api_url"
    KEY_CLOUDLET_IP = "ip_address"
    KEY_LATITUDE    = "latitude"
    KEY_LONGITUDE   = "longitude"

    REST_API_PORT       = 8022
    REST_API_URL        = "/api/v1/resource/"


class RESTConst(object):
    DFS_ROOT_DIR = "/magfs/home/kiryongh@ANDREW.AD.CMU.EDU"
