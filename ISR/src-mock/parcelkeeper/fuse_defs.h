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

#ifndef PK_FUSE_DEFS_H
#define PK_FUSE_DEFS_H

/* Shared header for source files in the FUSE module. */

struct pk_fuse {
	/* Fileystem handles */
	struct fuse *fuse;
	struct fuse_chan *chan;
	gchar *mountpoint;

	/* Chunk cache */
	struct {
		GMutex *lock;
		GHashTable *chunks;
		GQueue *dirty;
		GQueue *reclaimable;
		GCond *reclaimable_cond;
		unsigned allocatable;
		gboolean stopping;
	} image;
	struct {
		GThread *thread;
		GCond *cond;
	} cleaner;

	/* Open statistics files */
	GHashTable *stat_buffers;
	GMutex *stat_buffer_lock;

	/* Leave the local cache file dirty flag set at shutdown to force
	   the cache to be checked */
	gboolean leave_dirty;
};

/* fuse_image.c */
pk_err_t image_init(struct pk_state *state);
void image_shutdown(struct pk_state *state);
int image_read(struct pk_state *state, char *buf, off_t start, size_t count);
int image_write(struct pk_state *state, const char *buf, off_t start,
			size_t count);
void image_sync(struct pk_state *state);

/* fuse_stats.c */
gchar **stat_list(struct pk_state *state);
gchar *stat_get(struct pk_state *state, const char *name);
int stat_open(struct pk_state *state, const char *name);
int stat_read(struct pk_state *state, int fh, char *buf, off_t start,
			size_t count);
void stat_release(struct pk_state *state, int fh);
void stat_init(struct pk_state *state);
void stat_shutdown(struct pk_state *state, gboolean normal);

#endif
