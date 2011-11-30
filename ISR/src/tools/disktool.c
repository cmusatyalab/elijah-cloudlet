/*
 * disktool - perform server-side operations on parcel disk images
 *
 * Copyright (C) 2009-2010 Carnegie Mellon University
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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "sql.h"
#include "isrutil.h"

/* command line parameters */
static const char *importimage;
static const char *exportimage;
static int64_t expand_chunks;
static const char *destpath = ".";
static const char *keyring = "keyring";
static int chunksize = 128; /* chunk size in KiB */
static int chunksperdir = 512;
static gboolean want_lzf;
static gboolean want_progress;

static GOptionEntry options[] = {
	{"in", 'i', 0, G_OPTION_ARG_FILENAME, &importimage, "Image to import from", "PATH"},
	{"out", 'o', 0, G_OPTION_ARG_FILENAME, &exportimage, "Image to export to", "PATH"},
	{"expand", 'e', 0, G_OPTION_ARG_INT64, &expand_chunks, "Target size of parcel", "CHUNKS"},
	{"directory", 'd', 0, G_OPTION_ARG_FILENAME, &destpath, "Path to parcel version directory (default: .)", "PATH"},
	{"keyring", 'k', 0, G_OPTION_ARG_FILENAME, &keyring, "Keyring (default: keyring)", "PATH"},
	{"chunksize", 's', 0, G_OPTION_ARG_INT, &chunksize, "Chunksize (default: 128)", "KiB"},
	{"chunksperdir", 'm', 0, G_OPTION_ARG_INT, &chunksperdir, "Chunks per directory (default: 512)", "N"},
	{"lzf", 'l', 0, G_OPTION_ARG_NONE, &want_lzf, "Use LZF compression", NULL},
	{"progress", 'p', 0, G_OPTION_ARG_NONE, &want_progress, "Show progress bar", NULL},
	{NULL}
};

static void clear_progress(void);
#define die(str, args...) do { \
		clear_progress(); \
		g_printerr(str "\n", ## args); \
		exit(1); \
	} while(0)

/* encoding parameters */
static enum iu_chunk_crypto crypto = IU_CHUNK_CRY_AES_SHA1;
static enum iu_chunk_compress compressor = IU_CHUNK_COMP_ZLIB;
static unsigned int hash_len;
#define HASH_LEN 20

static struct db *sqlitedb;

static unsigned chunklen;
static gpointer tmpdata;

#define KEYRING_VERSION 1

/** Progress bar *************************************************************/

static FILE *tty;
static off_t progress_bytes;
static off_t progress_max_bytes;
static time_t progress_start;
static gboolean progress_redraw;
static struct winsize window_size;
#define TTYFILE "/dev/tty"

static void set_signal_handler(int sig, void (*handler)(int))
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = handler;
	act.sa_flags = SA_RESTART;
	if (sigaction(sig, &act, NULL))
		die("Couldn't set signal handler for signal %d", sig);
}

static void sigwinch_handler(int ignored)
{
	(void)ignored;

	if (tty == NULL)
		return;
	ioctl(fileno(tty), TIOCGWINSZ, &window_size);
	progress_redraw = TRUE;
}

static unsigned ndigits(unsigned val)
{
	unsigned n;

	for (n=0; val; val /= 10, n++);
	return n;
}

static char *seconds_to_str(unsigned seconds)
{
	if (seconds < 3600)
		return g_strdup_printf("%u:%.2u", seconds / 60,
					seconds % 60);
	else
		return g_strdup_printf("%u:%.2u:%.2u", seconds / 3600,
					(seconds / 60) % 60, seconds % 60);
}

static char *progress_bar(unsigned cols_used, unsigned percent)
{
	char *str;
	int availchars;
	unsigned fillchars;

	availchars = window_size.ws_col - cols_used - 2;
	if (availchars < 2)
		return g_strdup_printf("%*s", availchars + 2, "");
	fillchars = availchars * percent / 100;
	str = g_strdup_printf("[%*s]", availchars, "");
	memset(str + 1, '=', fillchars);
	if (percent < 100)
		str[fillchars + 1] = '>';
	return str;
}

