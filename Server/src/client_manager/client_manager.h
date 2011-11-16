
#ifndef __CLIENT_MANAGER_H_INCLUDED__
#define __CLIENT_MANAGER_H_INCLUDED__

#define TCP_SERVER_PORT			9090	// TCP port number for client connection
#define MAX_CLIENT_NUMBER		20 		// Maximum concurrent number of client
#define MAX_JSON_SIZE			1024	// JSON Size in byte

#pragma mark PUBLIC_METHOD
/*
 * public_method
 */
int init_client_manager();
int end_client_manager();


#pragma mark DATA_STRUCTURE
/*
 * Data structure
 */
typedef struct Client_MSG{
	int cmd;
	int payload_length;
}Client_Msg;

#endif  //  __CLIENT_MANAGER_H_INCLUDED__

