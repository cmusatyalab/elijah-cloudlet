#!/usr/bin/env python 
#
# cloudlet infrastructure for mobile computing
#
#   author: kiryong ha <krha@cmu.edu>
#
#   copyright (c) 2011-2013 carnegie mellon university
#   licensed under the apache license, version 2.0 (the "license");
#   you may not use this file except in compliance with the license.
#   you may obtain a copy of the license at
#
#       http://www.apache.org/licenses/license-2.0
#
#   unless required by applicable law or agreed to in writing, software
#   distributed under the license is distributed on an "as is" basis,
#   without warranties or conditions of any kind, either express or implied.
#   see the license for the specific language governing permissions and
#   limitations under the license.
#

import sys
sys.path.insert(0, "./src/")
from synthesis.Configuration import Const as Const
from setuptools import setup

setup(
        name='cloudlet',
        version=str(Const.VERSION),
        description='Cloudlet for cloudlet computing at the edge',
        long_description=open('README.md', 'r').read(),
        url='https://github.com/cmusatyalab/elijah-cloudlet/',
        author='Kiryong Ha',
        author_email='krha@cmu.edu',
        keywords="cloud cloudlet cmu VM libvirt KVM QEMU virtualization",
        license='Apache 2',
        scripts=['bin/cloudlet', 'bin/synthesis_server'], # TODO: include 'bin/network_client'
        package_dir = {'':'src'},
        packages=['synthesis'],
        install_requires=['bson', 'pyliblzma', 'psutil', 'sqlalchemy'],
        include_package_data=True,
        #classifier=[
        #    'Development Status :: 3 - Alpha',
        #    'License :: OSI Approved :: Apache Software License',
        #    'Operating System :: POSIX :: Linux',
        #],
        )
