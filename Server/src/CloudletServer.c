#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#include "client_manager/client_manager.h"
#include "client_manager/client_handler.h"
#include "lib/lib_type.h"
#include "util/json_util.h"

void print_help();
void check_config_file(int argc, char **argv);

extern FILE *log_file;

int main(int argc, char **argv) {
	create_logfile();
	check_config_file(argc, argv);
	// Run Socket Server
	if (init_client_manager() != SUCCESS) {
		fprintf(stderr, "Cannot run TCP Server\n");
		return -1;
	}

	while (1) {
		sleep(10000);
	}

	end_client_manager();
	return 0;
}

/*
 * Print Help message
 */
void print_help() {
	printf("cloudlet_server [-h] [-c FILE]\n\n");
	printf("  -h              print this help and exit\n");
	printf("  -v              set verbose flag\n");
	printf("  -c FILE         set configuration file\n");
}

/*
 * check validity of input argument & JSON configuration file
 */
void check_config_file(int argc, char **argv) {
	char *config_filename = NULL;
	int c;
	opterr = 0;

	// handle input parameters
	while ((c = getopt(argc, argv, "hc:")) != -1) {
		switch (c) {
		case 'h':
			print_help();
			exit(0);
			break;
		case 'c':
			config_filename = optarg;
			break;
		case '?':
			if (optopt == 'c')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else
				fprintf(stderr, "Unknown option character '%c'.\n", optopt);
			exit(1);
		default:
			print_help();
			exit(1);
		}
	}

	// configuration file check
	if (config_filename == NULL) {
		print_help();
		exit(1);
	}
	FILE *file;
	if ((file = fopen(config_filename, "r")) == NULL) {
		fprintf(stderr, "No such file : %s\n", config_filename);
		exit(1);
	}

	// configuration data check
	fseek(file, 0L, SEEK_END);
	int size = ftell(file);
	fseek(file, 0L, SEEK_SET);
	char *json_string = (char*) malloc(size * sizeof(char));
	memset(json_string, 0, size);
	fread(json_string, size, 1, file);
	fclose(file);

	json_object *jobj = json_tokener_parse((const char*) json_string); // Parse JSON
	char *cpu_clock = json_get_type_value(jobj, JSON_KEY_CLOUDLET_CPU_CLOCK,
			json_type_string); // Get JSON Parameters
	char *cpu_core = json_get_type_value(jobj, JSON_KEY_CLOUDLET_CPU_CORE,
			json_type_string); // Get JSON Parameters
	char *mem_size = json_get_type_value(jobj, JSON_KEY_CLOUDLET_MEMORY_SIZE,
			json_type_string); // Get JSON Parameters
	GPtrArray *VM_array = g_ptr_array_new();
	int ret_number = json_get_VM_Info(jobj, JSON_KEY_VM, VM_array);

	if (cpu_clock == NULL || cpu_core == NULL || mem_size == NULL || (strlen(
			cpu_clock) <= 0) || (strlen(cpu_core) <= 0) || (strlen(mem_size)
			<= 0) || (ret_number < 1)) {

		//Invalid argument
		fprintf(stderr, "configuration is invalid\n");
		fprintf(stderr, "%s : %s\n", JSON_KEY_CLOUDLET_CPU_CLOCK, cpu_clock);
		fprintf(stderr, "%s : %s\n", JSON_KEY_CLOUDLET_CPU_CORE, cpu_core);
		fprintf(stderr, "%s : %s\n", JSON_KEY_CLOUDLET_MEMORY_SIZE, mem_size);
		fprintf(stderr, "VM : %d\n", ret_number);

		free(cpu_clock);
		free(cpu_core);
		free(mem_size);
		delete_VM_Info(VM_array);
		g_ptr_array_free(VM_array, TRUE);

		exit(1);
	}

	// save it to configuration char string
	vm_configuration = (char*) malloc(sizeof(char) * strlen(json_string));
	strcpy(vm_configuration, json_string);
	PRINT_OUT("VM Configuration : %s", vm_configuration);

	free(json_string);
	free(cpu_clock);
	free(cpu_core);
	free(mem_size);
	delete_VM_Info(VM_array);
	g_ptr_array_free(VM_array, TRUE);
}

