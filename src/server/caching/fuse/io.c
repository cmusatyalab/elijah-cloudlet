/*
 * cloudletcacheFS - Cloudlet Cachcing emulation FS
 *
 * copyright (c) 2006-2012 carnegie mellon university
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
