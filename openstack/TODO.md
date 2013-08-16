TODO
----------

- Dashboard

- **Done**: Security confinement: Confine two level security module in AppArmor
- **Done**: Graceful terminatation of synthesized VM
- **Done**: Multinode test
- **Done**: Graceful termination at Creating Overlay: Saving memory snapshot requires
termination of VM. But this termination is outside of OpenStack logic
- **Done**:Remove unnecessary steps for synthesis: 
  	+ Injecting metadata/key/password into image
- **Done**: Handle FUSE O_Direct problem
- **Done**: change qmeu connection depend on usage. OpenStack need qemu:///system for bridge configuration
	Error messsage
		>> .. XML ..
		>> </domain>
		Error, make sure previous VM is closed and check QEMU_ARGUMENT

- turn off early start optimization:
  	+ need better packaging for overlay composed of multiple segments (files)
  	+ uncommon threading model in OpenStack

- Start Base VM with special XML for compatibility across different machine


