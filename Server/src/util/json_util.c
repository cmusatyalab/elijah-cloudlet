/*
 * json_util.c
 *
 *  Created on: Nov 16, 2011
 *      Author: krha
 */

#include "json_util.h"
#include "../lib/lib_type.h"
#include <stdlib.h>
#include <string.h>

/*
 * Private method
 */
static void print_json(json_object *jobj) {
	enum json_type type;
	json_object_object_foreach(jobj, key, val)	{
		PRINT_OUT("(%s --> %s) type: ", key, json_object_to_json_string(val), type);
		type = json_object_get_type(val);
		switch (type) {
		case json_type_null:
			PRINT_OUT("json_type_null\n");
			break;
		case json_type_boolean:
			PRINT_OUT("json_type_boolean\n");
			break;
		case json_type_double:
			PRINT_OUT("json_type_double\n");
			break;
		case json_type_int:
			PRINT_OUT("json_type_int\n");
			break;
		case json_type_object:
			PRINT_OUT("json_type_object\n");
			break;
		case json_type_array:
			PRINT_OUT("json_type_array\n");
			break;
		case json_type_string:
			PRINT_OUT("json_type_string\n");
			break;
		}
	}
}



/*
 * PUBLIC Method
 */
int json_get_VM_Info(json_object * jobj, const char* key_string, GPtrArray* VM_array){

	json_object *vm_objects = json_object_object_get(jobj, key_string);
	if(vm_objects == NULL)
		return -1;

	int arraylen = json_object_array_length(vm_objects);
//	printf("Array Length: %d\n", arraylen);
	if(arraylen <= 0)
		return -1;

	int index = 0;
	for (index = 0; index < arraylen; index++) {
		VM_Info *vm = (VM_Info *)malloc(sizeof(struct VM_Info));
		memset(vm, '\0', sizeof(struct VM_Info));
		json_object * jvalue = json_object_array_get_idx(vm_objects, index);

		vm->name = json_get_type_value(jvalue, JSON_KEY_VM_NAME, json_type_string);
		vm->type = json_get_type_value(jvalue, JSON_KEY_VM_TYPE, json_type_string);
		vm->diskimg_path = json_get_type_value(jvalue, JSON_KEY_VM_DISK_NAME, json_type_string);
		vm->memory_snapshot_path = json_get_type_value(jvalue, JSON_KEY_VM_MEMORY_NAME, json_type_string);
		vm->version = json_get_type_value(jvalue, JSON_KEY_VM_VERSION, json_type_string);
		vm->diskimg_size = json_get_type_value(jvalue, JSON_KEY_VM_DISK_SIZE, json_type_string);
		vm->memory_snapshot_size = json_get_type_value(jvalue, JSON_KEY_VM_MEMORY_SIZE, json_type_string);
		vm->uuid = json_get_type_value(jvalue, JSON_KEY_VM_UUID, json_type_string);

		g_ptr_array_add(VM_array, vm);
	}

	return VM_array->len;
}

/*
 * The returned string memory must be deallocated by caller
 */
char* json_get_type_value(json_object * jobj, const char *key_string, json_type json_type) {
	char *result = NULL;
	enum json_type type;
	json_object_object_foreach(jobj, key, val) {
		// match key
		if(strcasecmp(key_string, key) == 0){
			type = json_object_get_type(val);
			// match type
			if(type == json_type){
				// memory of val_string is managed by json lib
				const char* val_string = json_object_get_string(val);
				result = (char*) malloc(strlen(val_string) * sizeof(char) + 1);
				result[strlen(val_string)] = '\0';
				strcpy(result, val_string);
				free(val_string);
				return result;
			}
		}
	}

	return result;
}


char* json_parse(json_object * jobj, const char *key_string) {
	char *result = NULL;

	enum json_type type;
	json_object_object_foreach(jobj, key, val)
	{

		type = json_object_get_type(val);
		switch (type) {
		case json_type_string:
			printf("type: json_type_string, %s : ", key);
			printf("%s\n", json_object_get_string(val));
			break;

		case json_type_array:
			printf("type: json_type_array, ");
			jobj = json_object_object_get(jobj, key);
			int arraylen = json_object_array_length(jobj);
			printf("Array Length: %d\n", arraylen);
			int i;
			json_object * jvalue;
			for (i = 0; i < arraylen; i++) {
				jvalue = json_object_array_get_idx(jobj, i);
				printf("value[%d]: %s\n", i, json_object_get_string(jvalue));
			}
			break;
		}
	}

	return result;
}

