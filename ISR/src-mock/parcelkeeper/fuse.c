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

#include <sys/utsname.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#define FUSE_USE_VERSION 26
#include <fuse.h>
#include "defs.h"
#include "fuse_defs.h"
#include "config.h"

enum fuse_directory {
	DIR_ROOT,
	DIR_STATS,
};

static const int ignored_signals[]={SIGINT, SIGTERM, SIGUSR1, SIGUSR2,
			SIGTSTP, SIGTTOU, 0};
static const int caught_signals[]={SIGQUIT, SIGHUP, 0};

static void fuse_signal_handler(int sig)
{
	sigstate.signal = sig;
	if (sigstate.fuse != NULL)
		fuse_exit(sigstate.fuse->fuse);
}

static int do_getattr(const char *path, struct stat *st)
{
	struct pk_state *state = fuse_get_context()->private_data;
	gchar *value;

	st->st_nlink = 1;
	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_size = 0;
	st->st_atime = st->st_mtime = st->st_ctime = time(NULL);

	if (g_str_equal(path, "/")) {
		st->st_nlink = 3;
		st->st_mode = S_IFDIR | 0500;
	} else if (g_str_equal(path, "/stats")) {
		st->st_nlink = 2;
		st->st_mode = S_IFDIR | 0500;
	} else if (g_str_equal(path, "/image")) {
		st->st_mode = S_IFREG | 0600;
		st->st_size = ((off_t) state->parcel->chunks) *
					state->parcel->chunksize;
	} else if (g_str_has_prefix(path, "/stats/")) {
		/* Statistics file */
		value = stat_get(state, path + strlen("/stats/"));
		if (value == NULL)
			return -ENOENT;
		st->st_mode = S_IFREG | 0400;
		st->st_size = strlen(value);
		g_free(value);
	} else {
		return -ENOENT;
	}
	st->st_blocks = (st->st_size + 511) / 512;
	return 0;
}

static int do_truncate(const char *path, off_t len)
{
	struct stat st;
	int ret;

	ret = do_getattr(path, &st);
	if (ret) {
		return ret;
	} else if (S_ISDIR(st.st_mode)) {
		return -EISDIR;
	} else if (g_str_equal(path, "/image")) {
		/* Pretend to truncate the image file.  This allows
		   "dd of=image" to work without "conv=notrunc". */
		return 0;
	} else {
		return -EPERM;
	}
}

static int do_open(const char *path, struct fuse_file_info *fi)
{
	struct pk_state *state = fuse_get_context()->private_data;
	int fh;

	if (g_str_has_prefix(path, "/stats/")) {
		/* Statistics file */
		fh = stat_open(state, path + strlen("/stats/"));
		if (fh < 0)
			return fh;
		fi->fh = fh;
	} else if (!g_str_equal(path, "/image")) {
		return -ENOENT;
	}
	return 0;
}

static int do_read(const char *path, char *buf, size_t count, off_t start,
			struct fuse_file_info *fi)
{
	struct pk_state *state = fuse_get_context()->private_data;

	if (fi->fh)
		return stat_read(state, fi->fh, buf, start, count);
	else
		return image_read(state, buf, start, count);
}

static int do_write(const char *path, const char *buf, size_t count,
			off_t start, struct fuse_file_info *fi)
{
	struct pk_state *state = fuse_get_context()->private_data;

	g_assert(fi->fh == 0);
	return image_write(state, buf, start, count);
}

static int do_statfs(const char *path, struct statvfs *st)
{
	struct pk_state *state = fuse_get_context()->private_data;
	unsigned validchunks;

	if (cache_count_chunks(state, &validchunks, NULL))
		return -EIO;
	st->f_bsize = state->parcel->chunksize;
	st->f_blocks = state->parcel->chunks;
	st->f_bfree = st->f_bavail = state->parcel->chunks - validchunks;
	st->f_namemax = 256;
	return 0;
}

static int do_release(const char *path, struct fuse_file_info *fi)
{
	struct pk_state *state = fuse_get_context()->private_data;

	if (fi->fh)
		stat_release(state, fi->fh);
	return 0;
}

