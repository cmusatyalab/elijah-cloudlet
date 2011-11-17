#include <stdio.h>
#include <unistd.h>

#include "client_manager/client_manager.h"
#include "lib/lib_type.h"

int main(void) {

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


