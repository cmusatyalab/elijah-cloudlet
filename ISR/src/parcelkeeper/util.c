/*
 * Parcelkeeper - support daemon for the OpenISR (R) system virtual disk
 *
 * Copyright (C) 2006-2009 Carnegie Mellon University
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <uuid/uuid.h>
#include "isrcrypto.h"
#include "defs.h"

#define UUID_STR_LEN 36  /* not including trailing NUL */

struct pk_lockfile {
	gchar *path;
	int fd;
};

pk_err_t parseuint(unsigned *out, const char *in, int base)
{
	unsigned long val;
	char *endptr;

	val=strtoul(in, &endptr, base);
	if (*in == 0 || *endptr != 0)
		return PK_INVALID;
	/* XXX can overflow */
	*out=(unsigned)val;
	return PK_SUCCESS;
}

pk_err_t read_file(const char *path, gchar **buf, gsize *len)
{
	GError *err=NULL;
	pk_err_t ret;

	if (g_file_get_contents(path, buf, len, &err))
		return PK_SUCCESS;
	switch (err->code) {
	case G_FILE_ERROR_NOENT:
		ret=PK_NOTFOUND;
		break;
	case G_FILE_ERROR_NOMEM:
		ret=PK_NOMEM;
		break;
	default:
		ret=PK_IOERR;
		break;
	}
	g_error_free(err);
	return ret;
}

char *pk_strerror(pk_err_t err)
{
	switch (err) {
	case PK_SUCCESS:
		return "Success";
	case PK_OVERFLOW:
		return "Buffer too small for data";
	case PK_IOERR:
		return "I/O error";
	case PK_NOTFOUND:
		return "Object not found";
	case PK_INVALID:
		return "Invalid parameter";
	case PK_NOMEM:
		return "Out of memory";
	case PK_NOKEY:
		return "No such key in keyring";
	case PK_TAGFAIL:
		return "Tag did not match data";
	case PK_BADFORMAT:
		return "Invalid format";
	case PK_CALLFAIL:
		return "Call failed";
	case PK_PROTOFAIL:
		return "Driver protocol error";
	case PK_NETFAIL:
		return "Network failure";
	case PK_BUSY:
		return "Object busy";
	case PK_SQLERR:
		return "SQL error";
	case PK_INTERRUPT:
		return "Interrupted";
	}
	return "(Unknown)";
}

int set_signal_handler(int sig, void (*handler)(int sig))
{
	struct sigaction sa = {};
	sa.sa_handler=handler;
	sa.sa_flags=SA_RESTART;
	return sigaction(sig, &sa, NULL);
}

pk_err_t setup_signal_handlers(void (*caught_handler)(int sig),
			const int *caught_signals, const int *ignored_signals)
{
	int i;

	if (caught_signals != NULL) {
		for (i=0; caught_signals[i] != 0; i++) {
			if (set_signal_handler(caught_signals[i],
						caught_handler)) {
				pk_log(LOG_ERROR, "unable to register signal "
						"handler for signal %d",
						caught_signals[i]);
				return PK_CALLFAIL;
			}
		}
	}
	if (ignored_signals != NULL) {
		for (i=0; ignored_signals[i] != 0; i++) {
			if (set_signal_handler(ignored_signals[i], SIG_IGN)) {
				pk_log(LOG_ERROR, "unable to ignore signal %d",
						ignored_signals[i]);
				return PK_CALLFAIL;
			}
		}
	}
	return PK_SUCCESS;
}

static void interrupter(void *data, void *set)
{
	struct db *db = data;

	if (set)
		query_interrupt(db);
	else
		query_clear_interrupt(db);
}

void interrupter_add(struct db *db)
{
	sigstate.interrupter_dbs = g_list_prepend(sigstate.interrupter_dbs, db);
}

void interrupter_clear(void)
{
	g_list_foreach(sigstate.interrupter_dbs, interrupter, (void *)FALSE);
	g_list_free(sigstate.interrupter_dbs);
	sigstate.interrupter_dbs = NULL;
}

void generic_signal_handler(int sig)
{
	sigstate.signal=sig;
	g_list_foreach(sigstate.interrupter_dbs, interrupter, (void *)TRUE);
}

