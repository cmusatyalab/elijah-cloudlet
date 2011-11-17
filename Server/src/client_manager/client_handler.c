/*
 * client_handler.c
 *
 *  Created on: Nov 16, 2011
 *      Author: krha
 */

#include "client_handler.h"
#include "../util/json_util.h"
#include "../lib/lib_socket.h"

#include <json/json.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 * Handle Commands
 */
void parse_req_vmlist(int sock_fd, const char* json_string){
	// Data
	GPtrArray *VM_array = g_ptr_array_new();
	char* protocol_version;

	json_object *jobj = json_tokener_parse((const char*)json_string);	// Parse JSON
	protocol_version = json_get_type_value(jobj, JSON_KEY_PROTOCOL_VERSION, json_type_string);	// Get Parameters

	// Make VM List From JSON
	int ret_number = json_get_VM_Info(jobj, JSON_KEY_VM, VM_array);
	int i = 0;
	for(i = 0; i < ret_number; i++) {
		VM_Info *vm = (VM_Info *) g_ptr_array_index(VM_array, i);
		print_VM_Info(vm);
	}

	// Free memory
	free(protocol_version);
	delete_VM_Info(VM_array);
	g_ptr_array_free(VM_array, TRUE);
}

void parse_req_transfer(int sock_fd, const char* json_string){
	// Data
	GPtrArray *VM_array = g_ptr_array_new();
	json_object *jobj = json_tokener_parse((const char*)json_string);	// Parse JSON

	// Make VM List From JSON
	int ret_number = json_get_VM_Info(jobj, JSON_KEY_VM, VM_array);
	int i = 0;
	for(i = 0; i < ret_number; i++) {
		VM_Info *vm = (VM_Info *) g_ptr_array_index(VM_array, i);
		print_VM_Info(vm);
	}

	// Free memory
	delete_VM_Info(VM_array);
	g_ptr_array_free(VM_array, TRUE);

}

void parse_req_launch(int sock_fd, const char* json_string){
	// Data
	GPtrArray *VM_array = g_ptr_array_new();
	char* req_memory_size, req_vcpu_number;

	json_object *jobj = json_tokener_parse((const char*)json_string);	// Parse JSON
	req_memory_size = json_get_type_value(jobj, JSON_KEY_MEMORY_SIZE, json_type_string);	// Get Parameters
	req_vcpu_number = json_get_type_value(jobj, JSON_KEY_VCPU_NUMBER, json_type_string);	// Get Parameters

	// Make VM List From JSON
	int ret_number = json_get_VM_Info(jobj, JSON_KEY_VM, VM_array);
	int i = 0;
	for(i = 0; i < ret_number; i++) {
		VM_Info *vm = (VM_Info *) g_ptr_array_index(VM_array, i);
		print_VM_Info(vm);
	}

	// Free memory
	free(req_memory_size);
	free(req_vcpu_number);
	delete_VM_Info(VM_array);
	g_ptr_array_free(VM_array, TRUE);

}

void parse_req_stop(int sock_fd, const char* json_string){
	// Data
	GPtrArray *VM_array = g_ptr_array_new();

	// Parse JSON
	json_object *jobj = json_tokener_parse((const char*)json_string);
	// Make VM List From JSON
	int ret_number = json_get_VM_Info(jobj, JSON_KEY_VM, VM_array);
	int i = 0;
	for(i = 0; i < ret_number; i++) {
		VM_Info *vm = (VM_Info *) g_ptr_array_index(VM_array, i);
		print_VM_Info(vm);
	}

	// Free memory
	delete_VM_Info(VM_array);
	g_ptr_array_free(VM_array, TRUE);

}


