
#ifndef __CLIENT_MANAGER_H_INCLUDED__
#define __CLIENT_MANAGER_H_INCLUDED__

#define TCP_SERVER_PORT			9090	// TCP port number for client connection
#define MAX_CLIENT_NUMBER		20 		// Maximum concurrent number of client
#define MAX_JSON_SIZE			1024	// JSON Size in byte

int init_client_manager();	// initialization method for client manager thread
int end_client_manager();	// tear down method for client manager


#endif  //  __CLIENT_MANAGER_H_INCLUDED__