static int do_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	struct pk_state *state = fuse_get_context()->private_data;
	int ret;

	if (fi->fh)
		return -EINVAL;
	/* Write out dirty chunks */
	image_sync(state);
	/* Synchronize the cache file.  Note that we do not synchronize the
	   SQLite databases. */
	if (datasync)
		ret = fdatasync(state->cache_fd);
	else
		ret = fsync(state->cache_fd);
	if (ret)
		return -errno;
	return 0;
}

static int do_opendir(const char *path, struct fuse_file_info *fi)
{
	if (g_str_equal(path, "/"))
		fi->fh = DIR_ROOT;
	else if (g_str_equal(path, "/stats"))
		fi->fh = DIR_STATS;
	else
		return -ENOENT;
	return 0;
}

static int do_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			off_t off, struct fuse_file_info *fi)
{
	struct pk_state *state = fuse_get_context()->private_data;
	gchar **stats;
	gchar **cur;

	switch (fi->fh) {
	case DIR_ROOT:
		filler(buf, "image", NULL, 0);
		filler(buf, "stats", NULL, 0);
		return 0;
	case DIR_STATS:
		for (cur = stats = stat_list(state); *cur != NULL; cur++)
			filler(buf, *cur, NULL, 0);
		g_strfreev(stats);
		return 0;
	default:
		return -EIO;
	}
}

static const struct fuse_operations pk_fuse_ops = {
	.getattr = do_getattr,
	.truncate = do_truncate,
	.open = do_open,
	.read = do_read,
	.write = do_write,
	.statfs = do_statfs,
	.release = do_release,
	.fsync = do_fsync,
	.opendir = do_opendir,
	.readdir = do_readdir,
#ifdef HAVE_FUSE_NULLPATH_OK
	.flag_nullpath_ok = 1,
#endif
};

