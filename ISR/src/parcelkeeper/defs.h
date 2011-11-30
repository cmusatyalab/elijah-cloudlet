/*
 * Parcelkeeper - support daemon for the OpenISR (R) system virtual disk
 *
 * Copyright (C) 2006-2011 Carnegie Mellon University
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

#ifndef PK_DEFS_H
#define PK_DEFS_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <glib.h>
#include <pthread.h>
#include "sql.h"
#include "isrutil.h"

typedef enum pk_err {
	PK_SUCCESS=0,
	PK_OVERFLOW,
	PK_IOERR,
	PK_NOTFOUND,
	PK_INVALID,
	PK_NOMEM,
	PK_NOKEY,
	PK_TAGFAIL,
	PK_BADFORMAT,
	PK_CALLFAIL,
	PK_PROTOFAIL,
	PK_NETFAIL,  /* Used instead of IOERR if a retry might fix it */
	PK_BUSY,
	PK_SQLERR,
	PK_INTERRUPT,
} pk_err_t;

enum pk_log_type {
	LOG_INFO,
	LOG_CHUNK,
	LOG_FUSE,
	LOG_TRANSPORT,
	LOG_QUERY,
	LOG_SLOW_QUERY,
	LOG_WARNING,
	LOG_STATS
};
#define _LOG_BACKTRACE (1<<31)
#define LOG_ERROR (LOG_WARNING|_LOG_BACKTRACE)
enum cache_flags {
	CA_F_DIRTY	= 0x0001,  /* Cache was not closed properly */
	CA_F_DAMAGED	= 0x0002,  /* Cache has bad chunks */
};

enum mode {
	MODE_RUN,
	MODE_UPLOAD,
	MODE_HOARD,
	MODE_EXAMINE,
	MODE_VALIDATE,
	MODE_LISTHOARD,
	MODE_CHECKHOARD,
	MODE_RMHOARD,
	MODE_GCHOARD,
	MODE_REFRESH,
	MODE_HELP,
	MODE_VERSION,
};

enum mode_flags {
	WANT_LOCK	= 0x0001,
	WANT_CACHE	= 0x0002,
	WANT_PREV	= 0x0004,
	WANT_BACKGROUND = 0x0008,
	WANT_TRANSPORT	= 0x0010,
	WANT_CHECK	= 0x0020,
	WANT_SHM	= 0x0040,
	WANT_FULL_CHECK	= 0x0080,
	WANT_SPLICE	= 0x0100,
	WANT_COMPACT	= 0x0200,
	WANT_GC		= 0x0400,  /* Enable slower hoard cleanup steps */
	WANT_ALLOW_ROOT	= 0x0800,  /* Allow root to access FUSE FS */
	WANT_SINGLE_THREAD = 0x1000,  /* Run FUSE single-threaded */
};

struct pk_shm;
struct pk_fuse;
struct pk_connection;
struct pk_lockfile;

struct pk_config {
	/* mode data */
	const char *modename;
	unsigned flags;

	/* top-level parcel directory and its contents */
	gchar *parcel_dir;
	gchar *parcel_cfg;
	gchar *keyring;
	gchar *prev_keyring;
	gchar *cache_file;
	gchar *cache_index;
	gchar *vfspath;
	gchar *lockfile;
	gchar *pidfile;

	/* hoard cache and its contents */
	gchar *hoard_dir;
	gchar *hoard_file;
	gchar *hoard_index;

	/* upload directory */
	gchar *dest_dir;

	/* log parameters */
	gchar *log_file;
	unsigned log_file_mask;
	unsigned log_stderr_mask;

	/* miscellaneous parameters */
	enum iu_chunk_compress compress;
	gchar *uuid;
	unsigned chunk_cache; /* MB */
};

struct pk_parcel {
	enum iu_chunk_crypto crypto;
	unsigned required_compress;
	unsigned chunks;
	unsigned chunksize;
	unsigned chunks_per_dir;
	unsigned hashlen;
	gchar *uuid;
	gchar *server;
	gchar *user;
	gchar *parcel;
	gchar *master;
};

struct pk_state {
	struct pk_config *conf;
	struct pk_parcel *parcel;

	struct pk_lockfile *lockfile;
	int cache_fd;
	int hoard_fd;
	struct pk_fuse *fuse;
	struct pk_shm *shm;
	struct pk_connection_pool *cpool;
	struct db *db;
	struct db *hoard;

	int hoard_ident;

	unsigned offset;
	unsigned cache_flags;

	GMutex *stats_lock;
	struct {
		uint64_t bytes_read;
		uint64_t bytes_written;
		uint64_t chunk_reads;
		uint64_t chunk_writes;
		uint64_t chunk_errors;
		uint64_t cache_hits;
		uint64_t cache_misses;
		uint64_t cache_evictions;
		uint64_t cache_evictions_dirty;
		uint64_t data_bytes_written;
		uint64_t whole_chunk_updates;
	} stats;
};

struct pk_sigstate {
	volatile int signal;
	GList *interrupter_dbs;
	struct pk_fuse *fuse;
};

extern struct pk_sigstate sigstate;
extern const char isr_release[];
extern const char rcs_revision[];

/* cmdline.c */
enum mode parse_cmdline(struct pk_config **out, int argc, char **argv);
void cmdline_free(struct pk_config *conf);

