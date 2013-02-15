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


class cloudlet_resource_static_t(object):
    number_socket = None
    number_cores = None
    number_threads_per_cores = None
    cpu_clock_speed_ghz = None
    mem_total_mb = None
    LLC_size_mb = None


class cloudlet_resource_dynamic_t(object):
    mem_free_mb = None
    cpu_usage_percent = None


class cloudlet_t(object):
    ip_v4 = ''
    port_number = int(-1)
    resource_static = cloudlet_resource_static_t()
    resource_dynamic = cloudlet_resource_dynamic_t()


class Discovery(object):
    '''This class has a set of static methods that are a primitive method
    for cloudlet discovery. Some method format is not python-like and it is
    because we tried to keep consistent format with C API.
    '''

    FAILED  = 0
    SUCCESS = 1

    @staticmethod
    def find_nearby_cloudlets(cloudlet_list_ret):
        '''Step 1. Find several candidate using DNS
        :param cloudlet_list_ret: cloudlet_t objects will be returned at this list
        :type cloudlet_list_ret: list

        :return: success/fail
        :rtype: int
        '''
        return Discovery.SUCCESS

    @staticmethod
    def get_cloudlet_info(cloudlet_t):
        '''Step 1. fill out cloudlet_t field with more detailed information
        :param cloudlet_t: cloudlet_t instance that has ip_address of the cloudlet
        :type cloudlet_t: cloudlet_t

        :return: success/fail
        :rtype: int
        '''
        return Discovery.SUCCESS

    @staticmethod
    def associate_with_cloudlet(cloudlet_t):
        '''Step 2. Associate with given cloudlet
        :param cloudlet_t: cloudlet_t instance that has ip_address of the cloudlet
        :type cloudlet_t: cloudlet_t

        :return: session id or -1 if it failed
        :rtype: long
        '''

        return long(0)

    @staticmethod
    def disassociate(session_id):
        '''Step 2. disassociate with given cloudlet
        :param session_id: session_id that was returned when associated
        :type session_id: long

        :return: N/A
        '''
        pass

    @staticmethod
    def get_cost_of_cache_state_by_filename(session_id, file_names):
        '''Step 3. get the expected cost of warnming the cache
        :param session_id: session id
        :type session_id: long
        :param file_names: list of file name that will be used for running application
        :type file_names: list of string

        :return: expected cost. Less is better
        :rtype: long
        '''
        pass

    def get_cost_of_cache_state_by_hash(session_id, hash_list):
        '''Step 3. get the expected cost of warnming the cache
        :param session_id: session id
        :type session_id: long
        :param hash_list: list of hash that will be used for running application
        :type hash_list: list of hash

        :return: expected cost. Less is better
        :rtype: long
        '''
        pass


def main(argv):
    settings, args = process_command_line(sys.argv[1:])
    client = CloudletDiscoveryClient(settings.server_dns, log=sys.stdout)
    ret_list = client.search()
    print ret_list
    return 0


if __name__ == "__main__":
    status = main(sys.argv)
    sys.exit(status)
