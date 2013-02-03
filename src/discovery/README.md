This is directory for Cloudlet discovery

RESTful registration server
----------------------------
- At ``directory_server`` directory
- Central directory server using RESTful API

## Install ##
You will need:
* Django >= 1.4.3
* django-tastypie >= 0.9.11
* pytz

	> $ sudo pip install django django-tastypie pytz


DNS server for cloudlet
-----------------------------
- At ``dns`` directory
- Custom DNS server that returns cloudlet's ip address based on geolocation


UPnP discovery
--------------------
- At ``upnp`` directory
- Zeroconf type discovery within broadcasting domain

## Install ##
