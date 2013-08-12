from __future__ import with_statement

import sys
from fabric.api import env
from fabric.api import run
from fabric.api import local
from fabric.api import sudo
from fabric.api import task
from fabric.api import abort


def check_support():
    if run("egrep '^flags.*(vmx|svm)' /proc/cpuinfo > /dev/null").failed:
        abort("Need hardware VM support (vmx)")


def disable_EPT():
    if run("egrep '^flags.*(ept)' /proc/cpuinfo > /dev/null").failed:
        return
    else:
        # disable EPT
        sudo('modprobe -r kvm_intel')
        sudo('modprobe kvm_intel "ept=0"')
    pass


@task
def localhost():
    env.run = local
    env.warn_only = True
    env.hosts = ['localhost']


@task
def remote():
    env.run = run
    env.warn_only = True
    env.hosts = ['some.remote.host']

@task
def install():
    check_support()

    # install dependent package
    sudo("apt-get update")
    if sudo("apt-get install -y qemu-kvm libvirt-bin gvncviewer " +
            "python-libvirt python-xdelta3 python-dev openjdk-6-jre  " +
            "liblzma-dev apparmor-utils libc6-i386 python-pip").failed:
        abort("Failed to install libraries")
    if sudo("pip install bson pyliblzma psutil sqlalchemy").failed:
        abort("Failed to install python libraries")

    # disable libvirtd from appArmor to enable custom KVM
    if sudo("aa-complain /usr/sbin/libvirtd").failed:
        abort("Failed to disable AppArmor for custom KVM")

    # add current user to groups (optional)
    username = env.get('user')
    if sudo("adduser %s kvm" % username).failed:
        abort("Cannot add user to kvm group")
    if sudo("adduser %s libvirtd" % username).failed:
        abort("Cannot add user to libvirtd group")

    # Make sure to have fuse support
    # qemu-kvm changes the permission of /dev/fuse, so we revert back the
    # permission. This bug is fixed from udev-175-0ubuntu26
    # Please see https://bugs.launchpad.net/ubuntu/+source/udev/+bug/1152718
    if sudo("chmod 1666 /dev/fuse").failed:
        abort("Failed to enable fuse for the user")
    if sudo("chmod 644 /etc/fuse.conf").failed:
        abort("Failed to change permission of fuse configuration")
    if sudo("sed -i 's/#user_allow_other/user_allow_other/g' /etc/fuse.conf"):
        abort("Failed to allow other user to access FUSE file")

    # (Optional) disable EPT support
    # When you use EPT support with FUSE+mmap, it randomly causes kernel panic.
    # We're investigating it whether it's Linux kernel bug or not.
    disable_EPT()
    sys.stdout.write("[SUCCESS] VM synthesis code is installed\n")