int pending_signal(void)
{
	static int warned;

	if (sigstate.signal && !warned) {
		warned=1;
		pk_log(LOG_INFO, "Interrupt");
	}
	return sigstate.signal;
}

void print_progress_chunks(unsigned chunks, unsigned maxchunks)
{
	static time_t last_timestamp;
	time_t cur_timestamp;
	unsigned percent;

	if (maxchunks)
		percent=chunks*100/maxchunks;
	else
		percent=0;

	/* Don't talk if we've talked recently */
	cur_timestamp=time(NULL);
	if (last_timestamp == cur_timestamp)
		return;
	last_timestamp=cur_timestamp;

	/* Note carriage return rather than newline */
	printf("  %u%% (%u/%u)\r", percent, chunks, maxchunks);
	fflush(stdout);
}

void print_progress_mb(off64_t bytes, off64_t max_bytes)
{
	static time_t last_timestamp;
	time_t cur_timestamp;
	unsigned percent;
	unsigned long long mb = bytes >> 20;
	unsigned long long max_mb = max_bytes >> 20;

	if (max_mb)
		percent=mb*100/max_mb;
	else
		percent=0;

	/* Don't talk if we've talked recently */
	cur_timestamp=time(NULL);
	if (last_timestamp == cur_timestamp)
		return;
	last_timestamp=cur_timestamp;

	/* Note carriage return rather than newline */
	printf("  %u%% (%llu/%llu MB)\r", percent, mb, max_mb);
	fflush(stdout);
}

static pk_err_t file_lock(int fd, int op, short locktype)
{
	struct flock lock = {
		.l_type   = locktype,
		.l_whence = SEEK_SET,
		.l_start  = 0,
		.l_len    = 0
	};

	while (fcntl(fd, op, &lock) == -1) {
		if (errno == EACCES || errno == EAGAIN)
			return PK_BUSY;
		else if (errno != EINTR)
			return PK_CALLFAIL;
	}
	return PK_SUCCESS;
}

pk_err_t get_file_lock(int fd, int flags)
{
	return file_lock(fd, (flags & FILE_LOCK_WAIT) ? F_SETLKW : F_SETLK,
				(flags & FILE_LOCK_WRITE) ? F_WRLCK : F_RDLCK);
}

pk_err_t put_file_lock(int fd)
{
	return file_lock(fd, F_SETLK, F_UNLCK);
}

/* Create lock file.  flock locks don't work over NFS; byterange locks don't
   work over AFS; and dotlocks are difficult to check for freshness.  So
   we use a whole-file fcntl lock.  The lock shouldn't become stale because the
   kernel checks that for us; however, over NFS file systems without a lock
   manager, locking will fail.  For safety, we treat that as an error. */
pk_err_t acquire_lockfile(struct pk_lockfile **out, const char *path)
{
	struct pk_lockfile *lf;
	int fd;
	struct stat st;
	pk_err_t ret;

	*out = NULL;
	while (1) {
		fd=open(path, O_CREAT|O_WRONLY, 0666);
		if (fd == -1) {
			pk_log(LOG_ERROR, "Couldn't open lock file %s", path);
			return PK_IOERR;
		}
		ret=get_file_lock(fd, FILE_LOCK_WRITE);
		if (ret) {
			close(fd);
			return ret;
		}
		if (fstat(fd, &st)) {
			pk_log(LOG_ERROR, "Couldn't stat lock file %s", path);
			close(fd);
			return PK_CALLFAIL;
		}
		if (st.st_nlink == 1)
			break;
		/* We probably have a lock on a deleted lockfile, which
		   doesn't do anyone any good.  Try again. */
		close(fd);
	}
	lf = g_slice_new(struct pk_lockfile);
	lf->path = g_strdup(path);
	lf->fd = fd;
	*out = lf;
	return PK_SUCCESS;
}

void release_lockfile(struct pk_lockfile *lf)
{
	if (lf == NULL)
		return;
	/* To prevent races, we must unlink the lockfile while we still
	   hold the lock */
	unlink(lf->path);
	g_free(lf->path);
	close(lf->fd);
	g_slice_free(struct pk_lockfile, lf);
}

