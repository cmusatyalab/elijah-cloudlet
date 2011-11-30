/*
 * blobtool - encode/decode file data
 *
 * Copyright (C) 2007-2009 Carnegie Mellon University
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <ftw.h>
#include <termios.h>
#include <glib.h>
#include <archive.h>
#include <archive_entry.h>
#include "isrcrypto.h"

/* For libarchive 1.2 */
#ifndef ARCHIVE_EXTRACT_SECURE_SYMLINKS
#define ARCHIVE_EXTRACT_SECURE_SYMLINKS 0
#endif
#ifndef ARCHIVE_EXTRACT_SECURE_NODOTDOT
#define ARCHIVE_EXTRACT_SECURE_NODOTDOT 0
#endif

#define BUFSZ 32768
#define TTYFILE "/dev/tty"
#define SALT_MAGIC "Salted__"
#define SALT_LEN 8
#define ENC_HEADER_LEN (strlen(SALT_MAGIC) + SALT_LEN)
#define FTW_FDS 16
#define ARCHIVE_EXTRACT_FLAGS  (ARCHIVE_EXTRACT_TIME |\
				ARCHIVE_EXTRACT_UNLINK |\
				ARCHIVE_EXTRACT_SECURE_SYMLINKS |\
				ARCHIVE_EXTRACT_SECURE_NODOTDOT)

/* Crypto parameters */
static enum isrcry_cipher cipher = ISRCRY_CIPHER_AES;
static unsigned cipher_block = 16;
static unsigned keylen = 16;
static enum isrcry_mode mode = ISRCRY_MODE_CBC;
static enum isrcry_padding padding = ISRCRY_PADDING_PKCS5;
static enum isrcry_hash hash = ISRCRY_HASH_SHA1;
static unsigned hashlen = 20;
static gboolean detect_compress = TRUE;		/* Only for decode */
static gboolean use_internal_compress = FALSE;
static enum external_compress {
	EXTERNAL_COMPRESS_NONE,
	EXTERNAL_COMPRESS_GZIP,
} external_compress = EXTERNAL_COMPRESS_GZIP;	/* Only for encode */
static enum isrcry_compress internal_compress;

/* Command-line parameters */
static int keyroot_fd;
static const char *keyroot;
static const char *infile;
static const char *outfile;
static const char *parent_dir;
static const char *compress_alg;
static gboolean encode = TRUE;
static gboolean want_encrypt;
static gboolean want_hash;
static gboolean want_tar;
static gboolean want_progress;

/** Utility ******************************************************************/

struct iodata {
	FILE *infp;
	FILE *outfp;
	GString *in;
	GString *out;
};

static void clear_progress(void);

#define warn(str, args...) do { \
		clear_progress(); \
		g_printerr("blobtool: " str "\n", ## args); \
	} while (0)

#define die(str, args...) do { \
		clear_progress(); \
		g_printerr("blobtool: " str "\n", ## args); \
		exit(1); \
	} while (0)

static void *expand_string(GString *str, unsigned len)
{
	unsigned offset = str->len;

	g_string_set_size(str, offset + len);
	return str->str + offset;
}

static void swap_strings(struct iodata *iod, gboolean truncate_out)
{
	GString *tmp;

	tmp = iod->in;
	iod->in = iod->out;
	iod->out = tmp;
	if (truncate_out)
		g_string_truncate(iod->out, 0);
}

static void set_signal_handler(int sig, void (*handler)(int))
{
	struct sigaction act;

	memset(&act, 0, sizeof(act));
	act.sa_handler = handler;
	act.sa_flags = SA_RESTART;
	if (sigaction(sig, &act, NULL))
		die("Couldn't set signal handler for signal %d", sig);
}

/** Progress bar *************************************************************/

