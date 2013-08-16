from __future__ import with_statement
import os

from fabric.api import *
from fabric.operations import *
from fabric.contrib import *
from fabric.context_managers import cd

# constant
VM_SYNTEHSIS_REPO = "https://github.com/cmusatyalab/elijah-cloudlet.git"
PYTHON_LIBRARY_ROOT = "/usr/lib/python2.7/dist-packages"
NOVA_CONF_PATH = "/etc/nova/nova.conf"
NOVA_COMPUTE_CONF_PATH = "/etc/nova/nova-compute.conf"
HORIZON_PATH = "/usr/share/openstack-dashboard/openstack_dashboard/dashboards/project"
HOROZIN_API_PATH = "/usr/share/openstack-dashboard/openstack_dashboard/api"

def deploy_cloudlet_api():
    ext_file = os.path.abspath("./api/cloudlet.py")
    api_file = os.path.abspath("./api/cloudlet_api.py")
    ext_lib_dir = os.path.join(PYTHON_LIBRARY_ROOT, "nova/api/openstack/compute/contrib/")
    api_lib_dir = os.path.join(PYTHON_LIBRARY_ROOT, "nova/compute/")

    deploy_files = [ 
            (ext_file, ext_lib_dir), 
            (api_file, api_lib_dir), 
            ]

    # deploy files
    # TODO: use python package installer.
    for (src_file, target_dir) in deploy_files:
        dest_filepath = os.path.join(target_dir, os.path.basename(src_file))
        if put(src_file, dest_filepath, use_sudo=True, mode=0644).failed:
            abort("Cannot copy %s to %s" % (src_file, lib_dir))

    sudo("service nova-api restart", shell=False)


def _replace_compute_manager(nova_conf_path):
    """ replace or insert new compute manager configuration
    Cannot use append method since compute_manager option does not effective
    at last line (probably nova configuration bug).
    """
    compute_manager = "nova.compute.cloudlet_manager.CloudletComputeManager"
    conf_content = sudo("cat %s" % nova_conf_path)
    new_config = list()

    # replace if compute_manager option is exist
    is_replaced = False
    for oneline in conf_content.split("\n"):
        if oneline.strip().startswith("compute_manager") == True:
            current_manager = oneline.strip().split("=")[-1].strip()
            if current_manager == compute_manager:
                return
            else:
                new_config.append("compute_manager=%s" % compute_manager)
                is_replaced = True
        else:
            new_config.append(oneline.replace("\r", ""))
    
    if is_replaced == False:
        # insert it at first line
        new_config.insert(1, "compute_manager=%s" % compute_manager)
    temp_file_name = "nova-tmp"
    open(temp_file_name, "w+").write('\n'.join(new_config))
    files.upload_template(temp_file_name, nova_conf_path, use_sudo=True)
    os.remove(temp_file_name)


def deploy_compute_manager():
    global NOVA_CONF_PATH
    global NOVA_COMPUTE_CONF_PATH

    manager_file = os.path.abspath("./compute/cloudlet_manager.py")
    manager_lib_dir = os.path.join(PYTHON_LIBRARY_ROOT, "nova/compute/")
    libvirt_driver = os.path.abspath("./compute/cloudlet_driver.py")
    libvirt_driver_dir = os.path.join(PYTHON_LIBRARY_ROOT, "nova/virt/libvirt/")

    deploy_files = [ 
            (manager_file, manager_lib_dir),
            (libvirt_driver, libvirt_driver_dir),
            ]

    # use custom compute manager inherited from nova-compute manager
    if files.exists(NOVA_CONF_PATH, use_sudo=True) == False:
        abort("Cannot find nova-compute conf file at %s\n" % NOVA_CONF_PATH)
    _replace_compute_manager(NOVA_CONF_PATH)

    # use custom driver inherited from libvitDriver
    command = "sed -i 's/compute_driver=libvirt.LibvirtDriver/compute_driver=libvirt.cloudlet_driver.CloudletDriver/g' %s"\
            % (NOVA_CONF_PATH)
    sudo(command)
    if files.exists(NOVA_COMPUTE_CONF_PATH, use_sudo=True) == True:
        command = "sed -i 's/compute_driver=libvirt.LibvirtDriver/compute_driver=libvirt.cloudlet_driver.CloudletDriver/g' %s"\
                % (NOVA_COMPUTE_CONF_PATH)
        sudo(command)

    # copy files
    for (src_file, target_dir) in deploy_files:
        dest_filepath = os.path.join(target_dir, os.path.basename(src_file))
        if put(src_file, dest_filepath, use_sudo=True, mode=0644).failed:
            abort("Cannot copy %s to %s" % (src_file, lib_dir))

    sudo("service nova-compute restart", shell=False)


def check_system_requirement():
    msg = "Tested only Ubuntu 12.04 LTS\n"
    msg += "But the current OS is not Ubuntu 12.04 LTS"

    output = run('cat /etc/lsb-release')
    if output.failed == True:
        abort(msg)
    if str(output).find("DISTRIB_RELEASE=12.04") == -1:
        abort(msg)


def check_VM_synthesis_package():
    cloudlet_temp_repo = '/tmp/cloudlet_repo_temp'
    sudo("apt-get update")
    sudo("apt-get install git openssh-server fabric")
    run("git clone %s -o %s" % (VM_SYNTEHSIS_REPO, cloudlet_temp_repo))
    with cd(cloudlet_temp_repo):
        if sudo("fab localhost install").failed:
            msg = "Cannot install cloudlet package.\n"
            msg += "Manually install it downloading from %s" % VM_SYNTEHSIS_REPO
            abort(msg)

    if run("cloudlet --version").failed:
        abort("Cannot find cloudlet package.\nInstall Cloudlet from %s" % VM_SYNTEHSIS_REPO)
        

def deploy_dashboard():
    global HORIZON_PATH
    global HOROZIN_API_PATH

    # deploy files
    src_dir = os.path.abspath("./dashboard")
    dest_dir = os.path.join(HORIZON_PATH, "cloudlet/")
    if files.exists(dest_dir, use_sudo=True) == False:
        sudo("mkdir -p %s" % dest_dir)
    if put(src_dir, dest_dir, use_sudo=True).failed:
        abort("Cannot create symbolic link from %s to %s" % (src_dir, link_dir))


def deploy_svirt():
    # check apparmor file
    libvirt_svirt_file = "/etc/apparmor.d/abstractions/libvirt-qemu"
    if files.exists(libvirt_svirt_file, use_sudo=True) == False:
        abort("This system does not have libvirt profile for svirt")

    # append additional files that cloudlet uses
    security_rule = open("./svirt-profile", "r").read()
    if files.append(libvirt_svirt_file, security_rule, use_sudo=True) == False:
        abort("Cannot add security profile to libvirt-qemu")


@task
def localhost():
    env.run = local
    env.warn_only = True
    env.hosts = ['localhost']


@task
def remote():
    env.run = run
    env.warn_only = True
    env.user = 'krha'
    env.hosts = ['sleet.elijah.cs.cmu.edu']


@task
def install_control():
    with hide('stdout'):
        check_system_requirement()
        check_VM_synthesis_package()
        deploy_cloudlet_api()
        deploy_compute_manager()
        #deploy_dashboard()
        deploy_svirt()


@task
def install_compute():
    with hide('stdout'):
        check_system_requirement()
        check_VM_synthesis_package()
        deploy_cloudlet_api()
        deploy_compute_manager()
        deploy_svirt()