/* log.c */
void log_start(const char *path, unsigned file_mask, unsigned stderr_mask);
void log_shutdown(void);
void pk_log(enum pk_log_type type, const char *fmt, ...)
			__attribute__ ((format(printf, 2, 3)));
pk_err_t logtypes_to_mask(const char *list, unsigned *out);

/* parcelcfg.c */
pk_err_t parse_parcel_cfg(struct pk_parcel **out, const char *path);
void parcel_cfg_free(struct pk_parcel *parcel);

/* cache.c */
pk_err_t cache_init(struct pk_state *state);
void cache_shutdown(struct pk_state *state);
pk_err_t _cache_read_chunk(struct pk_state *state, unsigned chunk,
			void *buf, unsigned chunklen, const void *tag);
pk_err_t cache_get(struct pk_state *state, unsigned chunk, void *buf);
pk_err_t cache_update(struct pk_state *state, unsigned chunk, const void *buf);
pk_err_t cache_count_chunks(struct pk_state *state, unsigned *valid,
			unsigned *dirty);
pk_err_t cache_set_flag(struct pk_state *state, unsigned flag);
pk_err_t cache_clear_flag(struct pk_state *state, unsigned flag);
int cache_test_flag(struct pk_state *state, unsigned flag);
void cache_shm_set_dirty(struct pk_state *state, unsigned chunk);
void cache_shm_set_cached(struct pk_state *state, unsigned chunk,
				gboolean cached);
void cache_shm_set_cache_dirty(struct pk_state *state, unsigned chunk,
				gboolean dirty);

/* cache_modes.c */
int copy_for_upload(struct pk_state *state);
int validate_cache(struct pk_state *state);
int examine_cache(struct pk_state *state);

/* fuse.c */
pk_err_t fuse_init(struct pk_state *state);
void fuse_run(struct pk_state *state);
void fuse_shutdown(struct pk_state *state);

/* hoard.c */
pk_err_t hoard_init(struct pk_state *state);
void hoard_shutdown(struct pk_state *state);
pk_err_t _hoard_read_chunk(struct pk_state *state, int offset, int length,
			int crypto, const void *tag, int taglen, void *buf);
pk_err_t _hoard_write_chunk(struct pk_state *state, unsigned offset,
			unsigned length, const void *buf);
pk_err_t hoard_get_chunk(struct pk_state *state, const void *tag, void *buf,
			unsigned *len);
pk_err_t hoard_put_chunk(struct pk_state *state, const void *tag,
			const void *buf, unsigned len);
pk_err_t hoard_sync_refs(struct pk_state *state, gboolean new_chunks);
pk_err_t hoard_gc(struct pk_state *state);
void hoard_invalidate_chunk(struct pk_state *state, int offset,
			const void *tag, unsigned taglen);

/* hoard_modes.c */
int hoard(struct pk_state *state);
int examine_hoard(struct pk_state *state);
int list_hoard(struct pk_state *state);
int rmhoard(struct pk_state *state);
int gchoard(struct pk_state *state);
int check_hoard(struct pk_state *state);
int hoard_refresh(struct pk_state *state);

/* transport.c */
pk_err_t transport_init(void);
struct pk_connection_pool *transport_pool_alloc(struct pk_state *state);
void transport_pool_free(struct pk_connection_pool *cpool);
pk_err_t transport_fetch_chunk(struct pk_connection_pool *cpool, void *buf,
			unsigned chunk, const void *tag, unsigned *length);

/* util.c */
#define FILE_LOCK_READ     0
#define FILE_LOCK_WRITE 0x01
#define FILE_LOCK_WAIT  0x02
pk_err_t parseuint(unsigned *out, const char *in, int base);
pk_err_t read_file(const char *path, gchar **buf, gsize *len);
char *pk_strerror(pk_err_t err);
int set_signal_handler(int sig, void (*handler)(int sig));
pk_err_t setup_signal_handlers(void (*caught_handler)(int sig),
			const int *caught_signals, const int *ignored_signals);
void generic_signal_handler(int sig);
int pending_signal(void);
void interrupter_add(struct db *db);
void interrupter_clear(void);
void print_progress_chunks(unsigned chunks, unsigned maxchunks);
void print_progress_mb(off64_t bytes, off64_t max_bytes);
pk_err_t fork_and_wait(int *status_fd);
pk_err_t get_file_lock(int fd, int flags);
pk_err_t put_file_lock(int fd);
pk_err_t acquire_lockfile(struct pk_lockfile **out, const char *path);
void release_lockfile(struct pk_lockfile *lf);
pk_err_t create_pidfile(const char *path);
gchar *form_chunk_path(struct pk_parcel *parcel, const char *prefix,
			unsigned chunk);
gchar *format_tag(const void *tag, unsigned len);
void log_tag_mismatch(const void *expected, const void *found, unsigned len);
pk_err_t canonicalize_uuid(const char *in, gchar **out);
pk_err_t cleanup_action(struct db *db, const char *sql,
			enum pk_log_type logtype, const char *desc);
void _stats_increment(struct pk_state *state, uint64_t *var, uint64_t val);
#define stats_increment(state, field, count) \
	_stats_increment(state, &(state)->stats.field, count)

#endif
