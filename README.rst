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
     $ sudo apt-get install qemu-kvm libvirt-bin gvncviewer python-libvirt python-xdelta3 python-dev liblzma-dev apparmor-utils
     $ sudo pip install msgpack-python bson pyliblzma

2. Disable security module.
   Example at Ubuntu 12::
     $ sudo aa-complain /usr/sbin/libvirtd``


How to use
--------------
1. Creating new ``base vm``
2. Creating new ``overlay vm`` from ``base vm``
3. Synthesizing ``overlay vm``


Compiling external library that Cloudlet uses
----------------------------------------------
You will need:
 * qemu-kvm 1.1.1 (for Free memory and TRIM support)
 * libc6-dev-i386 (for Free memory support)
