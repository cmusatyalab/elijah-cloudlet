import os, traceback

from twisted.names import authority
from twisted.names.authority import PySourceAuthority
from twisted.python import usage
from twisted.names import dns
from twisted.names import secondary
from twisted.python import failure
from twisted.internet import defer

class Options(usage.Options):
    optParameters = [
        ["interface", "i", "",   "The interface to which to bind"],
        ["port",      "p", "53", "The port on which to listen"],
        ["resolv-conf", None, None,
            "Override location of resolv.conf (implies --recursive)"],
        ["hosts-file", None, None, "Perform lookups with a hosts file"],
    ]

    optFlags = [
        ["cache",       "c", "Enable record caching"],
        ["recursive",   "r", "Perform recursive lookups"],
        ["verbose",     "v", "Log verbosely"],
    ]
    
    '''
    compData = usage.Completions(
        optActions={"interface" : usage.CompleteNetInterfaces()}
        )
    '''

    zones = None
    zonefiles = None

    def __init__(self):
        usage.Options.__init__(self)
        self['verbose'] = 0
        self.bindfiles = []
        self.zonefiles = []
        self.secondaries = []


    def opt_pyzone(self, filename):
        """Specify the filename of a Python syntax zone definition"""
        if not os.path.exists(filename):
            raise usage.UsageError(filename + ": No such file")
        self.zonefiles.append(filename)

    def opt_bindzone(self, filename):
        """Specify the filename of a BIND9 syntax zone definition"""
        if not os.path.exists(filename):
            raise usage.UsageError(filename + ": No such file")
        self.bindfiles.append(filename)


    def opt_secondary(self, ip_domain):
        """Act as secondary for the specified domain, performing
        zone transfers from the specified IP (IP/domain)
        """
        args = ip_domain.split('/', 1)
        if len(args) != 2:
            raise usage.UsageError("Argument must be of the form IP[:port]/domain")
        address = args[0].split(':')
        if len(address) == 1:
            address = (address[0], dns.PORT)
        else:
            try:
                port = int(address[1])
            except ValueError:
                raise usage.UsageError(
                    "Specify an integer port number, not %r" % (address[1],))
            address = (address[0], port)
        self.secondaries.append((address, [args[1]]))


    def opt_verbose(self):
        """Increment verbosity level"""
        self['verbose'] += 1


    def postOptions(self):
        if self['resolv-conf']:
            self['recursive'] = True

        self.svcs = []
        self.zones = []
        for f in self.zonefiles:
            try:
                self.zones.append(MemoryResolver(f))
            except Exception:
                traceback.print_exc()
                raise usage.UsageError("Invalid syntax in " + f)
        for f in self.bindfiles:
            try:
                self.zones.append(authority.BindAuthority(f))
            except Exception:
                traceback.print_exc()
                raise usage.UsageError("Invalid syntax in " + f)
        for f in self.secondaries:
            svc = secondary.SecondaryAuthorityService.fromServerAddressAndDomains(*f)
            self.svcs.append(svc)
            self.zones.append(self.svcs[-1].getAuthority())
        try:
            self['port'] = int(self['port'])
        except ValueError:
            raise usage.UsageError("Invalid port: %r" % (self['port'],))


class MemoryResolver(PySourceAuthority):

    def add_A_record(self, name, ip_address, ttl=None):
        domain_records = self.records.get(name.lower())
        if not domain_records:
            domain_records = list()
            
        domain_records.append({'ttl':ttl, 'address':ip_address}) 
        self.records[name.lower()] = domain_records

    def _lookup(self, name, cls, type, timeout = None):
        cnames = []
        results = []
        authority = []
        additional = []
        default_ttl = max(self.soa[1].minimum, self.soa[1].expire)

        domain_records = self.records.get(name.lower())

        if domain_records:
            for record in domain_records:
                if record.ttl is not None:
                    ttl = record.ttl
                else:
                    ttl = default_ttl

                if record.TYPE == dns.NS and name.lower() != self.soa[0].lower():
                    # NS record belong to a child zone: this is a referral.  As
                    # NS records are authoritative in the child zone, ours here
                    # are not.  RFC 2181, section 6.1.
                    authority.append(
                        dns.RRHeader(name, record.TYPE, dns.IN, ttl, record, auth=False)
                    )
                elif record.TYPE == type or type == dns.ALL_RECORDS:
                    results.append(
                        dns.RRHeader(name, record.TYPE, dns.IN, ttl, record, auth=True)
                    )
                if record.TYPE == dns.CNAME:
                    cnames.append(
                        dns.RRHeader(name, record.TYPE, dns.IN, ttl, record, auth=True)
                    )
            if not results:
                results = cnames

            for record in results + authority:
                section = {dns.NS: additional, dns.CNAME: results, dns.MX: additional}.get(record.type)
                if section is not None:
                    n = str(record.payload.name)
                    for rec in self.records.get(n.lower(), ()):
                        if rec.TYPE == dns.A:
                            section.append(
                                dns.RRHeader(n, dns.A, dns.IN, rec.ttl or default_ttl, rec, auth=True)
                            )

            if not results and not authority:
                # Empty response. Include SOA record to allow clients to cache
                # this response.  RFC 1034, sections 3.7 and 4.3.4, and RFC 2181
                # section 7.1.
                authority.append(
                    dns.RRHeader(self.soa[0], dns.SOA, dns.IN, ttl, self.soa[1], auth=True)
                    )
            return defer.succeed((results, authority, additional))
        else:
            if name.lower().endswith(self.soa[0].lower()):
                # We are the authority and we didn't find it.  Goodbye.
                return defer.fail(failure.Failure(dns.AuthoritativeDomainError(name)))
            return defer.fail(failure.Failure(dns.DomainError(name)))
