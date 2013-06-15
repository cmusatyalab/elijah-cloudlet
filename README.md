Elijah: Cloudlet Infrastructure for Mobile Computing
========================================================
A cloudlet is a new architectural element that arises from the convergence of
mobile computing and cloud computing. It represents the middle tier of a
3-tier hierarchy:  mobile device - cloudlet - cloud.   A cloudlet can be
viewed as a "data center in a box" whose  goal is to "bring the cloud closer".
A cloudlet has four key attributes: 

Copyright (C) 2011-2012 Carnegie Mellon University
This is a developing project and some features might not be stable yet.
Please visit our website at [Elijah page](http://elijah.cs.cmu.edu/).


License
----------

All source code, documentation, and related artifacts associated with the
cloudlet open source project are licensed under the [Apache License, Version
2.0](http://www.apache.org/licenses/LICENSE-2.0.html).

A copy of this license is reproduced in the [LICENSE](LICENSE) file, and the
licenses of dependencies and included code are enumerated in the
[NOTICE](NOTICE) file.


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
* libc6-i386 (for extracting free memory of 32 bit vm)
* python library
    - msgpack-python
    - bson
	- pyliblzma
	- psutil
	- SQLAlchemy

To install:

1. install library dependency
   Example at ubuntu 12 LTS x86.

		> $ sudo apt-get install qemu-kvm libvirt-bin gvncviewer python-libvirt python-xdelta3 python-dev openjdk-6-jre liblzma-dev apparmor-utils libc6-i386 python-pip
		> $ sudo pip install msgpack-python bson pyliblzma psutil sqlalchemy

2. Disable security module.
   Example at Ubuntu 12

		> $ sudo aa-complain /usr/sbin/libvirtd

3. add current user to kvm, libvirtd group.

		> $ sudo adduser [your_account_name] kvm
		> $ sudo adduser [your_account_name] libvirtd



Recommended platform
---------------------

We have tested at __Ubuntu 12.04 LTS 64-bit__

This version of Cloudlet has several dependencies on other projects for
further optimization, and currently we include this dependency as a binary.
Therefore, we recommend you to use __Ubuntu 12.04 LTS 64-bit__



How to use
--------------			

1. Creating ``base vm``.  
	You will first create ``base vm`` from a regular VM disk image. This ``base
	vm`` will be a template VM for overlay VMs. To create ``base vm``, you need
	regular VM disk image in a raw format.  

        > $ cd ./bin
        > $ ./cloudlet base /path/to/base_disk.img
        > (__Use raw file format__)

	This will launch remote connection(VNC) to guest OS and cloudlet module
	will automatically start creating ``base vm`` when you close VNC window.
	After finishing all the processing, you can check generated ``base vm``
	using below command.

    	> $ cd ./bin
    	> $ ./cloudlet list_base


2. Creating ``overlay vm`` on top of ``base vm``.  
    Now you can create your customized VM based on top of ``base vm``  
  
        > $ cd ./bin
        > $ ./cloudlet overlay /path/to/base_disk.img

	This will launch VNC again. On top of this ``base vm``, you can install(and
	execute) your custom server. For example, if you're a developer of ``face
	recognition`` backend server, we will install required libraries and start
	your server. Cloudlet will automatically extracts this customized part from
	the ``base vm`` when you close VNC, and it will be your overlay.

	``overlay VM`` is composed of 2 files; 1) ``overlay-meta file`` ends with
	.overlay-meta, 2) compressed ``overlay blob files`` ends with .xz


	Note: if your application need specific port and you want to make a port
	forwarding host to VM, you can use -redir parameter as below. 

        > $ ./cloudlet overlay /path/to/base_disk.img -- -redir tcp:2222::22 -redir tcp:8080::80

	This will forward client connection at host port 2222 to VM's 22 and 8080
	to 80, respectively.


	### Note

	If you have kernel related issue like
	[this](https://github.com/cmusatyalab/elijah-cloudlet/issues/1), You should
	follow workaround for this problem. It happens at low-end machine with EPT
	support, and you can avoid it by disabling EPT support.


3. Synthesizing ``overlay vm``  

	Here, we'll show 3 different ways to perform VM synthesis using ``overlay
	vm`` that you just generated; 1) verifying synthesis using command line
	interface, 2) synthesize over network using desktop client, and 3)
	synthesize over network using Android client.  

    1) Command line interface: You can resume your ``overlay vm`` using 

        > $ cd ./bin
        > $ ./cloudlet synthesis /path/to/base_disk.img /path/to/overlay-meta
    
    2) Network client (python version)  

	We have a synthesis server that received ``VM synthesis`` request from
	mobile client and you can start the server as below.
  
        > $ cd ./bin
        > $ ./server
    
	You can test this server using the client. You also need to copy the
	overlay that you like to reconstruct to the other machine when you execute
	this client.
    
        > $ ./rapid_client.py -s [cloudlet ip address] -o [/path/to/overlay-meta]

    
    3) Network client (Android version)

	We have source codes for android client at ./src/client/andoid and you can
	import it to ``Eclipse`` as an Android porject. This client program will
	automatically find nearby Cloudlet using UPnP if both client and Cloudlet
	are located in same broadcasting domain (ex. share WiFi access point)

	Once installing application at your mobile device, you should copy your
	overlay VM (both overlay-meta and xz file) to Android phone. You can copy
	it to /sdcard/Cloudlet/overlay/ directory creating your overlay directory
	name.  For example, you can copy your ``face recognition overlay vm`` to
	/sdcard/Cloudlet/overlay/face/ directory. This directory name will be
	appeared to your Android application when you're asked to select ``overlay
	vm``.  Right directory name is important since the directory name will be
	saved as appName in internal data structure and being used to launch
	associated mobile application after finishing ``VM synthesis``. See more
	details at handleSucessSynthesis() method at CloudletConnector.java file.



Compiling external library that Cloudlet uses
----------------------------------------------

You will need:

* qemu-kvm 1.1.1 (for Free memory and TRIM support)
* libc6-dev-i386 (for Free memory support)


Research works
--------------------------

* [The Case for VM-based Cloudlets in Mobile Computing](http://www.cs.cmu.edu/~satya/docdir/satya-ieeepvc-cloudlets-2009.pdf)
* The Impact of Mobile Multimedia Applications on Data Center Consolidation (To be appeared)
* [Just-in-Time Provisioning for Cyber Foraging](http://reports-archive.adm.cs.cmu.edu/anon/2012/CMU-CS-12-148.pdf)
* [Scalable Crowd-Sourcing of Video from Mobile Devices](http://reports-archive.adm.cs.cmu.edu/anon/2012/CMU-CS-12-147.pdf)


