/*
 * cloudletcachefs - cloudlet cachcing emulation fs
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

#ifndef CACHEFS_PRIVATE_H
#define CACHEFS_PRIVATE_H

#include <sys/stat.h>
#include <stdint.h>
#include <stdbool.h>
#include <glib.h>

#define CACHEFS_WRITE_ERROR(fmt, ...) \
    do { \
    	fprintf(stdout, "[error] " fmt, ## __VA_ARGS__); \
    	fprintf(stdout, "\n"); fflush(stdout); \
    } while (0) 


struct cachefs {
    GMainLoop *glib_loop;
    GHashTable *file_locks;
    char *mountpoint;
    char *uri_root;
    struct fuse *fuse;
    struct fuse_chan *chan;

    // const variables
    char *redis_ip;
    unsigned int redis_port;
    char *cache_root;
    char *url_root;
};

struct cachefs_cond {
	GMutex *lock;
	GCond *condition;
    GList *waiting_threads;
};

/* fuse */
void _cachefs_fuse_new(struct cachefs *fs, GError **err);
void _cachefs_fuse_run();
void _cachefs_fuse_terminate();
void _cachefs_fuse_free();

/* io */
bool _cachefs_init_pipe_communication();
void _cachefs_close_pipe_communication();
cachefs_cond* _cachefs_write_request(cachefs *fs, const char *format, ... );
void _cachefs_write_error(const char *format, ... );
void _cachefs_write_debug(const char *format, ... );
bool _cachefs_safe_pread(const char *file, void *buf, uint64_t count, uint64_t offset);
bool _cachefs_safe_pwrite(const char *file, const void *buf, uint64_t count, uint64_t offset);

/* redis */
bool _redis_init(const char *address, int port);
void _redis_close();
int _redis_file_exists(const char *path, bool *is_exists);
int _redis_get_attr(const char* path, char** ret_buf);
int _redis_get_readdir(const char* path, GSList **ret_list);

/* cond */
struct cachefs_cond *_cachefs_cond_new(void);
void _cachefs_cond_free(struct cachefs_cond *cond);
void _cachefs_cond_broadcast(struct cachefs_cond *cond);


#endif
