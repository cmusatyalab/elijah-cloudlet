/*
 * vmnetfs - virtual machine network execution virtual filesystem
 *
 * Copyright (C) 2006-2012 Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * COPYING.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
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
#include "vmnetfs-private.h"

struct vmnetfs_fuse_dentry {
    const struct vmnetfs_fuse_ops *ops;
    GHashTable *children;
    uint32_t nlink;
    void *ctx;
};

static int dir_getattr(void *dentry_ctx G_GNUC_UNUSED, struct stat *st)
{
    st->st_mode = S_IFDIR | 0500;
    return 0;
}

static const struct vmnetfs_fuse_ops directory_ops = {
    .getattr = dir_getattr,
};

static void dentry_free(struct vmnetfs_fuse_dentry *dentry)
{
    if (dentry->children) {
        g_hash_table_destroy(dentry->children);
    }
    g_slice_free(struct vmnetfs_fuse_dentry, dentry);
}

struct vmnetfs_fuse_dentry *_vmnetfs_fuse_add_dir(
        struct vmnetfs_fuse_dentry *parent, const char *name)
{
    struct vmnetfs_fuse_dentry *dentry;

    dentry = g_slice_new0(struct vmnetfs_fuse_dentry);
    dentry->ops = &directory_ops;
    dentry->children = g_hash_table_new_full(g_str_hash, g_str_equal, g_free,
            (GDestroyNotify) dentry_free);
    dentry->nlink = 2;
    dentry->ctx = dentry;
    if (parent != NULL) {
        parent->nlink++;
        g_hash_table_insert(parent->children, g_strdup(name), dentry);
    }
    return dentry;
}

void _vmnetfs_fuse_add_file(struct vmnetfs_fuse_dentry *parent,
        const char *name, const struct vmnetfs_fuse_ops *ops, void *ctx)
{
    struct vmnetfs_fuse_dentry *dentry;

    dentry = g_slice_new0(struct vmnetfs_fuse_dentry);
    dentry->ops = ops;
    dentry->nlink = 1;
    dentry->ctx = ctx;
    g_hash_table_insert(parent->children, g_strdup(name), dentry);
}

static struct vmnetfs_fuse_dentry *lookup(struct vmnetfs_fuse *fuse,
        const char *path)
{
    struct vmnetfs_fuse_dentry *dentry = fuse->root;
    gchar **components;
    gchar **cur;

    if (g_str_equal(path, "/")) {
        return dentry;
    }

    components = g_strsplit(path, "/", 0);
    /* Drop leading slash */
    for (cur = components + 1; *cur != NULL; cur++) {
        if (dentry->children == NULL) {
            /* Not a directory */
            dentry = NULL;
            break;
        }
        dentry = g_hash_table_lookup(dentry->children, *cur);
        if (dentry == NULL) {
            /* No such dentry */
            break;
        }
    }
    g_strfreev(components);
    return dentry;
}

static int do_getattr(const char *path, struct stat *st)
{
    struct vmnetfs_fuse *fuse = fuse_get_context()->private_data;
    struct vmnetfs_fuse_dentry *dentry;
    int ret;

    dentry = lookup(fuse, path);
    if (dentry == NULL) {
        return -ENOENT;
    }
    if (dentry->ops->getattr == NULL) {
        return -ENOSYS;
    }

    st->st_nlink = dentry->nlink;
    st->st_uid = getuid();
    st->st_gid = getgid();
    st->st_size = 0;
    st->st_atime = st->st_mtime = st->st_ctime = time(NULL);

    ret = dentry->ops->getattr(dentry->ctx, st);
    st->st_blocks = (st->st_size + 511) / 512;
    return ret;
}

static int do_truncate(const char *path, off_t length)
{
    struct vmnetfs_fuse *fuse = fuse_get_context()->private_data;
    struct vmnetfs_fuse_dentry *dentry;

    dentry = lookup(fuse, path);
    if (dentry == NULL) {
        return -ENOENT;
    }
    if (dentry->children != NULL) {
        return -EISDIR;
    }
    if (dentry->ops->truncate) {
        return dentry->ops->truncate(dentry->ctx, length);
    } else {
        return -ENOSYS;
    }
}

