/*
 * libisrcrypto - cryptographic library for the OpenISR (R) system
 *
 * Copyright (C) 2008-2009 Carnegie Mellon University
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of version 2.1 of the GNU Lesser General Public License as
 * published by the Free Software Foundation.  A copy of the GNU Lesser General
 * Public License should have been distributed along with this library in the
 * file LICENSE.LGPL.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License
 * for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <glib.h>
#include "isrcrypto.h"

#define BUFLEN 131072

const char *algname = "zlib";
gboolean decode;
int level;

static GOptionEntry options[] = {
	{"decode", 'd', 0, G_OPTION_ARG_NONE, &decode, "Decompress", NULL},
	{"alg", 'a', 0, G_OPTION_ARG_STRING, &algname, "Algorithm (default: zlib)", "{zlib|lzf|lzma}"},
	{"level", 'l', 0, G_OPTION_ARG_INT, &level, "Compression level", "LEVEL"},
	{NULL, 0, 0, 0, NULL, NULL, NULL}
};

void G_GNUC_NORETURN die(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(1);
}

int main(int argc, char **argv)
{
	GOptionContext *optctx;
	GError *err = NULL;
	enum isrcry_compress alg;
	struct isrcry_compress_ctx *ctx;
	enum isrcry_direction direction;
	char in[BUFLEN];
	char out[BUFLEN];
	ssize_t count;
	unsigned inlen;
	unsigned outlen;
	unsigned offset;
	unsigned used;
	enum isrcry_result ret;

	optctx = g_option_context_new(" - (de)compress stdin to stdout");
	g_option_context_add_main_entries(optctx, options, NULL);
	if (!g_option_context_parse(optctx, &argc, &argv, &err))
		die("%s", err->message);
	g_option_context_free(optctx);

	direction = decode ? ISRCRY_DECODE : ISRCRY_ENCODE;
	if (!strcmp(algname, "zlib"))
		alg = ISRCRY_COMPRESS_ZLIB;
	else if (!strcmp(algname, "lzf"))
		alg = ISRCRY_COMPRESS_LZF_STREAM;
	else if (!strcmp(algname, "lzma"))
		alg = ISRCRY_COMPRESS_LZMA;
	else
		die("Unknown algorithm: %s", algname);

	ctx = isrcry_compress_alloc(alg);
	if (ctx == NULL)
		die("Couldn't alloc");
	ret = isrcry_compress_init(ctx, direction, level);
	if (ret)
		die("Couldn't init");
	while (1) {
		inlen = count = fread(in, 1, sizeof(in), stdin);
		if (count == -1)
			die("Error reading input");
		if (count == 0)
			break;
		for (offset = 0; offset < inlen; ) {
			used = inlen - offset;
			outlen = sizeof(out);
			ret = isrcry_compress_process(ctx, in + offset,
						&used, out, &outlen);
			if (ret)
				die("Process failed");
			offset += used;
			if (fwrite(out, 1, outlen, stdout) < outlen)
				die("Short write");
		}
	}
	while (1) {
		inlen = 0;
		outlen = sizeof(out);
		ret = isrcry_compress_final(ctx, NULL, &inlen, out, &outlen);
		if (fwrite(out, 1, outlen, stdout) < outlen)
			die("Short write on final");
		if (ret == ISRCRY_OK)
			break;
		if (ret != ISRCRY_BUFFER_OVERFLOW)
			die("Failure on final");
	}
	return 0;
}
