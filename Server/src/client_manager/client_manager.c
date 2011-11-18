#include "client_manager.h"
#include "client_handler.h"
#include "../protocol.h"
#include "../util/json_util.h"
#include "../lib/lib_socket.h"
#include "../lib/lib_type.h"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <json/json.h>

/*
 * Private method definition
 */
static void init_client(int client_handler);
void *start_client_handler(void *arg); //socket connection thread
void *start_client_manager(void *arg); // client handler thread

/*
 * Private data
 */
static pthread_t client_manager_thread;			// network socket accept thread
static pthread_t client_data_handler;			// client data handler thread
typedef struct {
	int sock_fd;
	char ip_address[16];
}__attribute__((packed)) TCPClient;
static TCPClient clients[MAX_CLIENT_NUMBER];	// array structure for client fd

pthread_mutex_t client_mutex;					// client socket mutex
static int client_lock;
static fd_set clients_fdset;

/*
 * Client Thread Lock
 */
static void lock() {
	client_lock = 1;
	pthread_mutex_lock(&client_mutex);
}
static void unlock() {
	pthread_mutex_unlock(&client_mutex);
	client_lock = 0;
}
static void waiting_lock() {
	while (client_lock) {
		sched_yield();
	}
	pthread_mutex_lock(&client_mutex);
}


/*
 * Public method implementation
 */
int init_client_manager(){
	pthread_create(&client_manager_thread, NULL, start_client_manager, NULL);
	return SUCCESS;
}

int end_client_manager(){
	pthread_cancel(client_manager_thread);
	return SUCCESS;
}

void *start_client_manager(void* args){
	struct sockaddr_in accepted_addr;
	int accepted_sock;
	int server_sock;
	int client_len;
	int i;

	// init client structure
	for (i = 0; i < MAX_CLIENT_NUMBER; i++) {
		init_client(i);
	}

	// start client handler
	pthread_create(&client_data_handler, NULL, start_client_handler, NULL);

	// get socket
	int port = TCP_SERVER_PORT;
	server_sock = make_local_tcp_server_socket(&port, MAX_CLIENT_NUMBER);
	if (server_sock == -1) {
		PRINT_ERR("Client Manager Error getting server socket!\n");
		return NULL;
	}

	PRINT_OUT("Client Manager start(%d).\n", port);
	while (1) {
		client_len = sizeof(accepted_addr);
		PRINT_OUT("Client Manager is waiting for Client.\n");
		accepted_sock = accept(server_sock, (struct sockaddr*) &accepted_addr, (socklen_t*) &client_len);

		//Get a empty slot
		for (i = 0; i < MAX_CLIENT_NUMBER; i++) {
			if (clients[i].sock_fd == 0)
				break;
		}

		// Fully Connected
		if (i == MAX_CLIENT_NUMBER) {
			close(accepted_sock);
			PRINT_OUT("Client Socket Full.\n");
			sleep(1 * 1000);
			continue;
		}

		PRINT_OUT("[%d] Client Manager Accepted new Client.\n", clients[i].sock_fd);
		strcpy(clients[i].ip_address, inet_ntoa(accepted_addr.sin_addr));
		clients[i].sock_fd = accepted_sock;
		FD_SET(accepted_sock, &clients_fdset);
	}
}


/*
 * Private method implementation
 */
static void init_client(int client_handler) {
	lock();
	memset(clients[client_handler].ip_address, 0, sizeof(clients[client_handler].ip_address));
	if (clients[client_handler].sock_fd != EMPTY)
		FD_CLR(clients[client_handler].sock_fd, &clients_fdset);
	clients[client_handler].sock_fd = EMPTY;
	unlock();
}