static int do_open(const char *path, struct fuse_file_info *fi)
{
    struct vmnetfs_fuse *fuse = fuse_get_context()->private_data;
    struct vmnetfs_fuse_dentry *dentry;
    struct vmnetfs_fuse_fh *fh;
    int ret;

    dentry = lookup(fuse, path);
    if (dentry == NULL) {
        return -ENOENT;
    }
    if (dentry->ops->open == NULL) {
        return -ENOSYS;
    }

    fh = g_slice_new0(struct vmnetfs_fuse_fh);
    fh->ops = dentry->ops;
    fh->blocking = !(fi->flags & O_NONBLOCK);
    ret = dentry->ops->open(dentry->ctx, fh);
    if (ret) {
        g_slice_free(struct vmnetfs_fuse_fh, fh);
    } else {
        fi->fh = (uintptr_t) fh;
        fi->nonseekable = fh->ops->nonseekable;
    }
    return ret;
}

static int do_read(const char *path G_GNUC_UNUSED, char *buf, size_t count,
        off_t start, struct fuse_file_info *fi)
{
    struct vmnetfs_fuse_fh *fh = (void *) (uintptr_t) fi->fh;

    if (fh->ops->read) {
        return fh->ops->read(fh, buf, start, count);
    } else {
        return -ENOSYS;
    }
}

static int do_write(const char *path G_GNUC_UNUSED, const char *buf,
        size_t count, off_t start, struct fuse_file_info *fi)
{
    struct vmnetfs_fuse_fh *fh = (void *) (uintptr_t) fi->fh;

    if (fh->ops->write) {
        return fh->ops->write(fh, buf, start, count);
    } else {
        return -ENOSYS;
    }
}

static int do_poll(const char *path G_GNUC_UNUSED, struct fuse_file_info *fi,
        struct fuse_pollhandle *ph, unsigned *reventsp)
{
    struct vmnetfs_fuse_fh *fh = (void *) (uintptr_t) fi->fh;
    bool readable = false;
    int ret;

    if (fh->ops->poll) {
        ret = fh->ops->poll(fh, ph, &readable);
        if (readable) {
            *reventsp = POLLIN;
        } else {
            *reventsp = 0;
        }
        return ret;
    } else {
        return -ENOSYS;
    }
}

static int do_release(const char *path G_GNUC_UNUSED,
        struct fuse_file_info *fi)
{
    struct vmnetfs_fuse_fh *fh = (void *) (uintptr_t) fi->fh;

    if (fh->ops->release) {
        fh->ops->release(fh);
    }
    g_slice_free(struct vmnetfs_fuse_fh, fh);
    return 0;
}

static int do_opendir(const char *path, struct fuse_file_info *fi)
{
    struct vmnetfs_fuse *fuse = fuse_get_context()->private_data;
    struct vmnetfs_fuse_dentry *dentry;

    dentry = lookup(fuse, path);
    if (dentry == NULL) {
        return -ENOENT;
    }
    if (dentry->children == NULL) {
        return -ENOTDIR;
    }
    fi->fh = (uintptr_t) dentry;
    return 0;
}

struct fill_data {
    fuse_fill_dir_t filler;
    void *buf;
    int failed;
};

static void collect_names(char *name,
        struct vmnetfs_fuse_dentry *dentry G_GNUC_UNUSED,
        struct fill_data *fill)
{
    if (!fill->failed) {
        fill->failed = fill->filler(fill->buf, name, NULL, 0);
    }
}

static int do_readdir(const char *path G_GNUC_UNUSED, void *buf,
        fuse_fill_dir_t filler, off_t off G_GNUC_UNUSED,
        struct fuse_file_info *fi)
{
    struct vmnetfs_fuse_dentry *dentry = (void *) (uintptr_t) fi->fh;
    struct fill_data fill = {
        .filler = filler,
        .buf = buf,
        .failed = 0,
    };

    g_assert(dentry->children != NULL);

    fill.failed = filler(buf, ".", NULL, 0);
    if (!fill.failed) {
        fill.failed = filler(buf, "..", NULL, 0);
    }
    g_hash_table_foreach(dentry->children, (GHFunc) collect_names, &fill);
    return fill.failed ? -EIO : 0;
}

static int do_statfs(const char *path G_GNUC_UNUSED, struct statvfs *st)
{
    struct vmnetfs_fuse *fuse = fuse_get_context()->private_data;
    uint64_t image_size;

    image_size = _vmnetfs_io_get_image_size(fuse->fs->disk, NULL);
    if (fuse->fs->memory != NULL) {
        image_size += _vmnetfs_io_get_image_size(fuse->fs->memory, NULL);
    }

    st->f_bsize = 512;
    st->f_blocks = image_size / 512;
    st->f_bfree = st->f_bavail = 0;
    st->f_namemax = 256;
    return 0;
}