pk_err_t fuse_init(struct pk_state *state)
{
	pk_err_t ret;
	struct utsname utsname;
	GPtrArray *argv;
	struct fuse_args args;
	gchar *str;

	/* Check for previous unclean shutdown of local cache */
	if (cache_test_flag(state, CA_F_DIRTY)) {
		pk_log(LOG_WARNING, "Local cache marked as dirty");
		pk_log(LOG_WARNING, "Will not run until the cache has been "
					"validated or discarded");
		return PK_BADFORMAT;
	}

	/* Log kernel version */
	if (uname(&utsname))
		pk_log(LOG_ERROR, "Can't get kernel version");
	else
		pk_log(LOG_INFO, "%s %s (%s) on %s", utsname.sysname,
					utsname.release, utsname.version,
					utsname.machine);

	/* Log FUSE version */
	pk_log(LOG_INFO, "FUSE version %d", fuse_version());

	/* Set up data structures */
	state->fuse = g_slice_new0(struct pk_fuse);
	stat_init(state);
	ret = image_init(state);
	if (ret)
	        goto bad_dealloc;

	/* Create mountpoint and canonical symlink.  We mount the filesystem
	   off of /var/tmp because mounting it in $HOME will cause Nautilus
	   and the Gtk file browser to show an icon for the filesystem. */
	state->fuse->mountpoint = g_strdup("/var/tmp/isr-vfs-XXXXXX");
	if (mkdtemp(state->fuse->mountpoint) == NULL) {
		pk_log(LOG_ERROR, "Couldn't create FUSE mountpoint");
		ret = PK_CALLFAIL;
		goto bad_dealloc_mountpoint;
	}
	unlink(state->conf->vfspath);
	if (symlink(state->fuse->mountpoint, state->conf->vfspath)) {
		pk_log(LOG_ERROR, "Couldn't create FUSE symlink");
		ret = PK_CALLFAIL;
		goto bad_rmdir;
	}

	/* Set the dirty flag on the local cache.  If the damaged flag is
	   already set, there's no point in forcing another check if we
	   crash. */
	if (!cache_test_flag(state, CA_F_DAMAGED)) {
		ret = cache_set_flag(state, CA_F_DIRTY);
		if (ret)
			goto bad_unlink;
	}

	/* Build FUSE command line */
	argv = g_ptr_array_new();
	g_ptr_array_add(argv, g_strdup("-odefault_permissions"));
	g_ptr_array_add(argv, g_strdup_printf("-ofsname=openisr#%s",
				state->parcel->uuid));
	g_ptr_array_add(argv, g_strdup("-osubtype=openisr"));
#ifdef FUSE_CAP_BIG_WRITES
	/* Older versions of libfuse don't support this option. */
	g_ptr_array_add(argv, g_strdup("-obig_writes"));
#endif
	if (state->conf->flags & WANT_ALLOW_ROOT) {
		/* This option is needed for certain VMMs which run their
		   monitor process as root.  The "user_allow_other" option
		   must be specified in /etc/fuse.conf or fuse_mount()
		   will fail. */
		g_ptr_array_add(argv, g_strdup("-oallow_root"));
	}
	g_ptr_array_add(argv, NULL);
	args.argv = (gchar **) g_ptr_array_free(argv, FALSE);
	args.argc = g_strv_length(args.argv);
	args.allocated = 0;
	str = g_strjoinv(" ", args.argv);
	pk_log(LOG_INFO, "Arguments: %s", str);
	g_free(str);

	/* Initialize FUSE */
	state->fuse->chan = fuse_mount(state->fuse->mountpoint, &args);
	if (state->fuse->chan == NULL) {
		pk_log(LOG_WARNING, "Couldn't mount FUSE filesystem");
		g_strfreev(args.argv);
		ret = PK_IOERR;
		goto bad_unflag;
	}
	state->fuse->fuse = fuse_new(state->fuse->chan, &args, &pk_fuse_ops,
				sizeof(pk_fuse_ops), state);
	g_strfreev(args.argv);
	if (state->fuse->fuse == NULL) {
		pk_log(LOG_ERROR, "Couldn't create FUSE filesystem");
		ret = PK_CALLFAIL;
		goto bad_unmount;
	}

	/* Register FUSE-specific signal handler */
	sigstate.fuse = state->fuse;
	ret = setup_signal_handlers(fuse_signal_handler, caught_signals,
				ignored_signals);
	if (ret)
		goto bad_destroy_fuse;
	/* If there's already a signal pending from the generic handlers,
	   make sure we respect it */
	if (pending_signal())
		fuse_exit(state->fuse->fuse);

	pk_log(LOG_INFO, "Initialized FUSE");
	return PK_SUCCESS;

bad_destroy_fuse:
	sigstate.fuse = NULL;
	fuse_destroy(state->fuse->fuse);
bad_unmount:
	fuse_unmount(state->fuse->mountpoint, state->fuse->chan);
bad_unflag:
	cache_clear_flag(state, CA_F_DIRTY);
bad_unlink:
	if (unlink(state->conf->vfspath))
		pk_log(LOG_ERROR, "Couldn't remove %s", state->conf->vfspath);
bad_rmdir:
	if (rmdir(state->fuse->mountpoint))
		pk_log(LOG_ERROR, "Couldn't remove %s",
					state->fuse->mountpoint);
bad_dealloc_mountpoint:
	g_free(state->fuse->mountpoint);
	image_shutdown(state);
bad_dealloc:
	stat_shutdown(state, FALSE);
	g_slice_free(struct pk_fuse, state->fuse);
	return ret;
}

void fuse_run(struct pk_state *state)
{
	int sig;

	if (state->conf->flags & WANT_SINGLE_THREAD)
		fuse_loop(state->fuse->fuse);
	else
		fuse_loop_mt(state->fuse->fuse);
	sig = pending_signal();
	if (sig)
		pk_log(LOG_INFO, "Caught signal %d, shutting down FUSE "
					"immediately", sig);
	else
		pk_log(LOG_INFO, "Shutting down FUSE");
	if (unlink(state->conf->vfspath))
		pk_log(LOG_ERROR, "Couldn't remove FUSE symlink");
	/* Normally the filesystem will already have been unmounted.  Try
	   to make sure. */
	fuse_unmount(state->fuse->mountpoint, state->fuse->chan);
	if (rmdir(state->fuse->mountpoint)) {
		/* FUSE doesn't return an error code if umount fails, so
		   detect it here */
		pk_log(LOG_ERROR, "Couldn't unmount FUSE filesystem");
	}
}

void fuse_shutdown(struct pk_state *state)
{
	sigstate.fuse = NULL;
	fuse_destroy(state->fuse->fuse);
	image_shutdown(state);
	stat_shutdown(state, TRUE);
	if (!state->fuse->leave_dirty)
		cache_clear_flag(state, CA_F_DIRTY);
	fsync(state->cache_fd);
	g_free(state->fuse->mountpoint);
	g_slice_free(struct pk_fuse, state->fuse);
}
