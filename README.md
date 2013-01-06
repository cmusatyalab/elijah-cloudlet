Ejah: Cloudlet Infrastructure for Mobile Computing
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
* apparmor-utils (for disable apparmor for libvirt)
* python library
    - msgpack-python
    - bson
	- pyliblzma

To install:

1. install library dependency
   Example at ubuntu 12 LTS x86.

		> $ sudo apt-get install qemu-kvm libvirt-bin gvncviewer python-libvirt python-xdelta3 python-dev liblzma-dev apparmor-utils python-pip
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
further optimization, and currently we include dependent codes as a binary.
Therefore we currently recommend you to use __Ubuntu 12.04 LTS 64-bit__



How to use
--------------			

1. Creating ``base vm``.  
    You will first create ``base vm`` from a regular VM disk image. This ``base vm`` will be a template VM for overlay VMs. To create ``base vm``, you need regular VM disk image as a raw format.  

        > $ cd ./bin
        > $ ./cloudlet base /path/to/vm.image

    This will launch remote connection(VNC) to guest OS and cloudlet module will automatically start creating ``base vm`` when you close VNC window.


2. Creating ``overlay vm`` on top of ``base vm``.  
    Now you can create your customized VM based on top of ``base vm``  
  
        > $ cd ./bin
        > $ ./cloudlet overlay /path/to/vm.image

    This will launch VNC again and we can install(and execute) your custom server, which will finally be your overlay.

    ``overlay VM`` is composed of 2 files; 1) ``overlay-meta file`` ends with .overlay-meta, 2) compressed ``overlay blob files`` ends .xz


    Note: if you want to make a portforward from host to VM, you can use -redir parameter as below. 

        > $ ./cloudlet overlay /path/to/vm.image -- -redir tcp:2222:22 -redir tcp:8080::80

    This will forward client connection at host port 2222 to VM's 22 and 8080 to 80, respectively.


3. Synthesizing ``overlay vm``  
    Here, we'll show 3 ways to perform VM synthesis using ``overlay vm``; 1) verifying synthesis using command line interface, 2) synthesize over network using desktop client, and 3) synthesize over network using Android client.  

    1) Command line interface: You can resume your ``overlay vm`` using 

        > $ cd ./bin
        > $ ./cloudlet synthesis /path/to/base.image /path/to/overlay-meta
    
    2) network client(desktop client)  
    You first need to change configuration file to inform ``base vms'``
  information to synthesis server. It is basically json formatted file and it
  should have ``name``, ``sha256``, ``path`` keys for each ``base vm`` (Please
  see the example configuration file at bin directory). ``sha256`` represents
  hash value of the ``base VM`` and you can find it at ``*.base-img-hash`` file
  in ``base vm`` directory. For ``path``, you can fill a path to ``base disk``
  of the ``base vm``.
  
        > $ cd ./bin
        > % Run server
        > $ ./synthesis -c config.json    
    
        > % Run client (at different machine or different terminal)
        > $ ./rapid_client.py -s [cloudlet ip address] -o [/path/to/overlay-meta]

    
    3) network client(Android client)  
        > TO BE WRITTEN



Compiling external library that Cloudlet uses
----------------------------------------------

You will need:

* qemu-kvm 1.1.1 (for Free memory and TRIM support)
* libc6-dev-i386 (for Free memory support)
