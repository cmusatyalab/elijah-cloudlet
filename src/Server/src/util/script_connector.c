/*
 * script_connector.c
 *
 *  Created on: Nov 16, 2011
 *      Author: krha
 */

#include "script_connector.h"

#include <stdio.h>
#include <sys/wait.h>


int python_exec() {
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


