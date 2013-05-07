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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include "cachefs-private.h"


#define DEBUG_FUSE
#ifdef DEBUG_FUSE
#define DPRINTF(fmt, ...) \
    do { \
    	fprintf(stdout, "[DEBUG][fuse] " fmt, ## __VA_ARGS__); \
    	fprintf(stdout, "\n"); fflush(stdout); \
    } while (0) 
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

static const char *hello_str = "Hello World!\n";
static const char *hello_path = "/hello";

/* internal utility methods */
static bool parse_stinfo(const char *buf, bool *is_local, struct stat *stbuf)
{
	gchar *st_key;
	guint64 st_value = -1;

    gchar **components;
    gchar **cur;
	gchar *end;
	components = g_strsplit(buf, ",", 0);
	if (!components){
		return false;
	}

	for (cur = components; *cur != NULL; cur++) {
		gchar **each_stinfo= g_strsplit(*cur, ":", 0);
		if ((*each_stinfo == NULL) || (*(each_stinfo+1) == NULL)){
			return false;
		}
		st_key = *(each_stinfo+0);
		st_value = g_ascii_strtoull(*(each_stinfo+1), &end, 10);
		if (*(each_stinfo+1) == end) {
			// string conversion failed
			return false;
		}
		if (strcmp(st_key, "atime") == 0){
			stbuf->st_atime = st_value;
		} else if (strcmp(st_key, "ctime") == 0){
			stbuf->st_ctime = st_value;
		} else if (strcmp(st_key, "mtime") == 0){
			stbuf->st_mtime = st_value;
		} else if (strcmp(st_key, "mode") == 0){
			stbuf->st_mode = st_value;
		} else if (strcmp(st_key, "gid") == 0){
			stbuf->st_gid = st_value;
		} else if (strcmp(st_key, "uid") == 0){
			stbuf->st_uid = st_value;
		} else if (strcmp(st_key, "nlink") == 0){
			stbuf->st_nlink= st_value;
		} else if (strcmp(st_key, "size") == 0){
			stbuf->st_size = st_value;
		} else if (strcmp(st_key, "exists") == 0){
			*is_local = st_value;
		} else {
			return false;
		}
		g_strfreev(each_stinfo);
	}
    g_strfreev(components);
    return true;
}

static char* convert_to_relpath(const char* url_root, const char* path)
{
	int url_root_len = strlen(url_root);
	char *rel_path = (char*)malloc(strlen(path)+url_root_len+1);
	rel_path[strlen(path)+url_root_len] = '\0';
	if (strcmp(path, "/") == 0){
		// remove '/'
		memset(rel_path, '\0', strlen(path)+url_root_len+1);
		memcpy(rel_path, url_root, url_root_len);
	}else{
		memcpy(rel_path, url_root, url_root_len);
		memcpy(rel_path+url_root_len, path, strlen(path));
	}

	return rel_path;
}


/* FUSE operation */

static int do_getattr(const char *path, struct stat *stbuf)
{
    struct cachefs *fs= fuse_get_context()->private_data;
	int res = 0;
	char *ret_buf = NULL;
	char* rel_path = convert_to_relpath(fs->url_root, path);

	memset(stbuf, 0, sizeof(struct stat));
	//DPRINTF("request getattr : %s (%s)", path, rel_path);
	if (_redis_get_attr(rel_path, &ret_buf) != EXIT_SUCCESS){
		return -ENOENT;
	}
	if (ret_buf == NULL){
		return -ENOENT;
	}

	//DPRINTF("ret getattr : %s --> %s", rel_path, ret_buf);
	bool is_local = false;
	if (!parse_stinfo(ret_buf, &is_local, stbuf)){
		return -ENOENT;
	}

	if (is_local){ // cached 
		free(ret_buf);
		return res;
	}else{
		// TODO:TO BE IMPLEMENTED
		DPRINTF("TO BE IMPLEMENTED");
	}
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
		off_t offset, struct fuse_file_info *fi)
{
    struct cachefs *fs= fuse_get_context()->private_data;
	int i = 0;
	(void) offset;
	(void) fi;

	char* rel_path = convert_to_relpath(fs->url_root, path);
    DPRINTF("readdir : %s", rel_path);
	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

    GSList *dirlist = NULL;
    if(_redis_get_readdir(rel_path, &dirlist) == EXIT_SUCCESS){
		for(i = 0; i < g_slist_length(dirlist); i++){
			gpointer dirname = g_slist_nth_data(dirlist, i);
			DPRINTF("readir : %s", (char *)dirname);
			filler(buf, dirname, NULL, 0);
		}
		g_slist_free(dirlist);
	}else{
    	DPRINTF("FAILED");
    	free(rel_path);
    	return -ENOENT;
	}

	free(rel_path);
	return 0;
}

static int do_open(const char *path, struct fuse_file_info *fi)
{
    struct cachefs *fs= fuse_get_context()->private_data;
	char* rel_path = convert_to_relpath(fs->url_root, path);
	bool is_exists = false;

	DPRINTF("open existance : %s (%s)", path, rel_path);
	if (_redis_file_exists(rel_path, &is_exists) != EXIT_SUCCESS){
		DPRINTF("[error] failed to check redis for open %s", rel_path);
		return -ENOENT;
	}
	if (is_exists == false){
		DPRINTF("[error] %s does not exists at redis", rel_path);
		return -ENOENT;
	}

	if((fi->flags & 3) != O_RDONLY)
		return -EACCES;

	return 0;
}

static int do_read(const char *path, char *buf, size_t size, off_t offset,
		struct fuse_file_info *fi)
{
    struct cachefs *fs= fuse_get_context()->private_data;
	char* rel_path = convert_to_relpath(fs->url_root, path);
    struct stat stbuf;
    uint64_t end = 0;
	int res = 0;
	char *ret_buf = NULL;

	memset(&stbuf, 0, sizeof(struct stat));
	DPRINTF("file existance : %s (%s)", path, rel_path);
	bool is_exists = false;
	if (_redis_file_exists(rel_path, &is_exists) != EXIT_SUCCESS){
		DPRINTF("[error] failed to check redis %s", rel_path);
		return -ENOENT;
	}
	if (is_exists == false){
		DPRINTF("[error] %s does not exists at redis", rel_path);
		return -ENOENT;
	}
	if (_redis_get_attr(rel_path, &ret_buf) != EXIT_SUCCESS){
		DPRINTF("[error] attribute does not exists : %s ", rel_path);
		return -ENOENT;
	}
	if (ret_buf == NULL){
		DPRINTF("[error] redis returned attribute is null for %s ", rel_path);
		return -ENOENT;
	}

	// check file is at local
	bool is_local = false;
	if (!parse_stinfo(ret_buf, &is_local, &stbuf)){
		DPRINTF("[error] cannot parser stinfor for %s ", ret_buf);
		return -ENOENT;
	}

	// check request validity
    if (offset > stbuf.st_size)
        return 0;
    if (offset + size > stbuf.st_size)
        size = stbuf.st_size - offset;

	free(ret_buf);
	if (is_local){ // cached 
		// get absolute path for the file
		const char* cache_root = fs->cache_root;
		char *abspath = g_strdup_printf("%s/%s", cache_root, rel_path);
		DPRINTF("abs path to read : %s", abspath);

		if (_cachefs_safe_pread(abspath, buf, size, offset) == true){
			g_free(abspath);
			return size;
		} else{
			g_free(abspath);
			DPRINTF("cannot read from file %s", abspath);
			return -EINVAL;
		}
	}else{
		// TODO: TO BE IMPLEMENTED
		DPRINTF("TO BE IMPLEMENTED");
		return -EINVAL;
	}
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
    fs->fuse = fuse_new(fs->chan, &args, &fuse_ops, sizeof(fuse_ops), fs);
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

