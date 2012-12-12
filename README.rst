Elijah: Cloudlet Infrastructure for Mobile Computing
Copyright (C) 2011-2012 Carnegie Mellon University

This is a developing project and some features might not be stable yet.
Please visit our website at <http://elijah.cs.cmu.edu/>.

INSTALL
-----------
qemu-kvm
libvirt-bin
gvncviewer
python-libvirt
python-xdelta3
python-dev (for message pack)
liblzma-dev (for pyliblzma)
apparmor-utils (for disable apparmor for libvirt)

python library
	- msgpack-python
	- bson
	- pyliblzma

disable apparmor
$ sudo aa-complain /usr/sbin/libvirtd

ADDITIONAL PACKAGE FOR COMPILIATION
-----------------------------
libc6-dev-i386 (for Free memory support)
