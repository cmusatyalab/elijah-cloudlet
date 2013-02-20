/*
 * Elijah: Cloudlet Infrastructure for Mobile Computing
 * Copyright (C) 2011-2012 Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * LICENSE.GPL.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 *      Author: Kiryong Ha (krha@cmu.edu)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>		// getaddrinfo	
#include <sys/socket.h>		// getaddrinfo	
#include <netdb.h>			// getaddrinfo	
#include <msgpack.h>        // message pack

#include "api.h"
#include "protocol.h"



/*
 * utility methods
 */
static cloudlet_t* init_cloudlet_t() {
	cloudlet_t *cloudlet = (cloudlet_t*)malloc(sizeof(struct cloudlet_t)*sizeof(char));
	memset(cloudlet->ip_v4, 0, sizeof(cloudlet->ip_v4));
	cloudlet->port_number = CLOUDLET_PORT;
	cloudlet->next = NULL;
	return cloudlet;
}

static void delete_cloudlet_t(cloudlet_t *cloudlet){
}

int static connect_server(char const *server_ip, int port_number){
    int sockfd = 0, n = 0;
    struct sockaddr_in serv_addr; 
    memset(&serv_addr, '0', sizeof(serv_addr)); 
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        strcpy(discovery_error, "Could not create socket");
        return RET_FAILED;
    } 
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port_number); 
    if(inet_pton(AF_INET, server_ip, &serv_addr.sin_addr)<=0) {
        strcpy(discovery_error, "inet_pton error occured");
        return RET_FAILED;
    } 
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        strcpy(discovery_error, "Connection failed");
        return RET_FAILED;
    } 

    return sockfd;
}

void print_cloudlet_t(cloudlet_t *cloudlet){
	cloudlet_resource_t *resource = (cloudlet_resource_t *)(&cloudlet->hw_resource);
	fprintf(stdout, "ip: %s, port: %d\n", cloudlet->ip_v4, cloudlet->port_number);
	fprintf(stdout, " - number_cpu: %d, cpu_clock_speed_mhz: %f\n\
			- mem_total_mb: %d, mem_free_mb: %d \n\
			- cpu_usage_percent: %f\n",\
			resource->number_cpu, resource->cpu_clock_speed_mhz,
			resource->mem_total_mb, resource->mem_free_mb,
			resource->cpu_usage_percent);
	return;
}

void print_cloudlets(cloudlet_t *cloudlet_list){
	int count = 0;
	cloudlet_t *cloudlet = cloudlet_list;
	while(cloudlet){
		count++;
		fprintf(stdout, "%d : ", count);
		print_cloudlet_t(cloudlet_list);
		cloudlet = cloudlet->next;
	}
}

/*
 * Read exact amount of data from fd
 */
int read_full(int fd, void *buffer, int count) {
	int got, done;
	done = 0;
	char *buf = buffer;
	while (count > 0) {
		got = read(fd, buf, count);
		if (got == 0)
			return done;
		if (got < 0)
			return -1;
		done += got;
		count -= got;
		buf = (char*) buf + got;
	}
	return done;
}

/*
 * Write exact amount of data at fd
 */
int write_full(int fd, void *buffer, int count) {
	int got, done;
	done = 0;
	char *buf = buffer;
	while (count > 0) {
		got = send(fd, buf, count, MSG_WAITALL);
		if (got == 0)
			return 0;
		if (got < 0) {
			return -1;
		}
		done += got;
		count -= got;
		buf = (char*) buf + got;
	}
	return done;
}

int endian_swap_int(int data) {
    int ret =  ((data>>24) & 0x000000FF) |((data<<8) & 0x00FF0000) |((data>>8) & 0x0000FF00) | ((data<<24) & 0xFF000000);
    return ret;
}

/*
 * public methods
 */