static const struct fuse_operations fuse_ops = {
    .getattr = do_getattr,
    .truncate = do_truncate,
    .open = do_open,
    .read = do_read,
    .write = do_write,
    .poll = do_poll,
    .release = do_release,
    .opendir = do_opendir,
    .readdir = do_readdir,
    .statfs = do_statfs,
    .flag_nullpath_ok = 1,
};

static void add_image(struct vmnetfs_fuse_dentry *parent,
        struct vmnetfs_image *img, const char *name)
{
    struct vmnetfs_fuse_dentry *dir;

    dir = _vmnetfs_fuse_add_dir(parent, name);
    _vmnetfs_fuse_image_populate(dir, img);
    _vmnetfs_fuse_stats_populate(dir, img);
    _vmnetfs_fuse_stream_populate(dir, img);
}

struct vmnetfs_fuse *_vmnetfs_fuse_new(struct vmnetfs *fs, GError **err)
{
    struct vmnetfs_fuse *fuse;
    GPtrArray *argv;
    struct fuse_args args;

    /* Set up data structures */
    fuse = g_slice_new0(struct vmnetfs_fuse);
    fuse->fs = fs;
    fuse->root = _vmnetfs_fuse_add_dir(NULL, NULL);
    add_image(fuse->root, fs->disk, "disk");
    if (fs->memory != NULL) {
        add_image(fuse->root, fs->memory, "memory");
    }

    /* Construct mountpoint */
    fuse->mountpoint = g_strdup("/var/tmp/vmnetfs-XXXXXX");
    if (mkdtemp(fuse->mountpoint) == NULL) {
        g_set_error(err, VMNETFS_FUSE_ERROR,
                VMNETFS_FUSE_ERROR_BAD_MOUNTPOINT,
                "Could not create mountpoint: %s", strerror(errno));
        goto bad_dealloc;
    }

    /* Build FUSE command line */
    argv = g_ptr_array_new();
    g_ptr_array_add(argv, g_strdup("-odefault_permissions"));
	//g_ptr_array_add(argv, g_strdup("-oallow_root"));
    g_ptr_array_add(argv, g_strdup_printf("-ofsname=vmnetfs#%d", getpid()));
    g_ptr_array_add(argv, g_strdup("-osubtype=vmnetfs"));
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
    fuse->chan = fuse_mount(fuse->mountpoint, &args);
    if (fuse->chan == NULL) {
        g_set_error(err, VMNETFS_FUSE_ERROR, VMNETFS_FUSE_ERROR_FAILED,
                "Couldn't mount FUSE filesystem");
        g_strfreev(args.argv);
        goto bad_rmdir;
    }
    fuse->fuse = fuse_new(fuse->chan, &args, &fuse_ops, sizeof(fuse_ops),
            fuse);
    g_strfreev(args.argv);
    if (fuse->fuse == NULL) {
        g_set_error(err, VMNETFS_FUSE_ERROR, VMNETFS_FUSE_ERROR_FAILED,
                "Couldn't create FUSE filesystem");
        goto bad_unmount;
    }

    return fuse;

bad_unmount:
    fuse_unmount(fuse->mountpoint, fuse->chan);
bad_rmdir:
    rmdir(fuse->mountpoint);
bad_dealloc:
    g_free(fuse->mountpoint);
    dentry_free(fuse->root);
    g_slice_free(struct vmnetfs_fuse, fuse);
    return NULL;
}

void _vmnetfs_fuse_run(struct vmnetfs_fuse *fuse)
{
    fuse_loop_mt(fuse->fuse);
}

void _vmnetfs_fuse_terminate(struct vmnetfs_fuse *fuse)
{
    char *argv[] = {"fusermount", "-uqz", "--", fuse->mountpoint, NULL};

    /* swallow errors */
    g_spawn_sync("/", argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, NULL,
            NULL, NULL, NULL);
}

void _vmnetfs_fuse_free(struct vmnetfs_fuse *fuse)
{
    if (fuse == NULL) {
        return;
    }
    /* Normally the filesystem will already have been unmounted.  Try
       to make sure. */
    fuse_unmount(fuse->mountpoint, fuse->chan);
    fuse_destroy(fuse->fuse);
    rmdir(fuse->mountpoint);
    g_free(fuse->mountpoint);
    dentry_free(fuse->root);
    g_slice_free(struct vmnetfs_fuse, fuse);
}

/* Return true if the current FUSE request was interrupted. */
bool _vmnetfs_interrupted(void)
{
    return fuse_interrupted();
}
