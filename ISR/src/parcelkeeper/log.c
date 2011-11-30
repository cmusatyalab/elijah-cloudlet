/*
 * Parcelkeeper - support daemon for the OpenISR (R) system virtual disk
 *
 * Copyright (C) 2006-2010 Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * LICENSE.GPL.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <execinfo.h>
#include "defs.h"

#define MAX_BACKTRACE_LEN 32

static struct {
	GMutex *lock;
	gchar *path;
	unsigned file_mask;
	unsigned stderr_mask;
	pid_t pk_pid;
	FILE *fp;
} log_state;

static void curtime(char *buf, unsigned buflen)
{
	struct timeval tv;
	struct tm tm;
	char fmt[22];

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm);
	snprintf(fmt, sizeof(fmt), "%%b %%d %%Y %%H:%%M:%%S.%.3u",
				(unsigned)(tv.tv_usec / 1000));
	buf[0]=0;
	strftime(buf, buflen, fmt, &tm);
}

static pk_err_t parse_logtype(const char *name, enum pk_log_type *out)
{
	if (!strcmp(name, "info"))
		*out=LOG_INFO;
	else if (!strcmp(name, "chunk"))
		*out=LOG_CHUNK;
	else if (!strcmp(name, "fuse"))
		*out=LOG_FUSE;
	else if (!strcmp(name, "transport"))
		*out=LOG_TRANSPORT;
	else if (!strcmp(name, "query"))
		*out=LOG_QUERY;
	else if (!strcmp(name, "slow"))
		*out=LOG_SLOW_QUERY;
	else if (!strcmp(name, "error"))
		*out=LOG_WARNING;  /* ERROR is just WARNING | _LOG_BACKTRACE */
	else if (!strcmp(name, "stats"))
		*out=LOG_STATS;
	else
		return PK_INVALID;
	return PK_SUCCESS;
}

static const char *log_prefix(enum pk_log_type type)
{
	switch (type & ~_LOG_BACKTRACE) {
	case LOG_INFO:
		return "INFO";
	case LOG_CHUNK:
		return "CHUNK";
	case LOG_FUSE:
		return "FUSE";
	case LOG_TRANSPORT:
		return "TRANSPORT";
	case LOG_QUERY:
		return "QUERY";
	case LOG_SLOW_QUERY:
		return "SLOW";
	case LOG_WARNING:
		return "ERROR";
	case LOG_STATS:
		return "STATS";
	}
	return NULL;
}

/* Cannot call pk_log(), since the logger hasn't started yet */
pk_err_t logtypes_to_mask(const char *list, unsigned *out)
{
	gchar **types;
	enum pk_log_type type;
	int i;

	*out=0;
	if (strcmp(list, "none")) {
		types=g_strsplit(list, ",", 0);
		for (i=0; types[i] != NULL; i++) {
			if (parse_logtype(types[i], &type)) {
				g_strfreev(types);
				return PK_INVALID;
			}
			*out |= (1 << type);
		}
		g_strfreev(types);
	}
	return PK_SUCCESS;
}

static void open_log(void)
{
	log_state.fp=fopen(log_state.path, "a");
	if (log_state.fp == NULL)
		pk_log(LOG_ERROR, "Couldn't open log file %s", log_state.path);
}

static void close_log(void)
{
	fclose(log_state.fp);
	log_state.fp=NULL;
}

static void check_log(void)
{
	struct stat st;

	if (log_state.fp == NULL)
		return;
	if (fstat(fileno(log_state.fp), &st)) {
		close_log();
		pk_log(LOG_ERROR, "Couldn't stat log file %s", log_state.path);
		return;
	}
	if (st.st_nlink == 0) {
		close_log();
		open_log();
		pk_log(LOG_INFO, "Log file disappeared; reopening");
	}
}

static void log_backtrace(FILE *fp)
{
	void *frames[MAX_BACKTRACE_LEN];
	char **syms;
	int i;
	int count;

	count = backtrace(frames, MAX_BACKTRACE_LEN);
	syms = backtrace_symbols(frames, count);
	if (syms == NULL)
		return;
	fprintf(fp, "Backtrace:\n");
	for (i = 0; i < count; i++)
		fprintf(fp, "   %s\n", syms[i]);
	free(syms);
}

static void g_log_handler(const gchar *domain, GLogLevelFlags level,
			const gchar *message, void *data)
{
	if (!strcmp(domain, "isrsql") || !strcmp(domain, "isrutil")) {
		switch (level) {
		case G_LOG_LEVEL_MESSAGE:
			pk_log(LOG_WARNING, "%s", message);
			break;
		case G_LOG_LEVEL_INFO:
			pk_log(LOG_STATS, "%s", message);
			break;
		case SQL_LOG_LEVEL_QUERY:
			pk_log(LOG_QUERY, "%s", message);
			break;
		case SQL_LOG_LEVEL_SLOW_QUERY:
			pk_log(LOG_SLOW_QUERY, "%s", message);
			break;
		default:
			pk_log(LOG_ERROR, "%s", message);
			break;
		}
	} else {
		g_log_default_handler(domain, level, message, data);
	}
}

void pk_log(enum pk_log_type type, const char *fmt, ...)
{
	va_list ap;
	char buf[50];

	g_mutex_lock(log_state.lock);
	if (log_state.fp != NULL && ((1 << type) & log_state.file_mask)) {
		curtime(buf, sizeof(buf));
		check_log();
		/* Ignore errors; it's better to write the log entry unlocked
		   than to drop it on the floor */
		get_file_lock(fileno(log_state.fp),
					FILE_LOCK_WRITE | FILE_LOCK_WAIT);
		fseek(log_state.fp, 0, SEEK_END);
		va_start(ap, fmt);
		fprintf(log_state.fp, "%s %d %s: ", buf, log_state.pk_pid,
					log_prefix(type));
		vfprintf(log_state.fp, fmt, ap);
		fprintf(log_state.fp, "\n");
		va_end(ap);
		if (type & _LOG_BACKTRACE)
			log_backtrace(log_state.fp);
		fflush(log_state.fp);
		put_file_lock(fileno(log_state.fp));
	}

	if ((1 << type) & log_state.stderr_mask) {
		va_start(ap, fmt);
		fprintf(stderr, "PK: ");
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, "\n");
		va_end(ap);
		if (type & _LOG_BACKTRACE)
			log_backtrace(stderr);
	}
	g_mutex_unlock(log_state.lock);
}

void log_start(const char *path, unsigned file_mask, unsigned stderr_mask)
{
	log_state.lock = g_mutex_new();
	log_state.path = g_strdup(path);
	log_state.file_mask = file_mask;
	log_state.stderr_mask = stderr_mask;
	log_state.pk_pid=getpid();
	/* stderr is unbuffered by default */
	setlinebuf(stderr);
	if (path != NULL && file_mask)
		open_log();
	g_log_set_handler("isrsql", ~0, g_log_handler, NULL);
	g_log_set_handler("isrutil", ~0, g_log_handler, NULL);
}

void log_shutdown(void)
{
	pk_log(LOG_INFO, "Parcelkeeper shutting down");
	if (log_state.fp != NULL)
		close_log();
	g_free(log_state.path);
	log_state.path = NULL;
	g_mutex_free(log_state.lock);
}
