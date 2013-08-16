1. install cinder-volume fails (https://answers.launchpad.net/openstack-manuals/+question/228960)

	Download the 1.0.15 version librdmacm1 from
	http://pkgs.org/ubuntu-12.10/ubuntu-main-amd64/librdmacm1_1.0.15-1_amd64.deb/download/,
	and install it using: dpkg -i librdmacm1_1.0.15-1_amd64.deb


2. SSL error in quantum

	change configuration file quantum.conf to add

	[keystone_authtoken]
	auth_protocol=http


3. Unable to communicate with identity service: {"error": {"message": "The request you have made requires authentication.", "code": 401, "title": "Not Authorized"}}. (HTTP 401)

	Authentication problem at keystone. OS_TENANT_NAME was wrong.


4. openstack /usr/lib/python2.7/dist-packages/bin/nova-dhcpbridge

	bindir=/usr/bin in nova.conf


5. DHCP client problem

	It's because virtual network does not properly generate chuecksum and dhcp
	client ignore that packet since it thought the message is correupted.

	iptables -A POSTROUTING -t mangle -p udp --dport 68 -j CHECKSUM --checksum-fill
	(https://github.com/mseknibilel/OpenStack-Folsom-Install-guide/issues/14)


6. IMPORTANT: Internet connection error using FlatDHCP network

	flat_intgerface should be interface that is not connected to Internet
		> nova.conf
		network_manager=nova.network.manager.FlatDHCPManager
		my_ip=[my_ip]
		public_interface=eth0
		flat_network_bridge=br100
		flat_interface=eth1
		fixed_range=''

	$ sudo ip link set eth1 promisc on
	$ sudo brctl addbr br100
	$ nova network-create private --fixed-range-v4=192.168.100.0/24 --bridge-interface=br100
	$ sudo /etc/init.d/networking restart

	make sure your routing table properly set for eth0
		> route -n

		> ifconfig
		br100     Link encap:Ethernet  HWaddr 68:05:ca:06:2a:58  
			...

		eth0      Link encap:Ethernet  HWaddr 18:03:73:3c:c0:57  
			...

	Floating IP addresses:
		$ echo 1 > /proc/sys/net/ipv4/ip_forward
		ex) nova-manage floating create --ip_range=68.99.26.170/31


	ref: http://www.mirantis.com/blog/openstack-networking-single-host-flatdhcpmanager/


7. install cinder-volume fails (https://answers.launchpad.net/openstack-manuals/+question/228960)

	Download the 1.0.15 version librdmacm1 from
	http://pkgs.org/ubuntu-12.10/ubuntu-main-amd64/librdmacm1_1.0.15-1_amd64.deb/download/,
	and install it using: dpkg -i librdmacm1_1.0.15-1_amd64.deb


8. SSL error in quantum

	change configuration file quantum.conf to add

	[keystone_authtoken]
	auth_protocol=http


9.  File "/usr/lib/python2.7/dist-packages/migrate/versioning/schema.py", line 10, in <module>
    from sqlalchemy import exceptions as sa_exceptions
    ImportError: cannot import name exceptions

    It is cause by the conflict in sqlalchemy at /usr/local/lib and /usr/lib.
    So remove sqlalchemy library at /usr/local/lib, which is latest version


10. Add new compute node
	- install ntp
	- install nova-compute nova-network only
	- ip link set eth1 promisc on
	- set ip address of br100 to the first address from fix_range set in nova.conf
	- http://docs.openstack.org/essex/openstack-compute/install/apt/content/installing-additional-compute-nodes.html