static FILE *tty;
static off_t progress_bytes;
static off_t progress_max_bytes;
static time_t progress_start;
static gboolean progress_redraw;
static struct winsize window_size;

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
		if (tty == NULL) {
			warn("Couldn't open " TTYFILE);
			return;
		}
		sigwinch_handler(0);
	}
	progress_bytes = 0;
	progress_max_bytes = max_bytes;
	progress_start = time(NULL);
	print_progress(FALSE);
}

static void init_progress_stream(FILE *fp)
{
	if (fseeko(fp, 0, SEEK_END)) {
		/* Make sure progress bar is disabled */
		init_progress(0);
	} else {
		init_progress(ftello(fp));
		if (fseeko(fp, 0, SEEK_SET))
			die("Couldn't reset position of input stream: %s",
						strerror(errno));
	}
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

/** Cipher *******************************************************************/

static void get_keyroot(void)
{
	GString *str;
	char buf[32];
	ssize_t ret;

	str = g_string_new("");
	while ((ret = read(keyroot_fd, buf, sizeof(buf))) > 0)
		g_string_append_len(str, buf, ret);
	if (ret == -1)
		die("Failed to read keyroot fd %d", keyroot_fd);
	keyroot = g_strchomp(g_string_free(str, FALSE));
}

/* enc(1)-compatible key derivation */
static void set_cipher_key(struct isrcry_cipher_ctx *ctx, const char *salt)
{
	struct isrcry_hash_ctx *hash;
	unsigned hashlen = isrcry_hash_len(ISRCRY_HASH_MD5);
	char buf[keylen + cipher_block + hashlen];  /* key + IV + overflow */
	char *key = buf;
	char *iv = buf + keylen;
	char *cur = buf;
	int remaining;
	unsigned krlen = strlen(keyroot);
	enum isrcry_result ret;

	hash = isrcry_hash_alloc(ISRCRY_HASH_MD5);
	if (hash == NULL)
		die("Couldn't allocate MD5 hash");
	for (remaining = keylen + cipher_block; remaining > 0;
				remaining -= hashlen, cur += hashlen) {
		if (cur > buf)
			isrcry_hash_update(hash, cur - hashlen, hashlen);
		isrcry_hash_update(hash, keyroot, krlen);
		isrcry_hash_update(hash, salt, SALT_LEN);
		isrcry_hash_final(hash, cur);
	}
	isrcry_hash_free(hash);
	ret = isrcry_cipher_init(ctx, encode ? ISRCRY_ENCRYPT : ISRCRY_DECRYPT,
				key, keylen, iv);
	if (ret)
		die("Couldn't initialize cipher: %s", isrcry_strerror(ret));
}

static void init_cipher(struct isrcry_cipher_ctx *ctx, const char **in,
			unsigned *inlen, GString *out)
{
	struct isrcry_random_ctx *rand;
	char salt[SALT_LEN];

	get_keyroot();
	if (encode) {
		rand = isrcry_random_alloc();
		if (rand == NULL)
			die("Couldn't allocate random context");
		isrcry_random_bytes(rand, salt, sizeof(salt));
		isrcry_random_free(rand);
		g_string_append(out, SALT_MAGIC);
		g_string_append_len(out, salt, sizeof(salt));
		set_cipher_key(ctx, salt);
	} else {
		if (*inlen < ENC_HEADER_LEN)
			die("Couldn't read header of encrypted data");
		if (memcmp(*in, SALT_MAGIC, strlen(SALT_MAGIC)))
			die("Invalid magic string in encrypted data");
		*in += strlen(SALT_MAGIC);
		set_cipher_key(ctx, *in);
		*in += SALT_LEN;
		*inlen -= ENC_HEADER_LEN;
	}
}

static void run_cipher(const char *in, unsigned inlen, GString *out,
			gboolean final)
{
	static struct isrcry_cipher_ctx *ctx;
	static void *partial;
	static unsigned offset;
	char finalbuf[2 * cipher_block];
	unsigned count;
	unsigned outlen;
	enum isrcry_result ret;

	if (ctx == NULL) {
		ctx = isrcry_cipher_alloc(cipher, mode);
		if (ctx == NULL)
			die("Couldn't allocate cipher");
		partial = g_malloc(cipher_block);
		init_cipher(ctx, &in, &inlen, out);
	}
	/* We always hold a block in reserve for isrcry_cipher_final().  This
	   means that we never run a block through isrcry_cipher_process()
	   unless at least one more byte is pending. */
	if (offset) {
		count = MIN(cipher_block - offset, inlen);
		memcpy(partial + offset, in, count);
		offset += count;
		in += count;
		inlen -= count;
		if (offset == cipher_block && inlen > 0) {
			isrcry_cipher_process(ctx, partial, cipher_block,
					expand_string(out, cipher_block));
			offset = 0;
		}
	}
	if (inlen / cipher_block) {
		count = (inlen / cipher_block) * cipher_block;
		if (!(inlen % cipher_block))
			count -= cipher_block;
		isrcry_cipher_process(ctx, in, count,
					expand_string(out, count));
		in += count;
		inlen -= count;
	}
	g_assert(inlen <= cipher_block);
	memcpy(partial, in, inlen);
	offset += inlen;
	if (final) {
		outlen = sizeof(finalbuf);
		ret = isrcry_cipher_final(ctx, padding, partial, offset,
					finalbuf, &outlen);
		if (ret)
			die("Couldn't finalize cipher: %s",
						isrcry_strerror(ret));
		g_string_append_len(out, finalbuf, outlen);
	}
}

/** Hash *********************************************************************/

static void run_hash(const char *in, unsigned inlen, GString *out,
			gboolean final)
{
	static struct isrcry_hash_ctx *ctx;
	unsigned char result[hashlen];
	unsigned n;

	if (ctx == NULL) {
		ctx = isrcry_hash_alloc(hash);
		if (ctx == NULL)
			die("Couldn't allocate hash");
	}
	isrcry_hash_update(ctx, in, inlen);
	if (final) {
		isrcry_hash_final(ctx, result);
		for (n = 0; n < hashlen; n++)
			g_string_append_printf(out, "%.2x", result[n]);
		g_string_append_c(out, '\n');
	}
}

/** Compression **************************************************************/

static const struct compress_desc {
	const char *name;
	gboolean internal;
	enum external_compress external_type;
	enum isrcry_compress internal_type;
	unsigned magiclen;
	const char magic[6];
} compress_algs[] = {
	{"none", FALSE, EXTERNAL_COMPRESS_NONE},
	{"gzip", FALSE, EXTERNAL_COMPRESS_GZIP},
	{"lzf", TRUE, 0, ISRCRY_COMPRESS_LZF_STREAM, 2, "ZV"},
	{"lzma", TRUE, 0, ISRCRY_COMPRESS_LZMA, 6, {0xfd, '7', 'z', 'X', 'Z', 0}},
	{NULL}
};

/* Only called for encode */
static void parse_compress_alg(void)
{
	const struct compress_desc *desc;

	if (compress_alg == NULL)
		return;  /* Use defaults */
	for (desc = compress_algs; desc->name != NULL; desc++) {
		if (!strcmp(compress_alg, desc->name)) {
			use_internal_compress = desc->internal;
			external_compress = desc->external_type;
			internal_compress = desc->internal_type;
			return;
		}
	}
	die("Unknown compression algorithm: %s", compress_alg);
}

/* Only called for decode */
static void detect_compression(const char *in, unsigned inlen)
{
	const struct compress_desc *desc;

	detect_compress = FALSE;
	for (desc = compress_algs; desc->name != NULL; desc++) {
		/* We only care about internal algorithms, since external
		   algorithms will be detected by libarchive. */
		if (!desc->internal || inlen < desc->magiclen)
			continue;
		if (!memcmp(in, desc->magic, desc->magiclen)) {
			use_internal_compress = TRUE;
			internal_compress = desc->internal_type;
			break;
		}
	}
}

/* Only called for internal compression */
static void run_compression(const char *in, unsigned inlen, GString *out,
			gboolean final)
{
	static struct isrcry_compress_ctx *ctx;
	unsigned in_offset = 0;
	unsigned in_count;
	unsigned out_offset = 0;
	unsigned out_count;
	enum isrcry_result ret;

	if (ctx == NULL) {
		ctx = isrcry_compress_alloc(internal_compress);
		if (ctx == NULL)
			die("Couldn't allocate compression algorithm");
		ret = isrcry_compress_init(ctx, encode ? ISRCRY_ENCODE :
					ISRCRY_DECODE, 0);
		if (ret)
			die("Couldn't initialize compression: %s",
						isrcry_strerror(ret));
	}

	do {
		in_count = inlen - in_offset;
		out_count = BUFSZ;
		g_string_set_size(out, out_offset + BUFSZ);
		ret = isrcry_compress_process(ctx, in + in_offset, &in_count,
					out->str + out_offset, &out_count);
		if (ret)
			die("Compression failed: %s", isrcry_strerror(ret));
		in_offset += in_count;
		out_offset += out_count;
	} while (in_offset < inlen);

	if (final) {
		do {
			in_count = 0;
			out_count = BUFSZ;
			g_string_set_size(out, out_offset + BUFSZ);
			ret = isrcry_compress_final(ctx, NULL, &in_count,
						out->str + out_offset,
						&out_count);
			out_offset += out_count;
		} while (ret == ISRCRY_BUFFER_OVERFLOW);
		if (ret)
			die("Finalizing compression failed: %s",
						isrcry_strerror(ret));
	}

	g_string_set_size(out, out_offset);
}

/** Generic control **********************************************************/

#define action(name) do { \
		run_ ## name(iod->in->str, iod->in->len, iod->out, final); \
		swap_strings(iod, TRUE); \
	} while (0)
