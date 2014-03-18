Cloudlet: Infrastructure for Mobile Computing
========================================================

A cloudlet is a new architectural element that arises from the convergence of
mobile computing and cloud computing. It represents the middle tier of a
3-tier hierarchy:  mobile device - cloudlet - cloud.   A cloudlet can be
viewed as a "data center in a box" whose  goal is to "bring the cloud closer".  
Copyright (C) 2011-2014 Carnegie Mellon University.

**Please visit our website at [Elijah Project](http://elijah.cs.cmu.edu/) for detail project information.**



Before You Start
--------------------------
Please take a look at following two papers to understand our work better.  

1. [The Case for VM-based Cloudlets in Mobile
   Computing](http://www.cs.cmu.edu/~satya/docdir/satya-ieeepvc-cloudlets-2009.pdf):
   a position paper proposing the concept of the cloudlet.
2. [The Impact of Mobile Multimedia Applications on Data Center
   Consolidation](http://www.cs.cmu.edu/~satya/docdir/ha-ic2e2013.pdf):
   experimental results presenting how the cloudlet can make a difference.



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
  │     ├─ Standalone server and client for VM provisioning
  │     └─ Paper: <a href=http://www.cs.cmu.edu/~satya/docdir/ha-mobisys-vmsynthesis-2013.pdf target="_blank">Just-in-Time Provisioning for Cyber Foraging </a>
  ├── <b>elijah-handoff (since v2.0)</b>
  │     ├─ Library for achieving VM handoff across Cloudlets
  │     ├─ Adaptive VM live migration optimized for WAN
  │     └─ Under development (Will be released at 2015 summer).
  │
  ├── <b><a href=https://github.com/cmusatyalab/elijah-discovery-basic target="_blank">elijah-discovery</a> (since v2.0)</b>
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
2. [QuiltView](https://github.com/cmusatyalab/quiltview): A Crowd-Sourced Video Response System
3. [Gabriel](https://github.com/cmusatyalab/gabriel): A Wearable Cognitive Assistance System



Publications
--------------------------

Recent publications are at [http://elijah.cs.cmu.edu/publications.html](http://elijah.cs.cmu.edu/publications.html)