pk_err_t create_pidfile(const char *path)
{
	FILE *fp;

	fp=fopen(path, "w");
	if (fp == NULL) {
		pk_log(LOG_ERROR, "Couldn't open pid file %s", path);
		return PK_IOERR;
	}
	fprintf(fp, "%d\n", getpid());
	fclose(fp);
	return PK_SUCCESS;
}

/* Fork, and have the parent wait for the child to indicate that the parent
   should exit.  In the parent, this returns only on error.  In the child, it
   returns success and sets *status_fd.  If the child writes a byte to the fd,
   the parent will exit with that byte as its exit status.  If the child closes
   the fd without writing anything, the parent will exit(0). */
pk_err_t fork_and_wait(int *status_fd)
{
	int fds[2];
	pid_t pid;
	char ret=1;
	int status;

	/* Make sure the child isn't killed if the parent dies */
	if (set_signal_handler(SIGPIPE, SIG_IGN)) {
		pk_log(LOG_ERROR, "Couldn't block SIGPIPE");
		return PK_CALLFAIL;
	}
	if (pipe(fds)) {
		pk_log(LOG_ERROR, "Can't create pipe");
		return PK_CALLFAIL;
	}

	pid=fork();
	if (pid == -1) {
		pk_log(LOG_ERROR, "fork() failed");
		return PK_CALLFAIL;
	} else if (pid) {
		/* Parent */
		close(fds[1]);
		if (read(fds[0], &ret, sizeof(ret)) == 0) {
			exit(0);
		} else {
			if (waitpid(pid, &status, 0) == -1)
				exit(ret);
			if (WIFEXITED(status))
				exit(WEXITSTATUS(status));
			if (WIFSIGNALED(status)) {
				set_signal_handler(WTERMSIG(status), SIG_DFL);
				raise(WTERMSIG(status));
			}
			exit(ret);
		}
	} else {
		/* Child */
		close(fds[0]);
		*status_fd=fds[1];
	}
	return PK_SUCCESS;
}

gchar *form_chunk_path(struct pk_parcel *parcel, const char *prefix,
			unsigned chunk)
{
	unsigned dir = chunk / parcel->chunks_per_dir;
	unsigned file = chunk % parcel->chunks_per_dir;

	return g_strdup_printf("%s/%.4u/%.4u", prefix, dir, file);
}

gchar *format_tag(const void *tag, unsigned len)
{
	gchar *buf;
	const unsigned char *tbuf=tag;
	unsigned u;

	buf=g_malloc(2 * len + 1);
	for (u=0; u<len; u++)
		sprintf(buf + 2 * u, "%.2x", tbuf[u]);
	return buf;
}

void log_tag_mismatch(const void *expected, const void *found, unsigned len)
{
	gchar *fmt_expected;
	gchar *fmt_found;

	fmt_expected=format_tag(expected, len);
	fmt_found=format_tag(found, len);
	pk_log(LOG_WARNING, "Expected %s, found %s", fmt_expected, fmt_found);
	g_free(fmt_expected);
	g_free(fmt_found);
}

pk_err_t canonicalize_uuid(const char *in, gchar **out)
{
	uuid_t uuid;

	if (uuid_parse(in, uuid)) {
		pk_log(LOG_ERROR, "Invalid UUID");
		return PK_INVALID;
	}
	if (out != NULL) {
		*out=g_malloc(UUID_STR_LEN + 1);
		uuid_unparse_lower(uuid, *out);
	}
	return PK_SUCCESS;
}

pk_err_t cleanup_action(struct db *db, const char *sql,
			enum pk_log_type logtype, const char *desc)
{
	struct query *qry;
	int changes;

	if (!query(&qry, db, sql, NULL)) {
		sql_log_err(db, "Couldn't clean %s", desc);
		return PK_IOERR;
	}
	query_row(qry, "d", &changes);
	query_free(qry);
	if (changes > 0)
		pk_log(logtype, "Cleaned %d %s", changes, desc);
	return PK_SUCCESS;
}

void _stats_increment(struct pk_state *state, uint64_t *var, uint64_t val)
{
	g_mutex_lock(state->stats_lock);
	*var += val;
	g_mutex_unlock(state->stats_lock);
}
