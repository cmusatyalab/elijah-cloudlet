TIPs
---------
1. rettach nic card

	- hotplug in module : https://help.ubuntu.com/community/ExpressCard
	  pciehp pciehp_force=1 acpiphp at /etc/modules
	- erase filterref element

2. ip assign command

	- nova add-floating-ip [server] [ip]
	- nova floating-ip-list
	- auto assign: auto_assign_floating_ip=True at nova.conf
	  if not enough floating ip error happends, check db. only the ip 
	  that is not assigned to project can be automatically assigned.



Custom Dashboard
----------------

1. Change /etc/openstack-dashboard/local_settings.py to enable customization

		>
		> # Default OpenStack Dashboard configuration.
		> HORIZON_CONFIG = {
		> 	"customization_module": "custom_www.overrides",
		> 	'dashboards': ('project', 'admin', 'settings',), 
		> 	...


