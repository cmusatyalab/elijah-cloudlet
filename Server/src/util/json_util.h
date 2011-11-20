/*
 * json_util.h
 *
 *  Created on: Nov 16, 2011
 *      Author: krha
 */

#ifndef JSON_UTIL_H_
#define JSON_UTIL_H_

#include <glib.h>
#include <json/json.h>
#include "../protocol.h"

#define json_object_object_foreach(obj,key,val) \
	char *key; struct json_object *val; \
	struct lh_entry *entry; \
	for(entry = json_object_get_object(obj)->head; ({ if(entry) { key = (char*)entry->k; val = (struct json_object*)entry->v; } ; entry; }); entry = entry->next )


/*
 * JSON Parsing
 */
char* json_parse(json_object * jobj, const char *key_string);
char* json_get_type_value(json_object * jobj, const char *key_string, json_type json_type);
int json_get_VM_Info(json_object * jobj, const char* key_string, GPtrArray* VM_array);

/*
 * JSON Creating
 */
const char* json_create_error(const char* error_message);
int json_add_VMs(json_object *jobj, GPtrArray *VM_array);
json_object* json_create_from_VMInfo(VM_Info *vminfo);

/*
 * JSON Util
 */
long int stringToLong(const char *string);
void print_VM_Info(VM_Info *vm);
void delete_VM_Info(GPtrArray *VM_array);

#endif /* JSON_UTIL_H_ */
