OpenStack extension for Cloudlet support
========================================================
A cloudlet is a new architectural element that arises from the convergence of
mobile computing and cloud computing. It represents the middle tier of a
3-tier hierarchy:  mobile device - cloudlet - cloud.   A cloudlet can be
viewed as a "data center in a box" whose  goal is to "bring the cloud closer".

Copyright (C) 2012-2013 Carnegie Mellon University
This is a developing project and some features might not be stable yet.
Please visit our website at [Elijah page](http://elijah.cs.cmu.edu/).



License
----------

All source code, documentation, and related artifacts associated with the
cloudlet open source project are licensed under the [Apache License, Version
2.0](http://www.apache.org/licenses/LICENSE-2.0.html).



Prerequisites
-------------------------

1. OpenStack installation: This work assumes that you already have working OpenStack.

2. You need extra IP address to allocate IP to the synthesized VMs. Otherwise,
the mobile device cannot access to the synthesized VM.

3. We have only tested the system with Ubuntu 12.04 LTS 64bit platform.



Installing
----------

You will need
* cloudlet


To install, you can run fabric script as below.

	> $ sudo apt-get install git openssh-server fabric
	> $ fab localhost install_control
	> $ fab localhost install_compute


Known Issues
----------

1. Possible resource leak from unexpected OpenStack termination

2. __Early start optimization__ is turned off
	- Early start optimization splits VM overlay into multiple segments (files) 
	- Need better packaging for VM overlay to handle segments

