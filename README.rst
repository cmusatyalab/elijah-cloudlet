Elijah: Cloudlet Infrastructure for Mobile Computing
========================================================
A cloudlet is a new architectural element that arises from the convergence of
mobile computing and cloud computing. It represents the middle tier of a
3-tier hierarchy:  mobile device --- cloudlet --- cloud.   A cloudlet can be
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
   Example at ubuntu 12 LTS x86::
     $ sudo apt-get install qemu-kvm libvirt-bin gvncviewer python-libvirt python-xdelta3 python-dev liblzma-dev apparmor-utils python-pip
     $ sudo pip install msgpack-python bson pyliblzma

2. Disable security module.
   Example at Ubuntu 12::
     $ sudo aa-complain /usr/sbin/libvirtd


How to use
--------------

1. Creating new ``base vm``.
* You will first create ``base vm``, which will be a template for rest of overlay VMs.
  To create ``base vm``, you need regular VM disk image as a raw format.
   ::
   cd ./bin
   ./cloudlet base /path/to/vm.image
* When you close VNC connetion, it will automatically create memory snapshot
  and relevant information for ``base vm``


2. Creating new ``overlay vm`` from ``base vm``.
* Now you can create your customized VM based onpon ``base vm``
  You can modify VM with VNC connection and close it when you're ready.
   ::
   cd ./bin
   ./cloudlet overlay /path/to/vm.image
* Again, it will create ``overlay`` vm at the same directory where ``base vm`` exist.
* You will keep only 1) overlay_blob file and 2) overlay-meta file as your ``overlay vm``


3. Synthesizing ``overlay vm``
* You can resume your ``overlay vm``
  ::
  cd ./bin
  ./cloudlet synthesis /path/to/base.image /path/to/overlay-meta



Compiling external library that Cloudlet uses
----------------------------------------------
You will need:
 * qemu-kvm 1.1.1 (for Free memory and TRIM support)
 * libc6-dev-i386 (for Free memory support)
