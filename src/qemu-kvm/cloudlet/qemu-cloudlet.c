#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

#include "qemu-cloudlet.h"
#include "stddef.h"

static FILE *cloudlet_logfile = NULL;

int printlog(const char* format, ...){

	if(cloudlet_logfile){
		struct timeval tv;
		gettimeofday(&tv, NULL);
		fprintf(cloudlet_logfile, "time:%ld.%ld, ", tv.tv_sec, tv.tv_usec);

		va_list argptr;
		va_start(argptr, format);
		vfprintf(cloudlet_logfile, format, argptr);
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
