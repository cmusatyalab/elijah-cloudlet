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

struct cachefs {
    GMainLoop *glib_loop;
    char *mountpoint;
    struct fuse *fuse;
    struct fuse_chan *chan;
};

//struct vmnetfs_image {
//    char *url;
//    char *username;
//    char *password;
//    char *total_overlay_chunks;
//    uint64_t initial_size;
//    /* if nonzero, server file is divided into segments of this size */
//    uint64_t segment_size;
//    uint32_t chunk_size;
//
//    /* io */
//    struct connection_pool *cpool;
//    struct chunk_state *chunk_state;
//    struct bitmap_group *bitmaps;
//    struct bitmap *accessed_map;
//
//    /* ll_pristine */
//    struct bitmap *total_overlay_map;
//    struct bitmap *current_overlay_map;
//    int base_fd;
//    int overlay_fd;
//
//    /* ll_modified */
//    int write_fd;
//    struct bitmap *modified_map;
//
//    /* stats */
//    struct vmnetfs_stream_group *io_stream;
//    struct vmnetfs_stat *bytes_read;
//    struct vmnetfs_stat *bytes_written;
//    struct vmnetfs_stat *chunk_fetches;
//    struct vmnetfs_stat *chunk_dirties;
//};
//
//
//struct vmnetfs_fuse_fh {
//    const struct vmnetfs_fuse_ops *ops;
//    void *data;
//    void *buf;
//    uint64_t length;
//    uint64_t change_cookie;
//    bool blocking;
//};
//
//struct fuse_pollhandle;
//struct vmnetfs_fuse_ops {
//    int (*getattr)(void *dentry_ctx, struct stat *st);
//    int (*truncate)(void *dentry_ctx, uint64_t length);
//    int (*open)(void *dentry_ctx, struct vmnetfs_fuse_fh *fh);
//    int (*read)(struct vmnetfs_fuse_fh *fh, void *buf, uint64_t start,
//            uint64_t count);
//    int (*write)(struct vmnetfs_fuse_fh *fh, const void *buf,
//            uint64_t start, uint64_t count);
//    int (*poll)(struct vmnetfs_fuse_fh *fh, struct fuse_pollhandle *ph,
//            bool *readable);
//    void (*release)(struct vmnetfs_fuse_fh *fh);
//    bool nonseekable;
//};
//
//#define VMNETFS_CONFIG_ERROR _vmnetfs_config_error_quark()
//#define VMNETFS_FUSE_ERROR _vmnetfs_fuse_error_quark()
//#define VMNETFS_IO_ERROR _vmnetfs_io_error_quark()
//#define VMNETFS_STREAM_ERROR _vmnetfs_stream_error_quark()
//#define VMNETFS_TRANSPORT_ERROR _vmnetfs_transport_error_quark()
//
//enum VMNetFSConfigError {
//    VMNETFS_CONFIG_ERROR_INVALID_ARGUMENT,
//};
//
//enum VMNetFSFUSEError {
//    VMNETFS_FUSE_ERROR_FAILED,
//    VMNETFS_FUSE_ERROR_BAD_MOUNTPOINT,
//};
//
//enum VMNetFSIOError {
//    VMNETFS_IO_ERROR_EOF,
//    VMNETFS_IO_ERROR_PREMATURE_EOF,
//    VMNETFS_IO_ERROR_INVALID_CACHE,
//    VMNETFS_IO_ERROR_INTERRUPTED,
//};
//
//enum VMNetFSStreamError {
//    VMNETFS_STREAM_ERROR_NONBLOCKING,
//    VMNETFS_STREAM_ERROR_CLOSED,
//};
//
//enum VMNetFSTransportError {
//    VMNETFS_TRANSPORT_ERROR_FATAL,
//    VMNETFS_TRANSPORT_ERROR_NETWORK,
//};

/* fuse */
void _cachefs_fuse_new(struct cachefs *fs, GError **err);
void _cachefs_fuse_run();
void _cachefs_fuse_terminate();
void _cachefs_fuse_free();

/* redis */
bool _redis_init(const char *address, int port);
void _redis_close();
int _redis_get_attr(const char* path, char** ret_buf);
int _redis_get_readdir(const char* path, GSList *ret_list);


//struct vmnetfs_fuse_dentry *_vmnetfs_fuse_add_dir(
//        struct vmnetfs_fuse_dentry *parent, const char *name);
//void _vmnetfs_fuse_add_file(struct vmnetfs_fuse_dentry *parent,
//        const char *name, const struct vmnetfs_fuse_ops *ops, void *ctx);
//void _vmnetfs_fuse_image_populate(struct vmnetfs_fuse_dentry *dir,
//        struct vmnetfs_image *img);
//void _vmnetfs_fuse_stats_populate(struct vmnetfs_fuse_dentry *dir,
//        struct vmnetfs_image *img);
//void _vmnetfs_fuse_stream_populate(struct vmnetfs_fuse_dentry *dir,
//        struct vmnetfs_image *img);
//bool _vmnetfs_interrupted(void);

#endif