static void run_buffer(struct iodata *iod, gboolean final)
{
	if (encode) {
		if (want_tar && use_internal_compress)
			action(compression);
		if (want_encrypt)
			action(cipher);
	} else {
		if (want_encrypt)
			action(cipher);
		if (want_tar) {
			/* detect_compression() just peeks in the input
			   buffer, so we don't use action() */
			if (detect_compress)
				detect_compression(iod->in->str, iod->in->len);
			if (use_internal_compress)
				action(compression);
		}
	}
	if (want_hash)
		action(hash);
	/* Now the output is positioned as the input buffer.  Fix this with
	   one final swap. */
	swap_strings(iod, FALSE);
}
#undef action

static void run_stream(struct iodata *iod)
{
	size_t len;

	init_progress_stream(iod->infp);
	do {
		g_string_set_size(iod->in, BUFSZ);
		g_string_truncate(iod->out, 0);
		len = fread(iod->in->str, 1, iod->in->len, iod->infp);
		if (ferror(iod->infp))
			die("Error reading input");
		g_string_set_size(iod->in, len);
		run_buffer(iod, feof(iod->infp));
		fwrite(iod->out->str, 1, iod->out->len, iod->outfp);
		if (ferror(iod->outfp))
			die("Error writing output");
		progress(len);
	} while (!feof(iod->infp));
	finish_progress();
}