static void print_progress(gboolean final)
{
	static time_t last_timestamp;
	time_t cur_timestamp;
	unsigned long long bytes = progress_bytes;
	unsigned long long max_bytes = progress_max_bytes;
	unsigned percent = 0;
	char *estimate = NULL;
	char *bar;
	int count;

	if (max_bytes == 0)
		return;  /* Progress bar disabled */
	if (final)
		percent = 100;
	else
		percent = MIN(bytes * 100 / max_bytes, 99);

	cur_timestamp = time(NULL);
	if (!final && !progress_redraw && last_timestamp == cur_timestamp)
		return;
	last_timestamp = cur_timestamp;
	progress_redraw = FALSE;

	if (bytes && !final)
		estimate = seconds_to_str((max_bytes - bytes) *
					(cur_timestamp - progress_start)
					/ bytes);

	count = fprintf(tty, " %3u%% (%*llu/%llu MB) %s%s", percent,
				ndigits(max_bytes >> 20), bytes >> 20,
				max_bytes >> 20, estimate ?: "",
				estimate ? " " : "");
	bar = progress_bar(count, percent);
	fprintf(tty, "%s\r", bar);
	if (final)
		fprintf(tty, "\n");
	fflush(tty);
	g_free(estimate);
	g_free(bar);
}

static void init_progress(off_t max_bytes)
{
	progress_max_bytes = 0;
	if (!want_progress || max_bytes == 0)
		return;
	set_signal_handler(SIGWINCH, sigwinch_handler);
	if (tty == NULL) {
		tty = fopen(TTYFILE, "w");
		if (tty == NULL)
			return;
		sigwinch_handler(0);
	}
	progress_bytes = 0;
	progress_max_bytes = max_bytes;
	progress_start = time(NULL);
	print_progress(FALSE);
}

static void progress(off_t count)
{
	progress_bytes += count;
	print_progress(FALSE);
}

static void finish_progress(void)
{
	print_progress(TRUE);
}

static void clear_progress(void)
{
	if (tty != NULL) {
		fprintf(tty, "%*s\r", window_size.ws_col, "");
		fflush(tty);
		progress_redraw = TRUE;
	}
}

static void init_progress_fd(int fd)
{
	off_t imagelen;
	int64_t nchunks;

	imagelen = lseek(fd, 0, SEEK_END);
	if (imagelen == -1)
		imagelen = 0;

	else if (lseek(fd, 0, SEEK_SET))
		die("Couldn't reset position of input stream: %s",
		    strerror(errno));

	nchunks = ((int64_t)imagelen + chunklen - 1) / chunklen;
	init_progress(nchunks * chunklen);
}

/** Initialization *******************************************************/

static void handle_log_message(const gchar *domain, GLogLevelFlags level,
			       const gchar *message, gpointer data)
{
	(void)domain;
	(void)data;

	switch (level & G_LOG_LEVEL_MASK) {
	case G_LOG_LEVEL_INFO:
	case SQL_LOG_LEVEL_QUERY:
	case SQL_LOG_LEVEL_SLOW_QUERY:
		break;
	default:
		clear_progress();
		fprintf(stderr, "%s\n", message);
		break;
	}
}

static void init_keyring(void)
{
	struct query *qry;
	int user_version;

	if (!begin(sqlitedb))
		die("Couldn't begin transaction");
	if (!query(&qry, sqlitedb, "PRAGMA user_version", NULL)) {
		sql_log_err(sqlitedb, "Couldn't query user version");
		goto bad;
	}
	query_row(qry, "d", &user_version);
	query_free(qry);

	switch (user_version) {
	case 0:
		break;
	case KEYRING_VERSION:
		rollback(sqlitedb);
		return;
	default:
		fprintf(stderr, "Unknown keyring version %d", user_version);
		goto bad;
	}

	if (!query(NULL, sqlitedb, "PRAGMA auto_vacuum = 0", NULL)) {
		sql_log_err(sqlitedb, "Couldn't disable auto-vacuum");
		goto bad;
	}
	if (!query(NULL, sqlitedb, "PRAGMA legacy_file_format = ON", NULL)) {
		sql_log_err(sqlitedb, "Couldn't set legacy_file_format");
		goto bad;
	}
	if (!query(NULL, sqlitedb, "PRAGMA user_version = "
				G_STRINGIFY(KEYRING_VERSION), NULL)) {
		sql_log_err(sqlitedb, "Couldn't set schema version");
		goto bad;
	}
	if (!query(NULL, sqlitedb, "CREATE TABLE keys ( "
				"chunk INTEGER PRIMARY KEY NOT NULL, "
				"tag BLOB NOT NULL, "
				"key BLOB NOT NULL, "
				"compression INTEGER NOT NULL)", NULL)) {
		sql_log_err(sqlitedb, "Couldn't create keys table");
		goto bad;
	}
	if (!query(NULL, sqlitedb, "CREATE INDEX keys_tags ON keys (tag)",
				NULL)) {
		sql_log_err(sqlitedb, "Couldn't create keys_tags index");
		goto bad;
	}
	if (!commit(sqlitedb)) {
		sql_log_err(sqlitedb, "Couldn't commit keyring");
		goto bad;
	}
	return;

bad:
	rollback(sqlitedb);
	die("Couldn't initialize keyring");
}

