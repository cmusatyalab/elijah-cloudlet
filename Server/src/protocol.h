/*
 * protocol.h
 *
 *  Created on: Nov 16, 2011
 *      Author: krha
 */

#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define PROTOCOl_VERSION "0.1"

// Network Command List between client and server
#define COMMAND_REQ_VMLIST					0x0011
#define COMMAND_ACK_VMLIST					0x0012
#define COMMAND_REQ_TRANSFER_START			0x0021
#define COMMAND_ACK_TRANSFER_START			0x0022
#define COMMAND_REQ_VM_LAUNCH				0x0031
#define COMMAND_ACK_VM_LAUNCH				0x0032
#define COMMAND_REQ_VM_STOP					0x0041
#define COMMAND_ACK_VM_STOP					0x0042

// KEY String used in JSON Format
#define JSON_KEY_PROTOCOL_VERSION	"Protocol-version"
#define JSON_KEY_MEMORY_SIZE		"memory_size"
#define JSON_KEY_VCPU_NUMBER		"vcpu_number"

#define JSON_KEY_VM					"VM"
#define JSON_KEY_VM_NAME			"name"
#define JSON_KEY_VM_TYPE			"type"
#define JSON_KEY_VM_UUID			"uuid"
#define JSON_KEY_VM_DISK_NAME		"diskimg_path"
#define JSON_KEY_VM_DISK_SIZE		"diskimg_size"
#define JSON_KEY_VM_MEMORY_NAME		"memorysnapshot_path"
#define JSON_KEY_VM_MEMORY_SIZE		"memorysnapshot_size"
#define JSON_KEY_VM_VERSION			"version"
#define JSON_KEY_CLOUDLET_CPU_CLOCK		"CPU-Clock"
#define JSON_KEY_CLOUDLET_CPU_CORE		"CPU-Core"
#define JSON_KEY_CLOUDLET_MEMORY_SIZE	"Memory-Size"

#define JSON_KEY_LAUNCH_VM_IP 		"LaunchVM-IP"

#define JSON_VALUE_VM_TYPE_BASE				"baseVM"
#define JSON_VALUE_VM_TYPE_OVERLAY			"overlay"

#define JSON_KEY_ERROR					"Error"
#define JSON_ERROR_SPECIFY_ONE_VM		"Specify One VM Information"
#define JSON_ERROR_CANNOT_GENERATE_FILE "Cannot generate overlay imge file"
#define JSON_ERROR_INVALID_OVERLAY		"Invalid overlay binaray"

// Cloudlet Host Information
typedef struct Host_Spec {
	char *cpu_clock;
	char *cpu_core;
	char *memory_size;
}Host_Spec;

// VM Information
typedef struct VM_Info {
	char *name;
	char *type;
	char *uuid;
	char *diskimg_path;
	char *diskimg_size;
	char *memory_snapshot_path;
	char *memory_snapshot_size;
	char *version;
}VM_Info;

// Network Message Format
typedef struct Client_MSG{
	int cmd;
	int payload_length;
}Client_Msg;


// Global Data
#define synthesis_script		"/home/krha/Cloudlet/src/Script/cloudet.py"
const char *vm_configuration;



#endif /* PROTOCOL_H_ */