/** Tar control **************************************************************/

/* tar I/O uses a different main loop than non-tar I/O, since we're not just
   processing one FD into another. */

static ssize_t archive_read(struct archive *arch, void *data,
			const void **buffer)
{
	struct iodata *iod = data;
	size_t len;

	/* At EOF, run_buffer() may or may not produce some output.  If not,
	   we return 0 immediately.  But if it does, we won't return 0 until
	   the next time we're called.  If we're not at EOF, we must not
	   return 0. */
	if (feof(iod->infp))
		return 0;
	do {
		g_string_set_size(iod->in, BUFSZ);
		g_string_truncate(iod->out, 0);
		len = fread(iod->in->str, 1, iod->in->len, iod->infp);
		if (ferror(iod->infp)) {
			archive_set_error(arch, EIO, "Error reading input");
			return ARCHIVE_FATAL;
		}
		g_string_set_size(iod->in, len);
		run_buffer(iod, feof(iod->infp));
		progress(len);
	} while (iod->out->len == 0 && !feof(iod->infp));
	*buffer = iod->out->str;
	return iod->out->len;
}

static ssize_t archive_write(struct archive *arch, void *data,
			const void *buffer, size_t length)
{
	struct iodata *iod = data;

	g_string_truncate(iod->in, 0);
	g_string_truncate(iod->out, 0);
	g_string_append_len(iod->in, buffer, length);
	run_buffer(iod, FALSE);
	if (fwrite(iod->out->str, 1, iod->out->len, iod->outfp) <
				iod->out->len) {
		archive_set_error(arch, EIO, "Error writing output");
		return ARCHIVE_FATAL;
	}
	return length;
}