/*
 * Print out VM information
 */
void print_VM_Info(VM_Info *vm){
	PRINT_OUT("%s, %s, %s, %s, %s\n", vm->name, vm->type, vm->uuid, vm->diskimg_path, vm->memory_snapshot_path);
	PRINT_OUT("disk-size:%ld\n", stringToLong(vm->diskimg_size));
	PRINT_OUT("mem-size:%ld\n", stringToLong(vm->memory_snapshot_size));

}
/*
 * Convert char* to long int
 */
long int stringToLong(const char *string){
	if(string != NULL && strlen(string) > 0){
		char * pEnd;
		long int li = strtol(string, &pEnd, 10);
		return li;
	}
	return -1;
}

/*
 * Deallocate memory at VM_array
 */
void delete_VM_Info(GPtrArray *VM_array){
	if(VM_array == NULL)
		return;

	int i = 0;
	for (i = 0; i < VM_array->len; i++) {
		VM_Info *vm = (VM_Info *) g_ptr_array_index(VM_array, i);
		if(vm != NULL){
			if(vm->diskimg_path != NULL)
				free(vm->diskimg_path);
			if(vm->memory_snapshot_path != NULL)
				free(vm->memory_snapshot_path);
			if(vm->name != NULL)
				free(vm->name);
			if(vm->type != NULL)
				free(vm->type);
			if(vm->uuid != NULL)
				free(vm->uuid);
			if(vm->version != NULL)
				free(vm->version);
		}
	}
}

/*
 * Create JSON String using error_message and return it as const char*
 */
const char* json_create_error(const char* error_message){
	json_object *jobj = json_object_new_object();
	json_object *jerror_string = json_object_new_string(error_message);
	json_object_object_add(jobj, JSON_KEY_ERROR, jerror_string);

	return json_object_to_json_string(jobj);
}

int json_add_VMs(json_object *jobj, GPtrArray *VM_array){

	json_object *jarray = json_object_new_array();
	int array_length = VM_array->len;
	int i;
	for(i = 0; i < array_length; i++){
		VM_Info *vm = (VM_Info *) g_ptr_array_index(VM_array, i);
		json_object* vm_object =json_create_from_VMInfo(vm);
		json_object_array_add(jarray, vm_object);
	}

	json_object_object_add(jobj, JSON_KEY_VM, jarray);
	return 1;
}

json_object* json_create_from_VMInfo(VM_Info *vminfo){
	if(vminfo == NULL)
		return NULL;

	json_object *jobj = json_object_new_object();

	const char* string = vminfo->name;
	if(string != NULL && strlen(string) > 0){
		json_object_object_add(jobj, JSON_KEY_VM_NAME, json_object_new_string(string));
	}
	string = vminfo->diskimg_path;
	if(string != NULL && strlen(string) > 0){
		json_object_object_add(jobj, JSON_KEY_VM_DISK_NAME, json_object_new_string(string));
	}
	string = vminfo->diskimg_size;
	if(string != NULL && strlen(string) > 0){
		json_object_object_add(jobj, JSON_KEY_VM_DISK_SIZE, json_object_new_string(string));
	}
	string = vminfo->memory_snapshot_path;
	if(string != NULL && strlen(string) > 0){
		json_object_object_add(jobj, JSON_KEY_VM_MEMORY_NAME, json_object_new_string(string));
	}
	string = vminfo->memory_snapshot_size;
	if(string != NULL && strlen(string) > 0){
		json_object_object_add(jobj, JSON_KEY_VM_MEMORY_SIZE, json_object_new_string(string));
	}
	string = vminfo->type;
	if(string != NULL && strlen(string) > 0){
		json_object_object_add(jobj, JSON_KEY_VM_TYPE, json_object_new_string(string));
	}
	string = vminfo->uuid;
	if(string != NULL && strlen(string) > 0){
		json_object_object_add(jobj, JSON_KEY_VM_UUID, json_object_new_string(string));
	}
	string = vminfo->version;
	if(string != NULL && strlen(string) > 0){
		json_object_object_add(jobj, JSON_KEY_VM_VERSION, json_object_new_string(string));
	}

	return jobj;
}
