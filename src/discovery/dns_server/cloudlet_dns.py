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

"""
Domain Name Server for Cloudlet
"""

import os
import sys
import socket

from twisted.internet import reactor
from twisted.names import dns

from server_factory import CloudletDNSServerFactory
from dns_resolver import Options as Options
from dns_resolver import MemoryResolver as MemoryResolver

class CloudletDNSError(Exception):
    pass


class CloudletDNS(object):
    def __init__(self, zone_file):
        input_file = os.path.abspath(zone_file)
        options = Options()
        options.opt_pyzone(input_file)
        options.opt_verbose()
        options.postOptions()

        ca, cl = self._buildResolvers(options)
        self.factory = CloudletDNSServerFactory(options.zones, ca, cl, options['verbose'])
        self.protocol = dns.DNSDatagramProtocol(self.factory)
        self.factory.noisy = 0

        self.local_ipaddress = self._local_ipaddress()
        self.add_A_record("ns1.%s" % MemoryResolver.CLOUDLET_DOMAIN, self.local_ipaddress)
        self.add_A_record("ns2.%s" % MemoryResolver.CLOUDLET_DOMAIN, self.local_ipaddress)


    def _local_ipaddress(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("gmail.com",80))
        ipaddress = (s.getsockname()[0])
        s.close()
        return ipaddress

    def list_record(self, name, record_type=None):
        zone = self.factory.resolver
        domain_records = zone.records.get(name.lower())
        if domain_records:
            ret_record = []
            if record_type:
                ret_record = [item for item in domain_records if item.TYPE == record_type]
            else:
                ret_record = domain_records
            return ret_record
        return None

    def add_A_record(self, name, address, ttl=None):
        if self._is_valid_ip(address) == False:
            raise CloudletDNSError("Invalid ip address: %s" % address)

        zone = self.factory.resolver
        domain_records = zone.records.get(name.lower())
        new_record = dns.Record_A(address=address, ttl=ttl)
        if domain_records:
            domain_records.append(new_record)
        else:
            domain_records = [new_record]
            zone.records[name.lower()] = domain_records

    def start_dns(self):
        reactor.listenUDP(53, self.protocol)
        reactor.listenTCP(53, self.factory)
        reactor.run()

    def _buildResolvers(self, config):
        """
        Build DNS resolver instances in an order which leaves recursive
        resolving as a last resort.

        @type config: L{Options} instance
        @param config: Parsed command-line configuration

        @return: Two-item tuple of a list of cache resovers and a list of client
            resolvers
        """
        from twisted.names import client, cache, hosts

        ca, cl = [], []
        if config['cache']:
            ca.append(cache.CacheResolver(verbose=config['verbose']))
        if config['hosts-file']:
            cl.append(hosts.Resolver(file=config['hosts-file']))
        if config['recursive']:
            cl.append(client.createResolver(resolvconf=config['resolv-conf']))
        return ca, cl

    def _is_valid_ip(self, address):
        try:
            socket.inet_aton(address)
            return True
        except socket.error:
            return False


def main(argv):
    if len(argv) != 1:
        sys.stderr.write("need input file\n")
        return 1
    input_file = argv[0]
    cloudlet_dns = CloudletDNS(input_file)

    # print DNS status
    zone_name = "findcloudlet.org"
    record_list = "\n".join(["  " + str(item) for item in cloudlet_dns.list_record(zone_name)])
    print "%s -> \n%s\n" % (zone_name, record_list)
    zone_name = "new_device.findcloudlet.org"
    print "START DNS SERVER"

    # start
    cloudlet_dns.start_dns()

if __name__ == "__main__":
    status = main(sys.argv[1:])
    sys.exit(status)
