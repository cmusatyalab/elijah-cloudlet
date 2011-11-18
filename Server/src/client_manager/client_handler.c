/*
 * client_handler.c
 *
 *  Created on: Nov 16, 2011
 *      Author: krha
 */

#include "client_handler.h"
#include "../lib/lib_type.h"
#include "../protocol.h"
#include "../util/json_util.h"
#include "../lib/lib_socket.h"

#include <json/json.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int saveToFile(int sock_fd, const char* path, int size){
	int buffer_size = 1024 * 100;
	char* buffer = (char*)malloc(sizeof(char) * buffer_size);

	int ret = 0, left_byte = size;
	while(left_byte > 0){
		if(left_byte >= buffer_size){
			// read full
			ret = read_full(sock_fd, buffer, buffer_size);
			left_byte -= ret;
			PRINT_OUT("save (%d/%d) to %s\n", (size-left_byte), size, path);
		}else{
			// read left
			ret = read_full(sock_fd, buffer, left_byte);
			left_byte -= ret;
			PRINT_OUT("last save (%d/%d) to %s\n", (size-left_byte), size, path);
		}
	}

	return -1;
}


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

	// return VM information with JSON
	Client_Msg send_msg;
	send_msg.cmd = endian_swap_int(COMMAND_ACK_VMLIST);						// send it with Big-endian
	send_msg.payload_length = endian_swap_int(strlen(vm_configuration));	// send it with Big-endian
	PRINT_OUT("[%d] COMMAND_ACK_VMLIST Sent\n", sock_fd);
	write_full(sock_fd, &send_msg, sizeof(send_msg));
	write_full(sock_fd, vm_configuration, strlen(vm_configuration));

	// Free memory
	free(protocol_version);
	delete_VM_Info(VM_array);
	g_ptr_array_free(VM_array, TRUE);
}

void parse_req_transfer(int sock_fd, const char* json_string){
	// Data
//	PRINT_OUT("Input JSON : %s\n", json_string);
	GPtrArray *VM_array = g_ptr_array_new();
	json_object *jobj = json_tokener_parse((const char*)json_string);	// Parse JSON

	// Find Base VM and Overlay VM Information From JSON
	int ret_number = json_get_VM_Info(jobj, JSON_KEY_VM, VM_array);
	int i = 0;
	VM_Info *baseVM = NULL, *overlayVM = NULL;
	for(i = 0; i < ret_number; i++) {
		VM_Info *vm = (VM_Info *) g_ptr_array_index(VM_array, i);
		if(strcasecmp(vm->type, JSON_VALUE_VM_TYPE_BASE) == 0){
			baseVM = vm;
		}else if(strcasecmp(vm->type, JSON_VALUE_VM_TYPE_OVERLAY) == 0){
			overlayVM = vm;
		}
		print_VM_Info(vm);
	}

	// Receive Overlay VM disk image and memory
	Client_Msg send_msg;
	send_msg.cmd = COMMAND_ACK_TRANSFER_START;
	const char tmp_overlay_disk_path[256], tmp_overlay_mem_path[256];

	const long overlay_disk_size = stringToLong(overlayVM->diskimg_size);
	const long overlay_mem_size = stringToLong(overlayVM->memory_snapshot_size);
	if(overlay_disk_size <= 0 || overlay_mem_size <= 0){
		PRINT_ERR("Invalid overlay disk or mem size : %s\n", json_string);
	}

	int rand = random();
	sprintf(tmp_overlay_disk_path, "/tmp/%s_%d.img", overlayVM->name, rand);
	sprintf(tmp_overlay_mem_path, "/tmp/%s_%d.mem", overlayVM->name, rand);

	if(overlayVM != NULL && baseVM != NULL){
		// Wait transfer
		boolean ret1 = saveToFile(sock_fd, tmp_overlay_disk_path, overlay_disk_size);
		boolean ret2 = saveToFile(sock_fd, tmp_overlay_mem_path, overlay_mem_size);

		// Validate VM Image

		// Send back request VM Information
		send_msg.payload_length = strlen(vm_configuration);
		PRINT_OUT("[%d] COMMAND_ACK_TRANSFER_START Sent\n", i);
		write_full(sock_fd, &send_msg, sizeof(send_msg));
		write_full(sock_fd, vm_configuration, strlen(vm_configuration));
	}else{
		// Required to specify one VM
		// Send error Message
		send_msg.payload_length = strlen(JSON_ERROR_SPECIFY_ONE_VM);
		PRINT_OUT("[%d] COMMAND_ACK_TRANSFER_START Sent\n", i);
		write_full(sock_fd, &send_msg, sizeof(send_msg));
		write_full(sock_fd, JSON_ERROR_SPECIFY_ONE_VM, strlen(JSON_ERROR_SPECIFY_ONE_VM));
	}



	// Free memory
	delete_VM_Info(VM_array);
	g_ptr_array_free(VM_array, TRUE);

}

void parse_req_launch(int sock_fd, const char* json_string){

}

void parse_req_stop(int sock_fd, const char* json_string){

}


/*
 * JSON Generator
 */
