This is directory for Cloudlet discovery

Tested platform
--------------------
RHEL 6 server 32 bit
Ubuntu 12.04 LTS 64 bit


RESTful registration server
----------------------------
- At ``register_server`` directory
- Central directory server using RESTful API

## Install ##
You will need:
* Django >= 1.4.3
* django-tastypie >= 0.9.11
* pytz >= 2012f
* python-mysqldb
* pygeoip >= 0.2.5
* mysql-server >= 14.14

	> $ sudo apt-get install mysql-server python-mysqldb
	> $ sudo pip install django django-tastypie pytz mysql-python pygeoip

Then, create user/database at mysql and register it at mysql.conf file at
project directory. For example,

	> $ mysql -u root -p 
	> mysql> CREATE USER 'cloudlet'@'localhost' IDENTIFIED BY 'cloudlet';
	> mysql> GRANT ALL PRIVILEGES ON *.* TO 'cloudlet'@'localhost';
	> mysql> FLUSH PRIVILEGES;
	> mysql> CREATE DATABASE cloudlet_registration;
	>
	> $ cat mysql.conf 
	> [client]
	> database = cloudlet_registration
	> user = cloudlet
	> password = cloudlet
	> default-character-set = utf8
	> $

Finally, you need IP geolocation DB to estimate location of Cloudlet machine.
You can download it from [link](http://dev.maxmind.com/geoip/geolite).
Or execute download_geoip_db.sh as follows:

	> ./download_geoip_db.sh


DNS server for cloudlet
-----------------------------
- At ``dns_server`` directory
- Custom DNS server that returns cloudlet's ip address based on geolocation proximity

## Install ##
You will need:
* Twisted
* sqlalchemy

	> $ sudo apt-get install python-twisted python-sqlalchemy


You can run this DNS Server as DAEMON using twistd

	> $ sudo twistd -y dns.tac

UPnP discovery
--------------------
- At ``upnp`` directory
- Zeroconf type discovery within broadcasting domain

## Install ##



SAMBA
--------------------
###Server side###
1. $ sudo apt-get install samba system-config-samba

2. make samba directory and change owner and mode
	> $ sudo mkdir /var/samba
	> $ sudo chown nobody:nogroup /var/samba
	> $ sudo chmod 771 /var/samba

3. samba configuration at /etc/samba/smb.conf
	> [global]
	> workgroup = workgroup
	> display charset = UTF8
	> unix charset = UTF8
	> 
	> ; load printers = yes
	> ; printing = lpmg
	> 
	> server string = CloudletSamba
	> printcap name = /etc/printcap
	> cups options = raw
	> log file = /var/log/samba/log.%m
	> include = /var/log/samba/smb.conf.%m
	> log level = 1
	> max log size = 100000
	> 
	> follow symlinks = yes
	> wide links = yes
	> unix extensions = no
	> 
	> interfaces = eth0 lo
	> bind interfaces only = true
	> hosts allow = localhost 127.0.0.1
	> ;security = USER
	> security = share
	> guest account = nobody
	> guest ok = yes
	> ;public = yes
	> writeable = yes
	> read only = no
	> usershare allow guests = yes 
	> create mask = 0771
	> directory mask = 0771
	> force user = nobody
	> force group = nogroup
	> 
	> socket options = TCP_NODELAY SO_RCVBUF=8192 SO_SNDBUF=8192
	> dns proxy = no
	> password server = None
	> 
	> [cloudlet]
	> comment = cloudlet samba
	> path = /var/samba


###Client side###
1. $ sudo addgroup user_name nogroup
	otherwise, typically user does not belong to nogroup, so it will limite you write permission
2. samba mount
	> $ sudo mkdir /share
	> $ sudo mount -t cifs //10.0.2.2/cloudlet /share -o directio,username=guest,iocharset=utf8
	> % it's important to mount with directio option to avoid page cache