int find_nearby_cloudlets(cloudlet_t **cloudlets, int *size){
    // Step 1. Find several candidate using DNS
    // :param cloudlet_list_ret: cloudlet_t objects will be returned at this list
    // :type cloudlet_list_ret: list
    // :return: success/fail
    // :rtype: int
    //
    int cloudlet_count = 0;
	cloudlet_t *prev_cloudlet = NULL;
	cloudlet_t *head = NULL;
    int i = 0;

	char addr_str[64];
	void *ptr;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;
	struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;
	ssize_t nread;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_DGRAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
	hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

	s = getaddrinfo(CLOUDLET_DOMAIN, NULL, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	while (result){
		ptr = &((struct sockaddr_in *) result->ai_addr)->sin_addr;
		inet_ntop (result->ai_family, ptr, addr_str, 100);
		result = result->ai_next;

		// add new cloudlet
		cloudlet_t *new_cloudlet = init_cloudlet_t();
		memcpy(new_cloudlet->ip_v4, addr_str, strlen(addr_str));
		if (prev_cloudlet == NULL){
			head = new_cloudlet;
			prev_cloudlet = new_cloudlet;
		}else{
			prev_cloudlet->next = new_cloudlet;
			prev_cloudlet = new_cloudlet;
		}
		cloudlet_count++;
	}
	freeaddrinfo(result);

    *cloudlets = head;
    *size = cloudlet_count;
	return RET_SUCCESS;
}

static int parse_resource_info(cloudlet_t *cloudlet, msgpack_object o){
    if(o.via.map.size != 0) {
        msgpack_object_kv* p = o.via.map.ptr;
        msgpack_object_kv* const pend = o.via.map.ptr + o.via.map.size;
        while (1){

        	int key_size = p->key.via.raw.size;
            char *key = (char *)malloc(sizeof(char)*key_size+1);
            memset(key, 0, key_size+1);
            memcpy(key, p->key.via.raw.ptr, key_size);
            if (strncmp(key, MACHINE_NUMBER_TOTAL_CPU, key_size) == 0){
                int value = p->val.via.i64;
                cloudlet->hw_resource.number_cpu = value;
            } else if(strncmp(key, MACHINE_CLOCK_SPEED, key_size) == 0){
                float value = p->val.via.dec;
                cloudlet->hw_resource.cpu_clock_speed_mhz = value;
            } else if(strncmp(key, MACHINE_MEM_TOTAL, key_size) == 0){
                int value = p->val.via.i64;
                cloudlet->hw_resource.mem_total_mb;
            } else if(strncmp(key, TOTAL_CPU_USE_PERCENT, key_size) == 0){
                float value = p->val.via.dec;
                cloudlet->hw_resource.cpu_usage_percent = value;
            } else if(strncmp(key, TOTAL_FREE_MEMORY, key_size) == 0){
                int value = p->val.via.i64;
                cloudlet->hw_resource.mem_free_mb;
            }
            free(key);

            ++p;
            if (p >= pend)
                break;
        }
    }else{
    }

}

int get_cloudlet_info(cloudlet_t *cloudlet){
    // Step 1. fill out cloudlet_t field with more detailed information
    // :param cloudlet_t: cloudlet_t instance that has ip_address of the cloudlet
    // :type cloudlet_t: cloudlet_t
    // :return: success/fail
    // :rtype: int
    memset(discovery_error, 0, sizeof(discovery_error));

    // Connect socket
    int sockfd = connect_server(cloudlet->ip_v4, cloudlet->port_number);
    if (sockfd == RET_FAILED){
        return RET_FAILED;
    }

	// pack request message
	msgpack_sbuffer* buffer = msgpack_sbuffer_new();
	msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);
	msgpack_pack_map(pk, 1);
	int len_command_key = strlen(KEY_COMMAND);
	msgpack_pack_raw(pk, len_command_key);
	msgpack_pack_raw_body(pk, KEY_COMMAND, len_command_key);
	msgpack_pack_int32(pk, MESSAGE_COMMAND_GET_RESOURCE_INFO);

	// send message
	int header_size = endian_swap_int(buffer->size);
	if (write_full(sockfd, &header_size, 4) != 4){
	    strcpy(discovery_error, "Cannot write to socket");
	    return RET_FAILED;
    }
	int sent_size = write_full(sockfd, buffer->data, buffer->size);
	if (sent_size != buffer->size){
	    strcpy(discovery_error, "Cannot write to socket");
	    return RET_FAILED;
    }

    // read message
    int read_size = read_full(sockfd, &header_size, 4);
    header_size = endian_swap_int(header_size);
    unsigned char *read_buffer = (unsigned char*)malloc(sizeof(char)*header_size);
    read_size = read_full(sockfd, read_buffer, header_size);
    if (header_size != read_size){
	    strcpy(discovery_error, "Cannot read from socket");
	    return RET_FAILED;
    }

	// deserializes it
	msgpack_unpacked msg;
	msgpack_unpacked_init(&msg);
	bool success = msgpack_unpack_next(&msg, read_buffer, header_size, NULL);

	// deserialized object
	bool is_error = false;
	msgpack_object o = msg.data;
    if(o.via.map.size != 0) {
        msgpack_object_kv* p = o.via.map.ptr;
        msgpack_object_kv* const pend = o.via.map.ptr + o.via.map.size;
        while (1){
        	int key_size = (p->key.via.raw.size);
            char *key = (char *)malloc(sizeof(char)*key_size+1);
            key[p->key.via.raw.size] = '\0';
            memcpy(key, p->key.via.raw.ptr, key_size);
            if (strncmp(key, KEY_COMMAND, key_size) == 0){
                int value = p->val.via.i64;
                if (value == MESSAGE_COMMAND_SUCCESS){
                    is_error = false;
                }else{
                    is_error = true;
                }
            } else if(strncmp(key, KEY_PAYLOAD, key_size) == 0){
                char *value = (char *)malloc(sizeof(char) * (p->val.via.raw.size));
                memcpy(value, p->val.via.raw.ptr, p->val.via.raw.size);
                parse_resource_info(cloudlet, p->val);
                free(value);
            } else if(strncmp(key, KEY_ERROR, key_size) == 0){
                char *value = (char *)malloc(sizeof(char) * (p->val.via.raw.size));
                memcpy(value, p->val.via.raw.ptr, p->val.via.raw.size);
                memcpy(discovery_error, value, strlen(value));
                free(value);
            }
            free(key);

            ++p;
            if (p >= pend)
                break;
        }
    }

	// cleaning
	msgpack_sbuffer_free(buffer);
	msgpack_packer_free(pk);

    if (is_error){
        return RET_FAILED;
    }else{
        return RET_SUCCESS;
    }

}


