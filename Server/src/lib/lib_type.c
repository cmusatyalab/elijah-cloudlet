/*
 * lib_type.c
 *
 *  Created on: Nov 21, 2011
 *      Author: krha
 */
#include "lib_type.h"

#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

static char buf[1024] = { '\0', };

void print_time_info(char *fmt, ...) {
	memset(buf, 0, sizeof(buf));

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (strncasecmp(buf, "[Time]", 5) != 0) {
		struct timeval val;
		struct tm *tm;
		gettimeofday(&val, NULL);
		tm = localtime(&val.tv_sec);
		fprintf(log_file, "[Time] %s : %02d:%02d:%02d.%06ld\n", buf, tm->tm_hour,	tm->tm_min, tm->tm_sec, val.tv_usec);
		PRINT_OUT("[Time] %s : %02d:%02d:%02d.%06ld\n", buf, tm->tm_hour, tm->tm_min, tm->tm_sec, val.tv_usec);
	} else {
		fprintf(log_file, buf);
		PRINT_OUT("%s", buf);
	}
	fflush(log_file);
}

/*
 * Save Log to file for Performance Analysis.
 * So, we only save logs that are related to Time measurement.
 */
void create_logfile() {
	time_t now;
	struct tm *tm;
	now = time(0);
	if ((tm = localtime(&now)) == NULL) {
		fprintf(stderr, "Error extracting time stuff\n");
	}

	log_filename = (char *)malloc(256);
	sprintf(log_filename, "log-%04d%02d%02d-%02d%02d%02d", tm->tm_year + 1900,
			tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	fprintf(stdout, "Log file name : %s\n", log_filename);
	log_file = fopen(log_filename, "w");
}

void close_logfile() {
	if (log_file != NULL) {
		fclose(log_file);
	}
}


/* Subtract the `struct timeval' values X and Y,
 storing the result in RESULT.
 Return 1 if the difference is negative, otherwise 0.

int timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y) {
	// Perform the carry for the later subtraction by updating y.
	if (x->tv_usec < y->tv_usec) {
		int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
		y->tv_usec -= 1000000 * nsec;
		y->tv_sec += nsec;
	}
	if (x->tv_usec - y->tv_usec > 1000000) {
		int nsec = (x->tv_usec - y->tv_usec) / 1000000;
		y->tv_usec += 1000000 * nsec;
		y->tv_sec -= nsec;
	}

	// Compute the time remaining to wait.
	// tv_usec is certainly positive.
	result->tv_sec = x->tv_sec - y->tv_sec;
	result->tv_usec = x->tv_usec - y->tv_usec;

	// Return 1 if result is negative.
	return x->tv_sec < y->tv_sec;
}
*/