static int archive_finish_write(struct archive *arch, void *data)
{
	struct iodata *iod = data;

	g_string_truncate(iod->in, 0);
	g_string_truncate(iod->out, 0);
	run_buffer(iod, TRUE);
	if (fwrite(iod->out->str, 1, iod->out->len, iod->outfp) <
				iod->out->len) {
		archive_set_error(arch, EIO, "Error writing final output");
		return ARCHIVE_FATAL;
	}
	return ARCHIVE_OK;
}

static void read_archive(struct iodata *iod)
{
	struct archive *arch;
	struct archive_entry *ent;
	int ret;

	init_progress_stream(iod->infp);
	arch = archive_read_new();
	if (arch == NULL)
		die("Couldn't read archive read object");
	if (archive_read_support_format_tar(arch))
		die("Enabling tar format: %s", archive_error_string(arch));
	if (archive_read_support_compression_gzip(arch))
		die("Enabling gzip format: %s", archive_error_string(arch));
	if (archive_read_open(arch, iod, NULL, archive_read, NULL))
		die("Opening archive: %s", archive_error_string(arch));
	while (!(ret = archive_read_next_header(arch, &ent)))
		if (archive_read_extract(arch, ent, ARCHIVE_EXTRACT_FLAGS))
			die("Extracting %s: %s", archive_entry_pathname(ent),
						archive_error_string(arch));
	if (ret != ARCHIVE_EOF)
		die("Reading archive: %s", archive_error_string(arch));
	if (archive_read_close(arch))
		die("Closing archive: %s", archive_error_string(arch));
	archive_read_finish(arch);
	finish_progress();
}

/* ftw() provides no means to pass a data pointer to the called function, so
   we have to use a global.  (fts() has a nicer interface but doesn't work
   for _FILE_OFFSET_BITS = 64.)  This is only for write_entry(); pretend it
   doesn't exist otherwise. */
static struct archive *write_entry_archive;
/* Likewise for gather_size(). */
static off_t gather_size_total;

static int gather_size(const char *path, const struct stat *st, int type,
			struct FTW *ignored)
{
	(void)path;
	(void)ignored;

	if (type == FTW_F)
		gather_size_total += st->st_size;
	return 0;
}

static int write_entry(const char *path, const struct stat *st, int type,
			struct FTW *ignored)
{
	struct archive *arch = write_entry_archive;
	struct archive_entry *ent;
	FILE *fp = NULL;
	char buf[BUFSZ];
	ssize_t len;

	(void)ignored;

	switch (type) {
	case FTW_D:
		break;
	case FTW_F:
		/* Make sure we can read the file. */
		fp = fopen(path, "r");
		if (fp == NULL) {
			warn("Couldn't read %s: %s", path, strerror(errno));
			return 0;
		}
		break;
	case FTW_SL:
		/* Get the symlink target. */
		len = readlink(path, buf, sizeof(buf) - 1);
		if (len == -1) {
			warn("Couldn't read link %s: %s", path,
						strerror(errno));
			return 0;
		}
		buf[len] = 0;
		break;
	case FTW_DNR:
		warn("Couldn't read directory: %s", path);
		return 0;
	case FTW_NS:
		warn("Couldn't stat: %s", path);
		return 0;
	default:
		die("write_entry: Unknown type code %d: %s", type, path);
	}

