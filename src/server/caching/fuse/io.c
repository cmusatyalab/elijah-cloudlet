/*
 * cloudletcacheFS - Cloudlet Cachcing emulation FS
 *
 * copyright (c) 2011-2013 carnegie mellon university
 *
 * this program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the gnu general public license as published
 * by the free software foundation.  a copy of the gnu general public license
 * should have been distributed along with this program in the file
 * copying.
 *
 * this program is distributed in the hope that it will be useful, but
 * without any warranty; without even the implied warranty of merchantability
 * or fitness for a particular purpose.  see the gnu general public license
 * for more details.
 */


#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "cachefs-private.h"

static GMutex *pipe_lock = NULL;

bool _cachefs_init_pipe_communication()
{
	pipe_lock = g_mutex_new();
	return true;
}

void _cachefs_close_pipe_communication()
{
	if (pipe_lock != NULL){
		g_mutex_free(pipe_lock);
	}
}

cachefs_cond* _cachefs_write_request(cachefs* fs, const char *format, ... )
{
	va_list arg;
	int done;

    g_mutex_lock(pipe_lock);

	// create conditional variable
	struct cachefs_cond* cond = NULL;
	cond = g_hash_table_lookup(fs->file_locks, request_file);
	if (cond == NULL){
		cond = _cachefs_cond_new();
		g_hash_table_insert(fs->file_locks, request_file, cond);

		// only the first thread send a request
		fprintf(stdout, "[request]");
		va_start (arg, format);
		done = vfprintf(stdout, format, arg);
		va_end (arg);
		fprintf(stdout, "\n");
		fflush(stdout);
	}
	// add myself to the list
    cond->threads = g_list_prepend(cond->threads, &pthread_self());

    g_mutex_unlock(pipe_lock);
    return cond;
}

void _cachefs_write_error(const char *format, ... )
{
	va_list arg;
	int done;

    g_mutex_lock(pipe_lock);
	fprintf(stdout, "[error]");
	va_start (arg, format);
	done = vfprintf(stdout, format, arg);
	va_end (arg);
	fprintf(stdout, "\n");
	fflush(stdout);
    g_mutex_unlock(pipe_lock);
}

void _cachefs_write_debug(const char *format, ... )
{
	va_list arg;
	int done;

    g_mutex_lock(pipe_lock);
	fprintf(stdout, "[debug]");
	va_start (arg, format);
	done = vfprintf(stdout, format, arg);
	va_end (arg);
	fprintf(stdout, "\n");
	fflush(stdout);
    g_mutex_unlock(pipe_lock);
}

bool _cachefs_safe_pread(const char *file, void *buf, uint64_t count, uint64_t offset)
{
    uint64_t cur;
    int fd = open(file, O_RDONLY);

    while (count > 0 && (cur = pread(fd, buf, count, offset)) > 0) {
        buf += cur;
        offset += cur;
        count -= cur;
    }

    close(fd);
    if (count == 0) {
        return true;
    } else if (cur == 0) {
        fprintf(stderr, "Couldn't read %s: Premature end of file", file);
        return false;
    } else {
        g_file_error_from_errno(errno); 
        fprintf(stderr, "Couldn't read %s: %s", file, strerror(errno));
        return false;
    }
}

bool _cachefs_safe_pwrite(const char *file, const void *buf, uint64_t count, uint64_t offset)
{
    int64_t cur;
    int fd = open(file, O_RDONLY);

    while (count > 0 && (cur = pwrite(fd, buf, count, offset)) >= 0) {
        buf += cur;
        offset += cur;
        count -= cur;
    }
    close(fd);
    if (count > 0) {
        g_file_error_from_errno(errno); 
    	fprintf(stderr, "Counln't write %s: %s", file, strerror(errno));
        return false;
    }
    return true;
}