static void init(void)
{
	GString *dbfile = g_string_new("");
	gchar *hdkdir;

	hash_len = iu_chunk_crypto_hashlen(crypto);
	if (hash_len > HASH_LEN)
		die("Unexpected hash size");

	/* allocate an empty chunk to compare against */
	chunklen = chunksize * 1024;
	tmpdata = g_malloc(chunklen);

	/* initialize sqlite */
	g_log_set_handler("isrsql", G_LOG_LEVEL_MASK, handle_log_message,
				NULL);
	sql_init();

	/* initialize isrutil */
	g_log_set_handler("isrutil", G_LOG_LEVEL_MASK, handle_log_message,
				NULL);

	/* make destination directory if it doesn't exist */
	if (!g_file_test(destpath, G_FILE_TEST_IS_DIR))
		if (g_mkdir(destpath, 0700))
			die("Couldn't create %s", destpath);
	/* make hdk subdirectory */
	hdkdir = g_strdup_printf("%s/hdk", destpath);
	if (!g_file_test(hdkdir, G_FILE_TEST_IS_DIR))
		if (g_mkdir(hdkdir, 0755))
			die("Couldn't create %s", hdkdir);
	g_free(hdkdir);

	/* check if keyring path is absolute or relative to cwd */
	if (!((keyring[0] == '/') ||
	      (keyring[0] == '.' && keyring[1] == '/') ||
	      (keyring[0] == '.' && keyring[1] == '.' && keyring[2] == '/')))
	{
		g_string_append(dbfile, destpath);
		g_string_append_c(dbfile, '/');
	}
	g_string_append(dbfile, keyring);
	if (exportimage && !g_file_test(dbfile->str, G_FILE_TEST_IS_REGULAR))
		die("Keyring does not exist");
	if (!sql_conn_open(dbfile->str, &sqlitedb))
		die("Couldn't open keyring");
	g_string_free(dbfile, TRUE);

	/* Initialize DB schema if necessary */
	init_keyring();
}

static void fini(void)
{
	sql_conn_close(sqlitedb);

	g_free(tmpdata);
}

struct chunk_desc {
	gpointer tag;
	gpointer key;
	gpointer data;
	unsigned len;
	unsigned int compression;
};

static void encrypt_chunk(struct chunk_desc *chunk)
{
	gpointer tmp;

	chunk->compression = compressor;
	if (!iu_chunk_encode(crypto, chunk->data, chunk->len, tmpdata,
				&chunk->len, chunk->tag, chunk->key,
				&chunk->compression))
		die("Couldn't encode chunk");
	tmp = tmpdata;
	tmpdata = chunk->data;
	chunk->data = tmp;
}

static void make_chunk_dir(unsigned int chunk_idx)
{
	static unsigned last = -1;
	unsigned dir = chunk_idx / chunksperdir;
	gchar *path;

	if (last == dir)
		return;
	last = dir;
	path = g_strdup_printf("%s/hdk/%04u", destpath, dir);
	if (!g_file_test(path, G_FILE_TEST_IS_DIR))
		if (g_mkdir(path, 0755))
			die("Couldn't create directory %s", path);
	g_free(path);
}

static gchar *form_chunk_path(unsigned int idx)
{
	return g_strdup_printf("%s/hdk/%04u/%04u", destpath,
				idx / chunksperdir, idx % chunksperdir);
}

