#
# You can run this .tac file directly as follows to run is as daemon
#    twistd -ny cloudlet_dns.tac
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
sys.path.append(".")	# for lower compatibility in twistd

from twisted.application import internet, service
from twisted.names import server, dns, hosts
from cloudlet_dns import CloudletDNS


CONFIG_FILE = "./findcloudlet.org"
port = 53


# Create a MultiService, and hook up a TCPServer and a UDPServer to it as
# children.
dnsService = service.MultiService()
cloudlet_dns = CloudletDNS(CONFIG_FILE)
tcpFactory = cloudlet_dns.factory
internet.TCPServer(port, tcpFactory).setServiceParent(dnsService)
udpFactory = cloudlet_dns.protocol
internet.UDPServer(port, udpFactory).setServiceParent(dnsService)

# Create an application as normal
application = service.Application("Cloudlet_DNS")

# Connect our MultiService to the application, just like a normal service.
dnsService.setServiceParent(application)