	ent = archive_entry_new();
	if (ent == NULL)
		die("Couldn't allocate archive entry");
	archive_entry_copy_stat(ent, st);
	archive_entry_set_pathname(ent, path);
	if (S_ISLNK(st->st_mode))
		archive_entry_set_symlink(ent, buf);
	if (archive_write_header(arch, ent))
		die("Couldn't write archive header for %s: %s", path,
					archive_error_string(arch));
	archive_entry_free(ent);

	if (fp != NULL) {
		while (!feof(fp)) {
			len = fread(buf, 1, sizeof(buf), fp);
			if (ferror(fp))
				die("Error reading %s", path);
			/* libarchive < 2.4.8 will fail zero-byte writes
			   when using gzip/bzip2 */
			if (len > 0 && archive_write_data(arch, buf, len)
						!= len)
				die("Couldn't write archive data for %s: %s",
						path,
						archive_error_string(arch));
			progress(len);
		}
		fclose(fp);
	}
	return 0;
}

static void write_archive(struct iodata *iod, char * const *paths)
{
	struct archive *arch;
	char * const *path;
	int ret;

	gather_size_total = 0;  /* sigh */
	for (path = paths; *path != NULL; path++)
		if (nftw(*path, gather_size, FTW_FDS, FTW_PHYS))
			die("Error traversing path: %s", *path);
	init_progress(gather_size_total);
	arch = archive_write_new();
	if (arch == NULL)
		die("Couldn't read archive write object");
	if (archive_write_set_format_pax_restricted(arch))
		die("Setting tar format: %s", archive_error_string(arch));
	if (!use_internal_compress) {
		switch (external_compress) {
		case EXTERNAL_COMPRESS_NONE:
			ret = 0;
			break;
		case EXTERNAL_COMPRESS_GZIP:
			ret = archive_write_set_compression_gzip(arch);
			break;
		default:
			g_assert_not_reached();
		}
		if (ret)
			die("Setting compression format: %s",
						archive_error_string(arch));
	}
	if (archive_write_set_bytes_in_last_block(arch, 1))
		die("Disabling final block padding: %s",
					archive_error_string(arch));
	/* The third parameter of archive_write_callback became const in
	   libarchive 2.0.  Do an explicit cast to prevent warnings when
	   compiling against earlier versions. */
	if (archive_write_open(arch, iod, NULL,
				(archive_write_callback *) archive_write,
				archive_finish_write))
		die("Opening archive: %s", archive_error_string(arch));
	write_entry_archive = arch;  /* sigh */
	for (path = paths; *path != NULL; path++)
		if (nftw(*path, write_entry, FTW_FDS, FTW_PHYS))
			die("Error traversing path: %s", *path);
	if (archive_write_close(arch))
		die("Closing archive: %s", archive_error_string(arch));
	archive_write_finish(arch);
	finish_progress();
}

/** Top level ****************************************************************/

static GOptionEntry general_options[] = {
	{"in", 'i', 0, G_OPTION_ARG_FILENAME, &infile, "Input file", "PATH"},
	{"out", 'o', 0, G_OPTION_ARG_FILENAME, &outfile, "Output file", "PATH"},
	{"decode", 'd', G_OPTION_FLAG_REVERSE, G_OPTION_ARG_NONE, &encode, "Input is encoded; decode it", NULL},
	{"progress", 'p', 0, G_OPTION_ARG_NONE, &want_progress, "Print progress bar if possible", NULL},
	{NULL}
};

