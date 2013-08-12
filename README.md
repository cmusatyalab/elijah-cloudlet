Elijah: Cloudlet Infrastructure for Mobile Computing
========================================================
A cloudlet is a new architectural element that arises from the convergence of
mobile computing and cloud computing. It represents the middle tier of a
3-tier hierarchy:  mobile device - cloudlet - cloud.   A cloudlet can be
viewed as a "data center in a box" whose  goal is to "bring the cloud closer".

Copyright (C) 2011-2012 Carnegie Mellon University
This is a developing project and some features might not be stable yet.
Please visit our website at [Elijah page](http://elijah.cs.cmu.edu/).



License
----------

All source code, documentation, and related artifacts associated with the
cloudlet open source project are licensed under the [Apache License, Version
2.0](http://www.apache.org/licenses/LICENSE-2.0.html).



Before you start
-----------------

This code is about **Virtual Machine Synthesis** that aimed to provide 
__Rapid provisioning of a custom virtual machine**. This does not include any
codes for mobile applications, rather it provides functions to create
**VM overlay** and perform **VM Synthesis** that will rapidly reconstruct 
your custom VM at an arbitrary computer.

Please read [The Case for VM-based Cloudlets in Mobile Computing](https://github.com/cmusatyalab/elijah-cloudlet/blob/master/doc/papers/satya-ieeepvc-cloudlets-2009.pdf?raw=true)
to understand what we do here and find the detail techniques at
[Just-in-Time Provisioning for Cyber Foraging](https://github.com/cmusatyalab/elijah-cloudlet/blob/master/doc/papers/kiryong-mobisys-vmsynthesis.pdf?raw=true)

The key to rapid provisioning is the recognition that a large part of
a VM image is devoted to the guest OS, software libraries, and
supporting software packages. The customizations of a base system
needed for a particular application are usually relatively small.
Therefore, if the ``base VM`` already exists on the cloudlet, only
its difference relative to the desired custom VM, called a ``VM overlay``,
needs to be transferred. Our approach of using VM overlays
to provision cloudlets is called ``VM synthesis``.  Good analogy is
a QCOW2 file with a backing file. can consider ``VM overlay`` as
a QCOW2 file and ``Base VM`` as a backing file. The main difference 
is that ``VM synthesis`` includes both disk and memory state and 
it is much more efficient in generating diff and reconstructing
suspended state.



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
    - bson
	- pyliblzma
	- psutil
	- SQLAlchemy
	- fabric

To run a installation script:

		> $ sudo apt-get install fabric openssh-server
		> $ fab localhost install

To install manually:

	1. install required package
			> $ sudo apt-get install qemu-kvm libvirt-bin gvncviewer python-libvirt python-xdelta3 python-dev openjdk-6-jre liblzma-dev apparmor-utils libc6-i386 python-pip
			> $ sudo pip install bson pyliblzma psutil sqlalchemy

	2. Disable security module. This is for allowing custom KVM.
	Example at Ubuntu 12

			> $ sudo aa-complain /usr/sbin/libvirtd

	3. add current user to kvm, libvirtd group.

			> $ sudo adduser [your_account_name] kvm
			> $ sudo adduser [your_account_name] libvirtd
	
	4. change permission of the fuse access (The qemu-kvm library changes fuse
	   access permission while it's being installed, and the permission is
	   recovered if you reboot the host.  We believe this is a bug in qemu-kvm
	   installation script, so you can either reboot the machine to have valid
	   permission of just revert the permission manually as bellow).

		   > $ sudo chmod 1666 /dev/fuse
		   > $ sudo chmod 644 /etc/fuse.conf
		   > $ sod sed -i 's/#user_allow_other/user_allow_other/g' /etc/fuse.conf



Recommended platform
---------------------

We have tested at __Ubuntu 12.04 LTS 64-bit__

This version of Cloudlet has several dependencies on other projects for
further optimization, and currently we include this dependency as a binary.
Therefore, we recommend you to use __Ubuntu 12.04 LTS 64-bit__



How to use
--------------			

1. Creating ``base vm``.  
	You will first create ``base vm`` from a regular VM disk image. Here the
	__regular VM disk image__ means a raw format virtual disk image 
	you typically use at KVM/QEMU or Xen. The code will start running the OS
	in this virtual disk and finally generate ``base vm``, which is composed
	``base disk`` and ``base memory``. 
	This ``base vm`` will be used as a template VM for your custom virtual machine.

        > $ cd ./bin
        > $ ./cloudlet base /path/to/base_disk.img
        > (__Use raw file format__)

	This will launch GUI (VNC) connecting to your guest OS and cloudlet module
	will start creating ``base vm`` when you close VNC window. So please loggin
	to the guest OS and close the GUI window when you think it's right snapshotting
	point as a base VM.
	The code will generate snapshot of the VM (for both memory and disk) and
	save the information at DB. You can check list of ``base vm`` by

		> $ ./cloudlet list-base


2. Creating ``overlay vm`` on top of ``base vm``.  
    Now you can create your customized VM based on top of ``base vm``  
  
        > $ cd ./bin
        > $ ./cloudlet overlay /path/to/base_disk.img
        > % Path to base_disk is the path for virtual disk you used ealier
        > % You can check the path by "cloudlet list-base"

	This will launch VNC again with resumed ``base vm``. Now you can start making
	any customizations on top of this ``base vm``. For example, if you're a
	developer of ``face recognition`` backend server, we will install required
	libraries, binaries and finally start your face recongition server. 
	After closing the GUI windows, cloudlet will capture only the change portion
	betwee your customization and ``base vm`` to generate ``VM overlay`` that
	is a minimal binary for reconsturcting your customized VM.

	``overlay VM`` is composed of 2 files; 1) ``overlay-meta file`` ends with
	.overlay-meta, 2) compressed ``overlay blob files`` ends with .xz


	Note: if your application need specific port and you want to make a port
	forwarding host to VM, you can use -redir parameter as below. 

        > $ ./cloudlet overlay /path/to/base_disk.img -- -redir tcp:2222::22 -redir tcp:8080::80

	This will forward client connection at host port 2222 to VM's 22 and 8080
	to 80, respectively.


	### Note

	If you have experience kernel panic error like
	[this](https://github.com/cmusatyalab/elijah-cloudlet/issues/1), You should
	follow workaround of this link. It happens at a machine that does not have
	enough memory with EPT support, and you can avoid this problem by disabling
	EPT support. We're current suspicious about kernel bug, and we'll report
	this soon.


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
	import it to ``Eclipse`` as an Android project. This client program will
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

* [The Case for VM-based Cloudlets in Mobile Computing](https://github.com/cmusatyalab/elijah-cloudlet/blob/master/doc/papers/satya-ieeepvc-cloudlets-2009.pdf?raw=true)
* [The Impact of Mobile Multimedia Applications on Data Center Consolidation](https://github.com/cmusatyalab/elijah-cloudlet/blob/master/doc/papers/kiryong-ic2e-latency.pdf?raw=true)
* [Just-in-Time Provisioning for Cyber Foraging](https://github.com/cmusatyalab/elijah-cloudlet/blob/master/doc/papers/kiryong-mobisys-vmsynthesis.pdf?raw=true)
* [Scalable Crowd-Sourcing of Video from Mobile Devices](http://reports-archive.adm.cs.cmu.edu/anon/2012/CMU-CS-12-147.pdf)


