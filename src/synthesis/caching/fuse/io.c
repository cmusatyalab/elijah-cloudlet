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

void _cachefs_write_error(const char *format, ... )
{
	va_list arg;
	int done;

    g_mutex_lock(pipe_lock);
	fprintf(stdout, "[error][%lu]", (unsigned long)pthread_self());
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
    	_cachefs_write_error("[io] Couldn't read %s: Premature end of file", file);
        return false;
    } else {
        g_file_error_from_errno(errno); 
    	_cachefs_write_error("[io] Couldn't read %s: error %d", file, errno);
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
    	_cachefs_write_error("[io] Couldn't write %s: Premature end of file", file);
        return false;
    }
    return true;
}