static void write_chunk(unsigned int idx, struct chunk_desc *chunk)
{
	gchar *dest;
	int fd;

	make_chunk_dir(idx);
	dest = form_chunk_path(idx);
	fd = g_creat(dest, 0444);
	if (fd == -1)
		die("Failed to create chunk #%d: %s", idx, strerror(errno));
	if (write(fd, chunk->data, chunk->len) != (ssize_t)chunk->len)
		die("Failed to write chunk #%d: %s", idx, strerror(errno));
	close(fd);

	/* update keyring */
	if (!query(NULL, sqlitedb,
		    "INSERT INTO keys (chunk, tag, key, compression) "
		    "VALUES (?, ?, ?, ?)", "dbbd", idx, chunk->tag,
		    hash_len, chunk->key, hash_len, chunk->compression))
	{
		sql_log_err(sqlitedb, "Couldn't update keyring");
		exit(1);
	}
	g_free(dest);
}

static void read_chunk(unsigned int idx, struct chunk_desc *chunk)
{
	gchar *dest;
	int fd;
	unsigned inlen;

	dest = form_chunk_path(idx);
	fd = g_open(dest, O_RDONLY, 0);
	if (fd == -1)
		die("Failed to open chunk #%u: %s", idx, strerror(errno));
	inlen = read(fd, tmpdata, chunklen);
	close(fd);
	g_free(dest);

	if (!iu_chunk_decode(crypto, chunk->compression, idx, tmpdata,
				inlen, chunk->key, chunk->data, chunklen))
		die("Couldn't decode chunk %u", idx);
	chunk->len = chunklen;
}

/* Return TRUE if the first len bytes of buf are zero, FALSE otherwise. */
static gboolean is_zero(void *buf, unsigned len)
{
	unsigned unaligned_mask;
	unsigned count_head;
	unsigned count_body;
	unsigned count_tail;
	char *cp;
	long *lp;

	unaligned_mask = sizeof(long) - 1;
	/* 1. If buf is one byte past an aligned offset, count_head should be
	      sizeof(long) - 1 bytes.
	   2. If buf is aligned, count_head should be zero.
	   3. count_head should never be greater than len. */
	count_head = MIN((sizeof(long) - ((long) buf & unaligned_mask)) &
				unaligned_mask, len);
	count_body = (len - count_head) / sizeof(long);
	count_tail = len - count_head - sizeof(long) * count_body;
	cp = buf;
	while (count_head--)
		if (*cp++)
			return FALSE;
	lp = (long *) cp;
	while (count_body--)
		if (*lp++)
			return FALSE;
	cp = (char *) lp;
	while (count_tail--)
		if (*cp++)
			return FALSE;
	return TRUE;
}

static void import_image(const gchar *img)
{
	int fd;
	unsigned int idx;
	ssize_t n;
	struct chunk_desc chunk, zerochunk;

	fd = g_open(img, O_RDONLY, 0);
	if (fd == -1)
		die("unable to open image: %s", strerror(errno));

	chunk.data = g_malloc(chunklen);
	chunk.tag = g_malloc(hash_len);
	chunk.key = g_malloc(hash_len);

	zerochunk.len = chunklen;
	zerochunk.data = g_malloc0(chunklen);
	zerochunk.tag = g_malloc(hash_len);
	zerochunk.key = g_malloc(hash_len);
	encrypt_chunk(&zerochunk);

	init_progress_fd(fd);

	if (!begin(sqlitedb))
		die("Couldn't begin transaction");
	for (idx = 0; (n = read(fd, chunk.data, chunklen)) > 0; idx++)
	{
		/* zero tail of a partial (last) chunk */
		if ((unsigned)n < chunklen)
			memset(chunk.data + n, 0, chunklen - n);
		chunk.len = chunklen;

		if (is_zero(chunk.data, chunklen)) {
			write_chunk(idx, &zerochunk);
		} else {
			encrypt_chunk(&chunk);
			write_chunk(idx, &chunk);
		}
		progress(chunklen);
	}
	if (n < 0) {
		rollback(sqlitedb);
		die("Error reading image file");
	}
	if (!commit(sqlitedb)) {
		rollback(sqlitedb);
		die("Couldn't commit transaction");
	}

	finish_progress();

	close(fd);

	g_free(zerochunk.data);
	g_free(zerochunk.tag);
	g_free(zerochunk.key);
	g_free(chunk.data);
	g_free(chunk.tag);
	g_free(chunk.key);
}

