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


