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

from twisted.names.server import DNSServerFactory
from dns_resolver import MemoryResolver

class CloudletDNSServerFactory(DNSServerFactory):
    def __init__(self, authorities = None, caches = None, clients = None, verbose = 0):
        resolvers = []
        if authorities is not None:
            resolvers.extend(authorities)
        if caches is not None:
            resolvers.extend(caches)
        if clients is not None:
            resolvers.extend(clients)

        self.canRecurse = not not clients
        self.resolver = resolvers[0]
        self.verbose = verbose
        if caches:
            self.cache = caches[-1]
        self.connections = []

    def handleQuery(self, message, protocol, address):
        # Discard all but the first query!  HOO-AAH HOOOOO-AAAAH
        # (no other servers implement multi-query messages, so we won't either)
        query = message.queries[0]

        # This part is added to pass source address to resolver
        if isinstance(self.resolver, MemoryResolver):
            self.resolver.address = address

        return self.resolver.query(query).addCallback(
            self.gotResolverResponse, protocol, message, address
        ).addErrback(
            self.gotResolverError, protocol, message, address
        )