static void export_image(const gchar *img)
{
	int fd, skip;
	unsigned int idx, nchunk;
	struct chunk_desc chunk;
	struct query *qry;
	ssize_t n;
	gboolean write_zeros;

	fd = g_creat(img, 0600);
	if (fd == -1)
		die("unable to create image: %s", strerror(errno));

	chunk.data = g_malloc(chunklen);

	if (!begin(sqlitedb))
		die("Couldn't begin transaction");

	if (!query(&qry, sqlitedb, "SELECT COUNT(*) FROM keys", NULL)) {
		sql_log_err(sqlitedb, "Couldn't enumerate keyring");
		rollback(sqlitedb);
		exit(1);
	}
	query_row(qry, "d", &nchunk);
	query_free(qry);

	init_progress((off_t)nchunk * chunklen);

	/* if the image is a disk or pipe ftruncate will fail and we should
	 * write out zero blocks */
	write_zeros = (ftruncate(fd, 0) != 0);
	if (!write_zeros && ftruncate(fd, (off_t) nchunk * chunklen))
		die("Couldn't resize image file: %s", strerror(errno));

	for (query(&qry, sqlitedb, "SELECT chunk, tag, key, compression "
				"FROM keys ORDER BY chunk", NULL), idx = 0;
				query_has_row(sqlitedb);
				query_next(qry), idx++) {
		unsigned int tmp0, tmp1, tmp2;
		query_row(qry, "dbbd", &tmp0, &chunk.tag, &tmp1,
			  &chunk.key, &tmp2, &chunk.compression);

		if (tmp0 != idx)
			die("missing chunk %u", idx);

		if (tmp1 != hash_len || tmp2 != hash_len)
			die("incorrect tag or key length");

		read_chunk(idx, &chunk);

		skip = !write_zeros && is_zero(chunk.data, chunklen);
		if (!skip) {
			n = write(fd, chunk.data, chunklen);
			if (n != (ssize_t)chunklen)
				die("Failed to write to image file: %s",
				    strerror(errno));
		} else {
			if (lseek(fd, chunklen, SEEK_CUR) == (off_t)-1)
				die("lseek failed");
		}

		progress(chunklen);
	}
	query_free(qry);
	if (!query_ok(sqlitedb)) {
		sql_log_err(sqlitedb, "Select failed");
		rollback(sqlitedb);
		exit(1);
	}
	if (!commit(sqlitedb)) {
		rollback(sqlitedb);
		die("Couldn't commit transaction");
	}

	close(fd);

	finish_progress();

	g_free(chunk.data);
}

static void expand_parcel(void)
{
	struct query *qry;
	struct chunk_desc zerochunk;
	unsigned int existing;
	unsigned int idx;

	zerochunk.len = chunklen;
	zerochunk.data = g_malloc0(chunklen);
	zerochunk.tag = g_malloc(hash_len);
	zerochunk.key = g_malloc(hash_len);
	encrypt_chunk(&zerochunk);

	if (!begin(sqlitedb))
		die("Couldn't begin transaction");

	if (!query(&qry, sqlitedb, "SELECT count(*) FROM keys", NULL)) {
		sql_log_err(sqlitedb, "Couldn't enumerate keyring");
		rollback(sqlitedb);
		exit(1);
	}
	query_row(qry, "d", &existing);
	query_free(qry);

	if (expand_chunks > existing)
		init_progress((expand_chunks - existing) * chunklen);

	for (idx = existing; idx < expand_chunks; idx++) {
		write_chunk(idx, &zerochunk);
		progress(chunklen);
	}
	if (!commit(sqlitedb)) {
		rollback(sqlitedb);
		die("Couldn't commit transaction");
	}

	finish_progress();

	g_free(zerochunk.data);
	g_free(zerochunk.tag);
	g_free(zerochunk.key);
}

int main(int argc, char **argv)
{
	GOptionContext *ctx;
	GError *err = NULL;

	ctx = g_option_context_new(" - generate/import/export VM disk image");
	g_option_context_add_main_entries(ctx, options, NULL);
	if (!g_option_context_parse(ctx, &argc, &argv, &err))
		die("%s", err->message);
	g_option_context_free(ctx);

	if (expand_chunks < 0)
		die("Invalid argument to --expand");

	if (chunksize <= 0)
		die("Invalid chunksize specified");

	if (chunksperdir <= 0)
		die("Invalid number of chunks per directory specified");

	if (!!importimage + !!exportimage + !!expand_chunks != 1)
		die("Specify one of --in, --out, or --expand");

	if (want_lzf)
		compressor = IU_CHUNK_COMP_LZF;

	init();

	if (importimage)
		import_image(importimage);
	else if (exportimage)
		export_image(exportimage);
	else
		expand_parcel();

	fini();

	exit(0);
}

