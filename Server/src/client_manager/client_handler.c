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
#include <time.h>


//static int launch_VM(const char *tmp_overlay_disk_path, const char *tmp_overlay_mem_path, VM_Info *overlayVM, VM_Info *baseVM);
static const char* create_output_string_with_vm(const VM_Info *vm);
static int saveToFile(int sock_fd, const char* path, int size);
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
	}

	// Receive Overlay VM's disk image and memory
	Client_Msg send_msg;
	const char tmp_overlay_disk_path[256], tmp_overlay_mem_path[256];

	const long overlay_disk_size = stringToLong(overlayVM->diskimg_size);
	const long overlay_mem_size = stringToLong(overlayVM->memory_snapshot_size);
	if(overlay_disk_size <= 0 || overlay_mem_size <= 0){
		PRINT_ERR("Invalid overlay disk or mem size : %s\n", json_string);
	}

	// Save them to disk
	srand(time(NULL));
	int rand = random();
	sprintf(tmp_overlay_disk_path, "/tmp/%s_%d.img", overlayVM->name, rand);
	sprintf(tmp_overlay_mem_path, "/tmp/%s_%d.mem", overlayVM->name, rand);

	if(overlayVM != NULL && baseVM != NULL){
		// Wait transfer
		int ret1 = saveToFile(sock_fd, tmp_overlay_disk_path, overlay_disk_size);
		int ret2 = saveToFile(sock_fd, tmp_overlay_mem_path, overlay_mem_size);
		if(ret1 != overlay_disk_size || ret2 != overlay_mem_size){
			// error while saving it to files
			// Send error message
			const char *error_string = json_create_error(JSON_ERROR_SPECIFY_ONE_VM);
			send_msg.cmd = endian_swap_int(COMMAND_ACK_TRANSFER_START);
			send_msg.payload_length = endian_swap_int(strlen(error_string));
			PRINT_OUT("[%d] COMMAND_ACK_TRANSFER_START Error Sent\n", sock_fd);
			write_full(sock_fd, &send_msg, sizeof(send_msg));
			write_full(sock_fd, JSON_ERROR_SPECIFY_ONE_VM, send_msg.payload_length);
			free(error_string);
		}else{
			// normal end, send back base VM information
			const char* jstring = create_output_string_with_vm(baseVM);
			send_msg.cmd = endian_swap_int(COMMAND_ACK_TRANSFER_START);
			send_msg.payload_length = endian_swap_int(strlen(jstring));
			PRINT_OUT("[%d] COMMAND_ACK_TRANSFER_START Success Sent\n", sock_fd);
			PRINT_OUT("ret : %s\n", jstring);

			write_full(sock_fd, &send_msg, sizeof(send_msg));
			write_full(sock_fd, jstring, strlen(jstring));


			//Launch VM
			int ret = launch_VM(tmp_overlay_disk_path, tmp_overlay_mem_path, overlayVM, baseVM);
			if(ret == SUCCESS){
				//Compact it into Full JSON format
				json_object *jobj_vm = json_create_from_VMInfo(baseVM);
				json_object *jobj = json_object_new_object();
				json_object *jarray = json_object_new_array();
				json_object_array_add(jarray, jobj_vm);
				json_object_object_add(jobj, JSON_KEY_VM, jarray);
				//Add VM Ip Address
				const char* VM_IP = myip();
				json_object_object_add(jobj, JSON_KEY_LAUNCH_VM_IP, json_object_new_string(VM_IP));
				const char* jstring = json_object_to_json_string(jobj);

				send_msg.cmd = endian_swap_int(COMMAND_ACK_VM_LAUNCH);
				send_msg.payload_length = endian_swap_int(strlen(jstring));
				PRINT_OUT("[%d] COMMAND_ACK_VM_LAUNCH Success Sent\n", sock_fd);
				PRINT_OUT("ret : %s\n", jstring);
				write_full(sock_fd, (const char*)&send_msg, sizeof(send_msg));
				write_full(sock_fd, jstring, strlen(jstring));
			}

			free(jstring);
		}

	}else{
		// Required to specify one VM
		// Send error Message
		const char *error_string = json_create_error(JSON_ERROR_SPECIFY_ONE_VM);
		send_msg.payload_length = endian_swap_int(strlen(error_string));
		PRINT_OUT("[%d] COMMAND_ACK_TRANSFER_START Error Sent\n", sock_fd);
		PRINT_OUT("error ret : %s\n", error_string);
		write_full(sock_fd, &send_msg, sizeof(send_msg));
		write_full(sock_fd, JSON_ERROR_SPECIFY_ONE_VM, send_msg.payload_length);
		free(error_string);
	}

	// Free memory
	delete_VM_Info(VM_array);
	g_ptr_array_free(VM_array, TRUE);

}