static GOptionEntry encrypt_options[] = {
	{"encrypt", 'e', 0, G_OPTION_ARG_NONE, &want_encrypt, "Encrypt data", NULL},
	{"keyroot-fd", 'k', 0, G_OPTION_ARG_INT, &keyroot_fd, "File descriptor from which to read the keyroot", "FD"},
	{NULL}
};

static GOptionEntry hash_options[] = {
	{"hash", 'h', 0, G_OPTION_ARG_NONE, &want_hash, "Hash data", NULL},
	{NULL}
};

static GOptionEntry tar_options[] = {
	{"tar", 't', 0, G_OPTION_ARG_NONE, &want_tar, "Generate or extract a compressed tar archive", NULL},
	{"compression", 'c', 0, G_OPTION_ARG_STRING, &compress_alg, "Compress with ALG (none, gzip, lzf, lzma)", "ALG"},
	{"directory", 'C', 0, G_OPTION_ARG_FILENAME, &parent_dir, "Change to DIR before tarring/untarring files", "DIR"},
	{NULL}
};

static GOptionContext *build_option_context(void)
{
	GOptionContext *ctx;
	GOptionGroup *grp;

	ctx = g_option_context_new("[paths] - encode/decode files");
	g_option_context_add_main_entries(ctx, general_options, NULL);

	grp = g_option_group_new("encrypt", "Encryption Options:",
				"Show help on encryption options",
				NULL, NULL);
	g_option_group_add_entries(grp, encrypt_options);
	g_option_context_add_group(ctx, grp);

	grp = g_option_group_new("hash", "Hash Options:",
				"Show help on hash options",
				NULL, NULL);
	g_option_group_add_entries(grp, hash_options);
	g_option_context_add_group(ctx, grp);

	grp = g_option_group_new("tar", "Tar Options:",
				"Show help on tar options",
				NULL, NULL);
	g_option_group_add_entries(grp, tar_options);
	g_option_context_add_group(ctx, grp);

	g_option_context_set_description(ctx, "tar mode compresses with gzip "
				"by default.  When unpacking a tarball,\n"
				"compression is auto-detected.");

	return ctx;
}

int main(int argc, char **argv)
{
	GOptionContext *octx;
	GError *err = NULL;
	struct iodata iod = {
		.infp = stdin,
		.outfp = stdout,
		.in = g_string_sized_new(BUFSZ),
		.out = g_string_sized_new(BUFSZ)
	};

	octx = build_option_context();
	if (!g_option_context_parse(octx, &argc, &argv, &err))
		die("%s", err->message);
	g_option_context_free(octx);
	if (want_tar && want_hash)
		die("--tar is incompatible with --hash");
	if (want_tar && encode && infile != NULL)
		die("--in invalid with --tar in encode mode");
	if (want_tar && !encode && outfile != NULL)
		die("--out invalid with --tar --decode");
	if (want_tar && encode && g_strv_length(argv) < 2)
		die("No input files or directories specified");
	if (!(want_tar && encode) && g_strv_length(argv) > 1)
		die("Extraneous arguments on command line");
	if (compress_alg && !want_tar)
		die("--compression only supported with --tar");
	if (compress_alg && !encode)
		die("--compression only supported when encoding");
	parse_compress_alg();
	if (infile != NULL) {
		iod.infp = fopen(infile, "r");
		if (iod.infp == NULL)
			die("Couldn't open %s for reading", infile);
	}
	if (outfile != NULL) {
		iod.outfp = fopen(outfile, "w");
		if (iod.outfp == NULL)
			die("Couldn't open %s for writing", outfile);
	}
	if (parent_dir != NULL)
		if (chdir(parent_dir))
			die("Couldn't change to directory %s", parent_dir);

	if (want_tar) {
		if (encode)
			write_archive(&iod, argv + 1);
		else
			read_archive(&iod);
	} else {
		run_stream(&iod);
	}
	fclose(iod.infp);
	fclose(iod.outfp);
	return 0;
}
