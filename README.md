Cloudlet: Infrastructure for Mobile Computing
========================================================

A cloudlet is a new architectural element that arises from the convergence of
mobile computing and cloud computing. It represents the middle tier of a
3-tier hierarchy:  mobile device - cloudlet - cloud.   A cloudlet can be
viewed as a "data center in a box" whose  goal is to "bring the cloud closer".  
Copyright (C) 2011-2014 Carnegie Mellon University.

**Please visit our website at [Elijah Project](http://elijah.cs.cmu.edu/) for detail project information.**



Cloudlet Project Repository
--------------------------
####OpenStack++ and library project####
<pre>
<b><a href=https://github.com/cmusatyalab/elijah-openstack target="_blank">elijah-OpenStack</a>: OpenStack extension for cloudlet</b>
  ├── Code and UI for cloudlet OpenStack extension.
  │   Dependency on other cloudlet projects as below.
  │
  ├── <b><a href=https://github.com/cmusatyalab/elijah-provisioning target="_blank">elijah-provisioning</a> (since v1.0)</b>
  │     ├─ Library for Cloudlet provisioning using VM synthesis
  │     └─ Standalone server and client for VM provisioning
  │
  ├── <b><a href=https://github.com/cmusatyalab/elijah-discovery target="_blank">elijah-discovery</a> (since v2.0)</b>
  │     ├─ Library for registration and Cloudlet query  
  │     │    ├─ Resource monitor
  │     │    ├─ Cache monitor
  │     │    └─ Registration daemon
  │     │
  │     ├─ Client library for discovery
  │     │
  │     └─ Cloud-based discovery server (sources for findcloudlet.org)
  │          ├─ Registration Server
  │          └─ Custom DNS Server
  │
..
</pre>  

####Applications leverging cloudlet####
1. [GigaSight](https://github.com/cmusatyalab/GigaSight): Scalable Crowd-Sourcing of Video from Mobile Devices
2. [QuiltView](https://github.com/cmusatyalab/quiltview) (private*): A Crowd-Sourced Video Response System
3. [Gabriel](https://github.com/cmusatyalab/gabriel) (private*): A Wearable Cognitive Assistance System

*: Private project will be opened to the public as soon as it gets stable. Please cntact Kiryong Ha (krha@cmu.edu), if you want to access earlier.



Publications
--------------------------

Recent publications are at [http://elijah.cs.cmu.edu/publications.html](http://elijah.cs.cmu.edu/publications.html)
