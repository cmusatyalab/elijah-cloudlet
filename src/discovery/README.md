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
* mysql-server >= 14.14

	> $ sudo apt-get install mysql-server python-mysqldb
	> $ sudo pip install django django-tastypie pytz mysql-python

Then, create user/database at mysql and register it at mysql.conf file at
project directory. For example,

	> $ mysql -u root -p 
	> mysql > CREATE USER 'cloudlet'@'localhost' IDENTIFIED BY 'cloudlet'
	> mysql > GRANT ALL PRIVILEGES ON *.* TO 'cloudlet'@'localhost'
	> mysql > FLUSH PRIVILEGES;
	>
	> $ cat mysql.conf 
	> [client]
	> database = cloudlet_registration
	> user = cloudlet
	> password = cloudlet
	> default-character-set = utf8
	> $


DNS server for cloudlet
-----------------------------
- At ``dns_server`` directory
- Custom DNS server that returns cloudlet's ip address based on geolocation proximity

## Install ##
You will need:
* Twisted

	> $ sudo apt-get install python-twisted


UPnP discovery
--------------------
- At ``upnp`` directory
- Zeroconf type discovery within broadcasting domain

## Install ##
