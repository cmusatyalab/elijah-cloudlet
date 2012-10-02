#include "qemu-cloudlet.h"
#include "stddef.h"

#include <stdio.h>
#include <stdarg.h>

static FILE *cloudlet_logfile = NULL;

int printlog(const char* format, ...){
	if(cloudlet_logfile){
		va_list argptr;
		va_start(argptr, format);
		fprintf(cloudlet_logfile, format, argptr);
		va_end(argptr);
		return 1;
	}
	return 0;
}


int cloudlet_init(const char *logfile_path){
	cloudlet_logfile = fopen(logfile_path, "w+");
	if (cloudlet_logfile == NULL) {
		return 0;
	}
	return 1;
}

int cloudlet_end(void){
    if (cloudlet_logfile) {
    	fclose(cloudlet_logfile);
    	return 1;
    }
    return 0;
}
