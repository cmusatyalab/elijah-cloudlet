#include "client_manager.h"
#include "../lib/lib_socket.h"
#include "../lib/lib_type.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <stdio.h>
#include <sys/wait.h>
#include <pthread.h>

#include <json/json.h>


static pthread_t client_manager_thread;
static pthread_t client_data_handler;
void *start_client_handler(void *arg); //socket connection thread
void *start_client_manager(void *arg); // client handler thread

typedef struct {
	int sock_fd;
	char ip_address[16];
}__attribute__((packed)) TCPClient;

//array for managing client
static TCPClient clients[MAX_CLIENT_NUMBER];

/*
 * Client Thread Conroller
 */
pthread_mutex_t client_mutex;
static int client_lock;
static fd_set clients_fdset;
static void init_client(int client_handler);
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


#pragma mark PUBLIC_METHOD
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

		PRINT_OUT("[%d] Client Manager Accepted new Client.\n", i);
		strcpy(clients[i].ip_address, inet_ntoa(accepted_addr.sin_addr));
		clients[i].sock_fd = accepted_sock;
		FD_SET(accepted_sock, &clients_fdset);
	}
}




#pragma mark PRIVATE_METHOD

// init(or reset) client socket connection
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
	unsigned char* json_byte;
	int json_max_size = MAX_JSON_SIZE;
	int result;
	int i, j;

	json_byte  = (unsigned char*)malloc(json_max_size * sizeof(char));

	FD_ZERO(&clients_fdset);
	FD_ZERO(&temp_fdset);

	while (1) {
		//waiting time
		timeout.tv_sec = 0; timeout.tv_usec = 10000;
		waiting_lock();

		temp_fdset = clients_fdset;
		result = select(FD_SETSIZE, &temp_fdset, (fd_set *) NULL, (fd_set *) NULL, &timeout);
		if (result == 0) {
			usleep(100);
			// time-out
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

			//PRINT_OUT("[%d] New Data is coming\n", i);
			char buffer[1];
			result = recv(clients[i].sock_fd, buffer, 1, MSG_WAITALL);
			printf("%s", buffer);

			/*
			memset(&client_msg, '\0', sizeof(client_msg));
			result = recv(clients[i].sock_fd, &client_msg, 2 * sizeof(int), MSG_WAITALL);
			if (result <= 0) {
				PRINT_OUT("[%d] Closed Client.\n", i);
				init_client(i);
				unlock();
				continue;
			}

			// check buffer size for json
			client_msg.payload_length = endian_swap_int(client_msg.payload_length);
			PRINT_OUT("[%d] Data Size : %d\n", i, client_msg.payload_length);
			if(client_msg.payload_length > json_max_size){
				json_max_size = sizeof(char) * client_msg.payload_length;
				json_byte = (char*)realloc(json_byte, sizeof(char) * json_max_size);
				if(!json_byte){
					PRINT_ERR("[%d] Cannot Allocate %d Memory.\n", i, json_max_size);
					exit(-1);
				}
			}

			// read pyaload
			recv(clients[i].sock_fd, json_byte, client_msg.payload_length, MSG_WAITALL);
			PRINT_OUT("[%d] All Data Received\n", i);

			// parse JSON
			 */

		}
	}

	free(json_byte);
	return NULL;
}

static int python_exec() {
	FILE *fp;
	int status;
	char path[1035];

	/* Open the command for reading. */
	fp= popen("~/Cloudlet/src/Script/cloudet.py -o ~/Cloudlet/image/baseVM/ubuntu_base.qcow2 ~/Cloudlet/image/baseVM/ubuntu_base.mem","r");
	if (fp == NULL) {
		printf("Failed to run command\n");
		return -1;
	}
	wait(&status);
	printf("********* return1\n");

	/* Read the output a line at a time - output it. */
	while (fgets(path, sizeof(path) - 1, fp) != NULL) {
		printf("%s", path);
	}
	printf("********* return2\n");

	/* close */
	pclose(fp);

	return 0;
}




int test_JSON() {
	char * string = "{\"name\" : \"joys of programming\"}";
	json_object * jobj = json_tokener_parse(string);
	enum json_type type = json_object_get_type(jobj);
	printf("type: %d", type);
	switch (type) {
	case json_type_null:
		printf("json_type_null\n");
		break;
	case json_type_boolean:
		printf("json_type_boolean\n");
		break;
	case json_type_double:
		printf("json_type_double\n");
		break;
	case json_type_int:
		printf("json_type_int\n");
		break;
	case json_type_object:
		printf("json_type_object\n");
		break;
	case json_type_array:
		printf("json_type_array\n");
		break;
	case json_type_string:
		printf("json_type_string\n");
		break;
	}
}
