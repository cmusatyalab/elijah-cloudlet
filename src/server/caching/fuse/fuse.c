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

#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include "cachefs-private.h"

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

/* FUSE operation */
static int do_getattr(const char *path, struct stat *stbuf)
{
	int res = 0;
	memset(stbuf, 0, sizeof(struct stat));
	if(strcmp(path, "/") == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
	}
	else if(strcmp(path, hello_path) == 0) {
		stbuf->st_mode = S_IFREG | 0444;
		stbuf->st_nlink = 1;
		stbuf->st_size = strlen(hello_str);
	}
	else
		res = -ENOENT;

	return res;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	if(strcmp(path, "/") != 0)
		return -ENOENT;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);
	filler(buf, hello_path + 1, NULL, 0);

	return 0;
}

static int do_open(const char *path, struct fuse_file_info *fi)
{
	if(strcmp(path, hello_path) != 0)
		return -ENOENT;

	if((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int do_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
	size_t len;
	(void) fi;
	if(strcmp(path, hello_path) != 0)
		return -ENOENT;

	len = strlen(hello_str);
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, hello_str + offset, size);
	} else
		size = 0;

	return size;
}

static const struct fuse_operations fuse_ops = {
    .getattr = do_getattr,
    .readdir = do_readdir,
    .open = do_open,
    .read = do_read,
    .flag_nullpath_ok = 1,
};


/* FUSE operation */
void _cachefs_fuse_new(struct cachefs *fs, GError **err)
{
	// TODO: clean-up error message
    GPtrArray *argv;
    struct fuse_args args;

    /* Construct mountpoint */
    fs->mountpoint = g_strdup("/var/tmp/cloudlet-cachefs-XXXXXX");
    if (mkdtemp(fs->mountpoint) == NULL) {
        //g_set_error(err, VMNETFS_FUSE_ERROR,
        //        VMNETFS_FUSE_ERROR_BAD_MOUNTPOINT,
        //        "Could not create mountpoint: %s", strerror(errno));
        goto bad_dealloc;
    }

    /* Build FUSE command line */
    argv = g_ptr_array_new();
    g_ptr_array_add(argv, g_strdup("-odefault_permissions"));
	//g_ptr_array_add(argv, g_strdup("-oallow_root"));
    g_ptr_array_add(argv, g_strdup_printf("-ofsname=cachefs#%d", getpid()));
    g_ptr_array_add(argv, g_strdup("-osubtype=cachefs"));
    g_ptr_array_add(argv, g_strdup("-obig_writes"));
    g_ptr_array_add(argv, g_strdup("-ointr"));
    /* Avoid kernel page cache in order to preserve semantics of read()
       and write() return values. */
    g_ptr_array_add(argv, g_strdup("-odirect_io"));
    g_ptr_array_add(argv, NULL);
    args.argv = (gchar **) g_ptr_array_free(argv, FALSE);
    args.argc = g_strv_length(args.argv);
    args.allocated = 0;

    /* Initialize FUSE */
    fs->chan = fuse_mount(fs->mountpoint, &args);
    if (fs->chan == NULL) {
        //g_set_error(err, VMNETFS_FUSE_ERROR, VMNETFS_FUSE_ERROR_FAILED,
        //        "Couldn't mount FUSE filesystem");
        //g_strfreev(args.argv);
        goto bad_rmdir;
    }
    fs->fuse = fuse_new(fs->chan, &args, &fuse_ops, sizeof(fuse_ops), NULL);
    g_strfreev(args.argv);
    if (fs->fuse == NULL) {
        //g_set_error(err, VMNETFS_FUSE_ERROR, VMNETFS_FUSE_ERROR_FAILED,
        //        "Couldn't create FUSE filesystem");
        goto bad_unmount;
    }

    return;

bad_unmount:
    fuse_unmount(fs->mountpoint, fs->chan);
bad_rmdir:
    rmdir(fs->mountpoint);
bad_dealloc:
    g_free(fs->mountpoint);
    return;
}

void _cachefs_fuse_run(struct cachefs *fs)
{
    fuse_loop_mt(fs->fuse);
}

void _cachefs_fuse_terminate(struct cachefs *fs)
{
    char *argv[] = {"fusermount", "-uqz", "--", fs->mountpoint, NULL};

    /* swallow errors */
    g_spawn_sync("/", argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL,
            NULL, NULL, NULL);
}

void _cachefs_fuse_free(struct cachefs *fs)
{
    if (fs->fuse == NULL) {
        return;
    }

    /* Normally the filesystem will already have been unmounted.  Try
       to make sure. */
    fuse_unmount(fs->mountpoint, fs->chan);
    fuse_destroy(fs->fuse);
    rmdir(fs->mountpoint);
    g_free(fs->mountpoint);
}