void *start_client_handler(void *arg) {
	struct timeval timeout;
	fd_set temp_fdset;
	Client_Msg client_msg;
	char* json_string;
	int json_max_size = MAX_JSON_SIZE;
	int result;
	int i;

	json_string = (char*) malloc(json_max_size * sizeof(char));

	FD_ZERO(&clients_fdset);
	FD_ZERO(&temp_fdset);

	while (1) {
		//waiting time
		timeout.tv_sec = 0; timeout.tv_usec = 10000;
		waiting_lock();

		temp_fdset = clients_fdset;
		result = select(FD_SETSIZE, &temp_fdset, (fd_set *) NULL, (fd_set *) NULL, &timeout);
		if (result == 0) {
			usleep(100);	// time-out
			unlock();
			continue;
		} else if (result == -1) {
			PRINT_ERR("FD Select Error!\n");
			usleep(100);
			unlock();
			continue;
		}

		//read data from client
		for (i = 0; i < MAX_CLIENT_NUMBER; i++) {
			if (clients[i].sock_fd == 0 || !FD_ISSET(clients[i].sock_fd, &temp_fdset)){
				unlock();
				continue;
			}

			PRINT_OUT("[%d] New Data is coming\n", clients[i].sock_fd);

			memset(&client_msg, '\0', sizeof(client_msg));
			memset(json_string, '\0', json_max_size);
			result = recv(clients[i].sock_fd, &client_msg, 2 * sizeof(int), MSG_WAITALL);
			if (result <= 0) {
				PRINT_OUT("[%d] Closed Client.\n", clients[i].sock_fd);
				init_client(i);
				unlock();
				continue;
			}

			// check buffer size for json
			client_msg.payload_length = endian_swap_int(client_msg.payload_length);
			client_msg.cmd = endian_swap_int(client_msg.cmd);
			PRINT_OUT("[%d] Data Size : %d\n", clients[i].sock_fd, client_msg.payload_length);
			if(client_msg.payload_length > json_max_size){
				json_max_size = sizeof(char) * client_msg.payload_length;
				json_string = (char*)realloc(json_string, sizeof(char) * json_max_size);
				if(!json_string){
					PRINT_ERR("[%d] Cannot Allocate %d Memory.\n", clients[i].sock_fd, json_max_size);
					exit(-1);
				}
			}

			// read payload
			recv(clients[i].sock_fd, json_string, client_msg.payload_length, MSG_WAITALL);

			// protocol version check
			json_object *jobj = json_tokener_parse((const char*)json_string);
			char* protocol_version = json_get_type_value(jobj, JSON_KEY_PROTOCOL_VERSION, json_type_string);
			if(strcasecmp(protocol_version, PROTOCOl_VERSION) != 0){
				PRINT_ERR("[%d] Procotol version is Wrong %s != %s\n", protocol_version, PROTOCOl_VERSION);
				return;
			}
			free(protocol_version);

			// parsing packet
			switch(client_msg.cmd){
				case COMMAND_REQ_VMLIST:
					PRINT_OUT("[%d] COMMAND_REQ_VMLIST\n", i);
					parse_req_vmlist(clients[i].sock_fd, json_string);
					break;
				case COMMAND_REQ_TRANSFER_START:
					PRINT_OUT("[%d] COMMAND_REQ_TRANSFER_START\n", i);
					parse_req_transfer(clients[i].sock_fd, json_string);
					break;
				case COMMAND_REQ_VM_LAUNCH:
					PRINT_OUT("[%d] COMMAND_REQ_VM_LAUNCH\n", i);
					parse_req_launch(clients[i].sock_fd, json_string);
					break;
				case COMMAND_REQ_VM_STOP:
					PRINT_OUT("[%d] COMMAND_REQ_VM_STOP\n", i);
					parse_req_stop(clients[i].sock_fd, json_string);
					break;
				default:
					PRINT_ERR("[%d] Not valid command: %d\n", i, client_msg.cmd);
					break;
			}
		}
	}

	free(json_string);
	return NULL;
}

/*
 * Parse JSON and returns VM_Info data point
 * caller have responsibility for deallocating memory
 */

