from __future__ import with_statement

from fabric.api import env
from fabric.api import run
from fabric.api import local
from fabric.api import sudo
from fabric.api import task


@task
def localhost():
    env.run = local
    env.hosts = ['localhost']

@task
def remote():
    env.run = run
    env.hosts  = ['some.remote.host']


def check_support():
    run("egrep '^flags.*(vmx|svm)' /proc/cpuinfo > /dev/null")


@task
def install():
    check_support()
    # install dependent package
    sudo("apt-get update")
    sudo("apt-get install -y qemu-kvm libvirt-bin gvncviewer python-libvirt python-xdelta3 python-dev openjdk-6-jre liblzma-dev apparmor-utils libc6-i386 python-pip")
    sudo("pip install bson pyliblzma psutil sqlalchemy")

    # disable libvirtd from appArmor to enable custom KVM
    sudo("aa-complain /usr/sbin/libvirtd")

    # add current user to groups (optional)
    username = env.get('user')
    sudo("adduser %s kvm" % username)
    sudo("adduser %s libvirtd" % username)

    # make sure to have fuse support
    sudo("chmod 1666 /dev/fuse")
    sudo("chmod 644 /etc/fuse.conf")

