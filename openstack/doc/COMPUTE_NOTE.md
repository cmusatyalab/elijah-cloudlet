install
--------
- NTP update
  	> $ sudo apt-get install -y ntp

- Add keyring to repo
  	> add "deb http://ubuntu-cloud.archive.canonical.com/ubuntu precise-updates/grizzly main"
  	> to /etc/apt/sources.list.d/grizzly.list 
  	>
  	> $ sudo apt-get update && apt-get upgrade

- install nova-compute and nova-network
  	> $ sudo apt-get install nova-compute nova-network