long associate_with_cloudlet(cloudlet_t *cloudlet){
    // Step 3. Associate with given cloudlet
    // :param cloudlet_t: cloudlet_t instance that has ip_address of the cloudlet
    // :type cloudlet_t: cloudlet_t
    // :return: session id or -1 if it failed
    // :rtype: long
	return 0L;
}


int disassociate(cloudlet_t *cloudlet, long session_id){
	// Step 4. disassociate with given cloudlet
	// :param session_id: session_id that was returned when associated
	// :type session_id: long
	// :return: N/A
	return RET_FAILED;
}

void _test(){
	/* creates buffer and serializer instance. */
	msgpack_sbuffer* buffer = msgpack_sbuffer_new();
	msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

	/* serializes ["Command": "GET_RESOURCE_INFO"]. */
	/* https://github.com/msgpack/msgpack-c/blob/master/src/msgpack/pack.h */
	msgpack_pack_map(pk, 1);
	int len_command_key = strlen(KEY_COMMAND);
	msgpack_pack_raw(pk, len_command_key);
	msgpack_pack_raw_body(pk, KEY_COMMAND, len_command_key);
	msgpack_pack_int32(pk, MESSAGE_COMMAND_GET_RESOURCE_INFO);

	/* deserializes it. */
	msgpack_unpacked msg;
	msgpack_unpacked_init(&msg);
	bool success = msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL);

	/* prints the deserialized object. */
	msgpack_object obj = msg.data;
	msgpack_object_print(stdout, obj);  /*=> ["Hello", "MessagePack"] */
	fprintf(stdout,"\n\n");

	/* cleaning */
	msgpack_sbuffer_free(buffer);
	msgpack_packer_free(pk);
}


int main(int argc, char **argv){
	//_test();
	int i = 0;
	cloudlet_t *cloudlet_list;
	int size = 0;

    fprintf(stdout, "Step 1.\n");
	int ret = find_nearby_cloudlets(&cloudlet_list, &size);
	if (ret == RET_SUCCESS){
		print_cloudlets(cloudlet_list);
	}

    fprintf(stdout, "\nStep 2.\n");
	cloudlet_t *cloudlet = cloudlet_list;
	while(cloudlet){
		int ret = get_cloudlet_info(cloudlet);
		if (ret == RET_SUCCESS){
		    print_cloudlet_t(cloudlet);
        }else {
            fprintf(stdout, "Failed to get Cloudlet information from %s :\n", cloudlet->ip_v4);
            fprintf(stdout, "%s\n", discovery_error);
        }
		cloudlet = cloudlet->next;
	}
	return 0;
}
