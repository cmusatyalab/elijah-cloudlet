Cloudle Protocol
====================
This document explains network protocol for VM Synthesis between Cloudlet and
mobile client. Here we currently used *simplified version* of entire protocol
excluding discovery, resource negociation, atn etc.



Introduction
-------------

This communication protocol in high level simply transfer ``overlay meta`` file and
``overlay VM files`` over the network.  We tried to be independent of programming
language and operating platform, and thus we used
[Messsage Pack]("http://msgpack.org/") as a data communication format.



Protocol (at Client)
------------

1. Client send ``overlay meta`` file to Cloudlet
```python
	sock.sendall(struct.pack("!I", len(header)))	# send header size as unsigned int (4 bytes)
	sock.sendall(header)							# send header data
```

2. Wait for server-side message
	Server will send ``message-pack`` formatted message for each command.
	Each command will be composed of 
	
	* message size (4 bytes unsigned int)
	* message formatted as ``message-pack`` (variable length).
	
	```python
	message_size, = struct.unpack("!I", sock.recv(4))	# read 4 bytes and unpack it as unsigned int
	message = recv_all(sock, message_size);				# read message
	```

	* You can read command type from 'command' key. 
	```json
	{
		'command': 1
	}
	```

	1) SUCCESS (``0x01``)
		* Server successfully reconstruct VM. 
		* You can start own application at this point. For example,
		  if you have launched Face recognition VM, then you can start
		  face recognition client at mobile device at this point.

	2) Failed (``0x02``)
		* Server failed to synthesizing VM.
		* You can find detail reasons using 'reasons' key from message

	2) On-demand overlay segmenet fetching (``0x03``)
		* ``overlay-meta`` contains list of the ``overlay files`` and Cloudlet
		  will ask one of the for each request.
		* You can get the requested file name from 'blob_url' key and
		  need to simply send binary file of that overlay file to the server.
		 

		  