void parse_req_launch(int sock_fd, const char* json_string){
	// DO NOT USE
	// We will automatically start launch once we got overlay VM.
	// So, there is no more launch request.
	// Just check
}

void parse_req_stop(int sock_fd, const char* json_string){

	/*
	GPtrArray *VM_array = g_ptr_array_new();
	json_object *jobj = json_tokener_parse((const char*) vm_configuration); // Parse JSON
	int ret_number = json_get_VM_Info(jobj, JSON_KEY_VM, VM_array);

	VM_Info *vm = (VM_Info *) g_ptr_array_index(VM_array, 0);
	vm->version = "10000";

	json_object *ret_jobj = json_object_new_object();
	json_add_VMs(ret_jobj, VM_array);
	json_object_object_add(ret_jobj, JSON_KEY_CLOUDLET_CPU_CLOCK,
			json_object_new_string("3.25"));
	PRINT_OUT("ret : %s\n", json_object_to_json_string(ret_jobj));
	*/
}


/*
 * Private Method
 * Launch VM base on overlay images
 */
int launch_VM(const char *tmp_overlay_disk_path, const char *tmp_overlay_mem_path, VM_Info *overlayVM, VM_Info *baseVM){
	//run python script
	char command[512] = {'\0'};
	sprintf(command, "%s -o %s %s", synthesis_script, baseVM->diskimg_path, baseVM->memory_snapshot_path);
	printf("%s\n", command);
	int ret = system(command);
	printf("run result : %d\n", ret);

	return SUCCESS;
}

static const char* create_output_string_with_vm(const VM_Info *vm){
	json_object *jobj = json_object_new_object();
	json_object *jarray = json_object_new_array();
	json_object *jobj_vm = json_create_from_VMInfo(vm);
	json_object_array_add(jarray, jobj_vm);
	json_object_object_add(jobj, JSON_KEY_VM, jarray);
	const char* jstring = json_object_to_json_string(jobj);
}

static int saveToFile(int sock_fd, const char* path, int size){
	FILE *file = fopen(path, "wb");
	if(file == NULL)
		return -1;

	int buffer_size = 1024 * 1024 * 3;
	char* buffer = (char*)malloc(sizeof(char) * buffer_size);

	int file_write_total = 0;
	int read_size = 0, left_byte = size;
	while(left_byte > 0){
		if(left_byte >= buffer_size){
			// read full
			read_size = read_full(sock_fd, buffer, buffer_size);
			file_write_total += fwrite(buffer, 1, read_size, file);
			left_byte -= read_size;
			PRINT_OUT("save (%d/%d) -> %d to %s\n", (size-left_byte), size, file_write_total, path);
		}else{
			// read left
			read_size = read_full(sock_fd, buffer, left_byte);
			file_write_total += fwrite(buffer, 1, read_size, file);
			left_byte -= read_size;
			PRINT_OUT("save (%d/%d) -> %d to %s\n", (size-left_byte), size, file_write_total, path);
		}
	}

	free(buffer);
	fclose(file);
	return file_write_total;
}

