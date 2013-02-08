Elijah: Cloudlet Infrastructure for Mobile Computing
========================================================
A cloudlet is a new architectural element that arises from the convergence of
mobile computing and cloud computing. It represents the middle tier of a
3-tier hierarchy:  mobile device - cloudlet - cloud.   A cloudlet can be
viewed as a "data center in a box" whose  goal is to "bring the cloud closer".
A cloudlet has four key attributes: 

Copyright (C) 2011-2012 Carnegie Mellon University
This is a developing project and some features might not be stable yet.
Please visit our website at <http://elijah.cs.cmu.edu/>.

Cloudlet is licensed under the GNU General Public License, version 2.



Installing
----------

You will need:

* qemu-kvm
* libvirt-bin
* gvncviewer
* python-libvirt
* python-xdelta3
* python-dev (for message pack)
* liblzma-dev (for pyliblzma)
* Java JRE (for UPnP server)
* apparmor-utils (for disable apparmor for libvirt)
* python library
    - msgpack-python
    - bson
	- pyliblzma

To install:

1. install library dependency
   Example at ubuntu 12 LTS x86.

		> $ sudo apt-get install qemu-kvm libvirt-bin gvncviewer python-libvirt python-xdelta3 python-dev openjdk-6-jre liblzma-dev apparmor-utils python-pip
		> $ sudo pip install msgpack-python bson pyliblzma

2. Disable security module.
   Example at Ubuntu 12

		> $ sudo aa-complain /usr/sbin/libvirtd

3. add current user to kvm, libvirtd group.

		> $ sudo adduser [user_name] kvm
		> $ sudo adduser [user_name] libvirtd



Recommended platform
---------------------

We have tested at __Ubuntu 12.04 LTS 64-bit__

This version of Cloudlet code have several dependencies on other project for
further optimization, and currently we include this dependency as a binary.
Therefore, it recommend you to use __Ubuntu 12.04 LTS 64-bit__



How to use
--------------			

1. Creating ``base vm``.  
	You will first create ``base vm`` from a regular VM disk image. This ``base
	vm`` will be a template VM for overlay VMs. To create ``base vm``, you need
	regular VM disk image in a raw format.  

        > $ cd ./bin
        > $ ./cloudlet base /path/to/vm.img

	This will launch remote connection(VNC) to guest OS and cloudlet module
	will automatically start creating ``base vm`` when you close VNC window.
	After finishing all the processing, you can check generated ``base vm``
	using below command.

    	> $ cd ./bin
    	> $ ./cloudlet list_base


2. Creating ``overlay vm`` on top of ``base vm``.  
    Now you can create your customized VM based on top of ``base vm``  
  
        > $ cd ./bin
        > $ ./cloudlet overlay /path/to/vm.image

	This will launch VNC again. On top of this ``base vm``, you can install(and
	execute) your custom server. For example, if you're a developer of ``face
	recognition`` backend server, we will install required libraries and start
	your server. Cloudlet will automatically extracts this customized part from
	the ``base vm`` when you close VNC, and it will be your overlay.

	``overlay VM`` is composed of 2 files; 1) ``overlay-meta file`` ends with
	.overlay-meta, 2) compressed ``overlay blob files`` ends with .xz


	Note: if your application need specific port and you want to make a port
	forwarding host to VM, you can use -redir parameter as below. 

        > $ ./cloudlet overlay /path/to/vm.image -- -redir tcp:2222::22 -redir tcp:8080::80

	This will forward client connection at host port 2222 to VM's 22 and 8080
	to 80, respectively.


3. Synthesizing ``overlay vm``  

	Here, we'll show 3 different ways to perform VM synthesis using ``overlay
	vm`` that you just generated; 1) verifying synthesis using command line
	interface, 2) synthesize over network using desktop client, and 3)
	synthesize over network using Android client.  

    1) Command line interface: You can resume your ``overlay vm`` using 

        > $ cd ./bin
        > $ ./cloudlet synthesis /path/to/base.image /path/to/overlay-meta
    
    2) Network client (desktop client)  

	We have a synthesis server that received ``VM synthesis`` request from
	mobile client and you can start the server as below.
  
        > $ cd ./bin
        > $ ./synthesis
    
	You can test this server using the client. You also need to copy the
	overlay that you like to reconstruct to the other machine when you execute
	this client.
    
        > $ ./rapid_client.py -s [cloudlet ip address] -o [/path/to/overlay-meta]

    
    3) network client (Android client)

	We have source codes for android client at ./src/client/andoid and you can
	import to ``Eclipse``. This client program will automatically find the
	Cloudlet machine using UPnP if both client and Cloudlet are located in
	broadcasting domain (ex. share WiFi access point)


Compiling external library that Cloudlet uses
----------------------------------------------

You will need:

* qemu-kvm 1.1.1 (for Free memory and TRIM support)
* libc6-dev-i386 (for Free memory support)
