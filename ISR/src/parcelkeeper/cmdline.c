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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "defs.h"

static struct pk_config default_config = {
	/* WARNING implies ERROR */
	.log_file_mask = (1 << LOG_INFO) | (1 << LOG_WARNING) |
				(1 << LOG_STATS),
	.log_stderr_mask = 1 << LOG_WARNING,
	.compress = IU_CHUNK_COMP_NONE,
	.chunk_cache = 32, /* MB */
};

enum arg_type {
	REQUIRED,
	OPTIONAL,
	ANY,  /* any number permitted, including zero */
};

enum option {
	OPT_PARCEL,
	OPT_HOARD,
	OPT_UUID,
	OPT_DESTDIR,
	OPT_COMPRESSION,
	OPT_LOG,
	OPT_MASK_FILE,
	OPT_MASK_STDERR,
	OPT_FOREGROUND,
	OPT_CHECK,
	OPT_FULL,
	OPT_SPLICE,
	OPT_COMPACT,
	OPT_ALLOW_ROOT,
	OPT_SINGLE_THREAD,
	OPT_MODE,
	OPT_CHUNK_CACHE,
	END_OPTS
};

struct pk_option {
	char *name;
	enum option opt;
	char *arg;
	char *comment;
};

struct pk_option_record {
	enum option opt;
	enum arg_type type;
	char *comment;
};

struct pk_mode {
	char *name;
	enum mode type;
	unsigned flags;
	const struct pk_option_record *opts;
	char *desc;
};

static const struct pk_option pk_options[] = {
	{"parcel",         OPT_PARCEL,         "parcel_dir"},
	{"hoard",          OPT_HOARD,          "hoard_dir"},
	{"uuid",           OPT_UUID,           "uuid"},
	{"destdir",        OPT_DESTDIR,        "dir"},
	{"chunk-cache",    OPT_CHUNK_CACHE,    "MB",                       "Size of the decrypted chunk cache"},
	{"compression",    OPT_COMPRESSION,    "algorithm",                "Accepted algorithms: none (default), zlib, lzf"},
	{"log",            OPT_LOG,            "file"},
	{"log-filter",     OPT_MASK_FILE,      "comma_separated_list",     "Override default list of log types"},
	{"stderr-filter",  OPT_MASK_STDERR,    "comma_separated_list",     "Override default list of log types"},
	{"foreground",     OPT_FOREGROUND,     NULL,                       "Don't run in the background"},
	{"check",          OPT_CHECK,          NULL},
	{"full",           OPT_FULL,           NULL,                       "Perform full data-integrity check"},
	{"splice",         OPT_SPLICE,         NULL,                       "Revert chunks that fail tag validation (requires --full)"},
	{"compact",        OPT_COMPACT,        NULL,                       "Compact the hoard cache"},
	{"allow-root",     OPT_ALLOW_ROOT,     NULL,                       "Allow the root user to access the virtual filesystem"},
	{"single-thread",  OPT_SINGLE_THREAD,  NULL,                       "Don't run multi-threaded"},
	{"mode",           OPT_MODE,           "mode",                     "Print detailed usage message about the given mode"},
	{0}
};

struct pk_cmdline_parse_ctx {
	const struct pk_mode *curmode;
	char *optparam;
	int optind;
	unsigned optseen[END_OPTS];
};

#define mode(sym) static const struct pk_option_record sym ## _opts[]

mode(RUN) = {
	{OPT_PARCEL,        REQUIRED},
	{OPT_HOARD,         OPTIONAL},
	{OPT_CHUNK_CACHE,   OPTIONAL},
	{OPT_COMPRESSION,   OPTIONAL},
	{OPT_LOG,           OPTIONAL},
	{OPT_MASK_FILE,     OPTIONAL},
	{OPT_MASK_STDERR,   OPTIONAL},
	{OPT_ALLOW_ROOT,    OPTIONAL},
	{OPT_SINGLE_THREAD, OPTIONAL},
	{OPT_FOREGROUND,    OPTIONAL},
	{END_OPTS}
};

mode(UPLOAD) = {
	{OPT_PARCEL,        REQUIRED},
	{OPT_DESTDIR,       REQUIRED},
	{OPT_HOARD,         OPTIONAL, "Also update the specified hoard cache"},
	{OPT_LOG,           OPTIONAL},
	{OPT_MASK_FILE,     OPTIONAL},
	{OPT_MASK_STDERR,   OPTIONAL},
	{END_OPTS}
};

mode(HOARD) = {
	{OPT_PARCEL,        REQUIRED},
	{OPT_HOARD,         REQUIRED},
	{OPT_CHECK,         OPTIONAL, "Don't download; just return 0 if fully hoarded or 1 otherwise"},
	{OPT_LOG,           OPTIONAL},
	{OPT_MASK_FILE,     OPTIONAL},
	{OPT_MASK_STDERR,   OPTIONAL},
	{END_OPTS}
};

mode(EXAMINE) = {
	{OPT_PARCEL,        REQUIRED},
	{OPT_HOARD,         OPTIONAL, "Print statistics on the specified hoard cache"},
	{OPT_LOG,           OPTIONAL},
	{OPT_MASK_FILE,     OPTIONAL},
	{OPT_MASK_STDERR,   OPTIONAL},
	{END_OPTS}
};

mode(VALIDATE) = {
	{OPT_PARCEL,        REQUIRED},
	{OPT_FULL,          OPTIONAL},
	{OPT_SPLICE,        OPTIONAL},
	{OPT_CHECK,         OPTIONAL, "Don't validate; just set $? & 2 if cache is dirty, $? & 4 if damaged"},
	{OPT_LOG,           OPTIONAL},
	{OPT_MASK_FILE,     OPTIONAL},
	{OPT_MASK_STDERR,   OPTIONAL},
	{END_OPTS}
};

mode(LISTHOARD) = {
	{OPT_HOARD,         REQUIRED},
	{OPT_LOG,           OPTIONAL},
	{OPT_MASK_FILE,     OPTIONAL},
	{OPT_MASK_STDERR,   OPTIONAL},
	{END_OPTS}
};

mode(CHECKHOARD) = {
	{OPT_HOARD,         REQUIRED},
	{OPT_FULL,          OPTIONAL},
	{OPT_COMPACT,       OPTIONAL},
	{OPT_LOG,           OPTIONAL},
	{OPT_MASK_FILE,     OPTIONAL},
	{OPT_MASK_STDERR,   OPTIONAL},
	{END_OPTS}
};

mode(RMHOARD) = {
	{OPT_HOARD,         REQUIRED},
	{OPT_UUID,          REQUIRED, "UUID of parcel to remove from hoard cache"},
	{OPT_LOG,           OPTIONAL},
	{OPT_MASK_FILE,     OPTIONAL},
	{OPT_MASK_STDERR,   OPTIONAL},
	{END_OPTS}
};

mode(GCHOARD) = {
	{OPT_HOARD,         REQUIRED},
	{OPT_LOG,           OPTIONAL},
	{OPT_MASK_FILE,     OPTIONAL},
	{OPT_MASK_STDERR,   OPTIONAL},
	{END_OPTS}
};

mode(REFRESH) = {
	{OPT_PARCEL,        REQUIRED},
	{OPT_HOARD,         REQUIRED},
	{OPT_LOG,           OPTIONAL},
	{OPT_MASK_FILE,     OPTIONAL},
	{OPT_MASK_STDERR,   OPTIONAL},
	{END_OPTS}
};

mode(HELP) = {
	{OPT_MODE,          OPTIONAL},
	{END_OPTS}
};

mode(VERSION) = {
	{END_OPTS}
};

#undef mode

#define RUN_flags		WANT_LOCK|WANT_CACHE|WANT_PREV|WANT_BACKGROUND|WANT_TRANSPORT|WANT_SHM
#define UPLOAD_flags		WANT_LOCK|WANT_CACHE|WANT_PREV
#define HOARD_flags		WANT_PREV|WANT_TRANSPORT
#define EXAMINE_flags		WANT_CACHE|WANT_PREV
#define VALIDATE_flags		WANT_LOCK|WANT_CACHE|WANT_PREV
#define LISTHOARD_flags		0
#define CHECKHOARD_flags	0
#define RMHOARD_flags		0
#define GCHOARD_flags		0
#define REFRESH_flags		WANT_PREV|WANT_GC
#define HELP_flags		0
#define VERSION_flags		0

#define sym(str) MODE_ ## str, str ## _flags, str ## _opts
static const struct pk_mode pk_modes[] = {
	{"run",         sym(RUN),        "Bind and service a virtual disk"},
	{"upload",      sym(UPLOAD),     "Split a local cache into individual chunks for upload"},
	{"hoard",       sym(HOARD),      "Download all chunks into hoard cache"},
	{"examine",     sym(EXAMINE),    "Print cache statistics"},
	{"validate",    sym(VALIDATE),   "Validate local cache against keyring"},
	{"listhoard",   sym(LISTHOARD),  "List parcels in hoard cache"},
	{"checkhoard",  sym(CHECKHOARD), "Validate hoard cache"},
	{"rmhoard",     sym(RMHOARD),    "Remove parcel from hoard cache"},
	{"gchoard",     sym(GCHOARD),    "Manually garbage-collect hoard cache"},
	{"refresh",     sym(REFRESH),    "Update hoard cache reference list"},
	{"help",        sym(HELP),       "Show usage summary"},
	{"version",     sym(VERSION),    "Show version information"},
	{0}
};
#undef sym

static const struct pk_option *get_option(enum option opt)
{
	const struct pk_option *curopt;

	for (curopt=pk_options; curopt->name != NULL; curopt++) {
		if (curopt->opt == opt)
			return curopt;
	}
	printf("BUG: Unknown option %d\n", opt);
	return NULL;
}

static void usage(const struct pk_mode *mode) __attribute__ ((noreturn));
static void usage(const struct pk_mode *mode)
{
	const char *progname = g_get_prgname();
	const struct pk_mode *mtmp;
	const struct pk_option_record *rtmp;
	const struct pk_option *otmp;
	char *str_start=NULL;
	char *str_end=NULL;
	int have_options=0;

	if (mode == NULL) {
		printf("Usage: %s <mode> <options>\n", progname);
		printf("Available modes:\n");
		for (mtmp=pk_modes; mtmp->name != NULL; mtmp++) {
			printf("     %-11s %s\n", mtmp->name, mtmp->desc);
		}
		printf("Run \"%s help --mode <mode>\" for more information.\n",
					progname);
	} else {
		for (rtmp=mode->opts; rtmp->opt != END_OPTS; rtmp++) {
			otmp=get_option(rtmp->opt);
			if (!have_options) {
				have_options=1;
				printf("Usage: %s %s <options>\n", progname,
							mode->name);
				printf("Available options:\n");
			}
			switch (rtmp->type) {
			case REQUIRED:
				str_start=" ";
				str_end="";
				break;
			case OPTIONAL:
				str_start="[";
				str_end="]";
				break;
			case ANY:
				str_start="[";
				str_end="]+";
				break;
			}
			printf("    %s--%s", str_start, otmp->name);
			if (otmp->arg)
				printf(" <%s>", otmp->arg);
			printf("%s\n", str_end);
			if (otmp->comment != NULL)
				printf("          %s\n", otmp->comment);
			if (rtmp->comment != NULL)
				printf("          %s\n", rtmp->comment);
		}
		if (!have_options)
			printf("Usage: %s %s\n", progname, mode->name);
	}
	exit(1);
}

#define PARSE_ERROR(ctx, str, args...) do { \
		printf("ERROR: " str "\n\n" , ## args); \
		usage((ctx)->curmode); \
	} while (0)

/* Instead of using getopt_long() we roll our own.  getopt_long doesn't support
   several things we need:
   - More than one parameter per option
   - Checking for required or once-only options
   - Different permissible parameters depending on circumstances (mode)
*/
static enum option pk_getopt(struct pk_cmdline_parse_ctx *ctx, int argc,
			char *argv[])
{
	const struct pk_option_record *opts;
	const struct pk_option *curopt;
	char *arg;

	if (ctx->optind == argc) {
		/* We've read the entire command line; make sure all required
		   arguments have been handled */
		for (opts=ctx->curmode->opts; opts->opt != END_OPTS; opts++) {
			if (opts->type == REQUIRED && !ctx->optseen[opts->opt])
				PARSE_ERROR(ctx, "missing required option "
						"--%s",
						get_option(opts->opt)->name);
		}
		return END_OPTS;
	}

	arg=argv[ctx->optind++];
	if (arg[0] != '-' || arg[1] != '-')
		PARSE_ERROR(ctx, "\"%s\" is not an option element", arg);
	arg += 2;

	for (opts=ctx->curmode->opts; opts->opt != END_OPTS; opts++) {
		curopt=get_option(opts->opt);
		if (strcmp(curopt->name, arg))
			continue;
		if (opts->type != ANY && ctx->optseen[opts->opt])
			PARSE_ERROR(ctx, "--%s may only be specified once",
						arg);
		ctx->optseen[opts->opt]++;
		if (curopt->arg) {
			if (optind == argc)
				PARSE_ERROR(ctx, "wrong number of arguments "
							"to --%s", arg);
			ctx->optparam=argv[ctx->optind++];
			if (ctx->optparam[0] == '-' && ctx->optparam[1] == '-')
				PARSE_ERROR(ctx, "wrong number of arguments "
							"to --%s", arg);
		}
		return opts->opt;
	}

	/* This option is invalid.  See if it would have been valid for a
	   different mode. */
	for (curopt=pk_options; curopt->name != NULL; curopt++)
		if (!strcmp(curopt->name, arg))
			PARSE_ERROR(ctx, "--%s not valid in this mode", arg);
	PARSE_ERROR(ctx, "unknown option --%s", arg);
}

static const struct pk_mode *parse_mode(const char *name)
{
	const struct pk_mode *cur;

	for (cur=pk_modes; cur->name != NULL; cur++) {
		if (!strcmp(name, cur->name))
			return cur;
	}
	return NULL;
}

static void check_dir(struct pk_cmdline_parse_ctx *ctx, const char *dir)
{
	if (!g_file_test(dir, G_FILE_TEST_IS_DIR))
		PARSE_ERROR(ctx, "%s is not a valid directory", dir);
}

static gchar *filepath(struct pk_cmdline_parse_ctx *ctx, const char *dir,
			const char *file, int must_exist)
{
	gchar *ret;
	ret = g_strdup_printf("%s/%s", dir, file);
	if (must_exist && !g_file_test(ret, G_FILE_TEST_IS_REGULAR))
		PARSE_ERROR(ctx, "%s does not exist", ret);
	return ret;
}

enum mode parse_cmdline(struct pk_config **out, int argc, char **argv)
{
	const struct pk_mode *helpmode=NULL;
	struct pk_config *conf;
	struct pk_cmdline_parse_ctx ctx = {0};
	enum option opt;
	char *cp;

	conf=g_slice_new(struct pk_config);
	*conf=default_config;
	if (argc == 0)
		usage(NULL);
	ctx.curmode=parse_mode(argv[0]);
	if (ctx.curmode == NULL)
		PARSE_ERROR(&ctx, "unknown mode %s", argv[0]);
	conf->modename=ctx.curmode->name;
	conf->flags=ctx.curmode->flags;

	ctx.optind=1;  /* ignore argv[0] */
	while ((opt=pk_getopt(&ctx, argc, argv)) != END_OPTS) {
		switch (opt) {
		case OPT_PARCEL:
			conf->parcel_dir=g_strdup(ctx.optparam);
			check_dir(&ctx, conf->parcel_dir);
			cp=conf->parcel_dir;
			conf->parcel_cfg=filepath(&ctx, cp, "parcel.cfg", 1);
			conf->keyring=filepath(&ctx, cp, "keyring",
						conf->flags & WANT_CACHE);
			conf->prev_keyring=filepath(&ctx, cp, "prev-keyring",
						conf->flags & WANT_PREV);
			conf->cache_file=filepath(&ctx, cp, "disk", 0);
			conf->cache_index=filepath(&ctx, cp, "disk.idx", 0);
			conf->vfspath=filepath(&ctx, cp, "vfs", 0);
			conf->lockfile=filepath(&ctx, cp, "parcelkeeper.lock",
						0);
			conf->pidfile=filepath(&ctx, cp, "parcelkeeper.pid",
						0);
			break;
		case OPT_UUID:
			if (canonicalize_uuid(ctx.optparam, &conf->uuid))
				PARSE_ERROR(&ctx, "invalid uuid: %s",
							ctx.optparam);
			break;
		case OPT_DESTDIR:
			conf->dest_dir=g_strdup(ctx.optparam);
			break;
		case OPT_COMPRESSION:
			conf->compress = iu_chunk_compress_parse(ctx.optparam);
			if (conf->compress == IU_CHUNK_COMP_UNKNOWN)
				PARSE_ERROR(&ctx, "invalid compression type: "
							"%s", ctx.optparam);
			break;
		case OPT_HOARD:
			conf->hoard_dir=g_strdup(ctx.optparam);
			conf->hoard_file=filepath(&ctx, ctx.optparam, "hoard",
						0);
			conf->hoard_index=filepath(&ctx, ctx.optparam,
						"hoard.idx", 0);
			break;
		case OPT_LOG:
			conf->log_file=g_strdup(ctx.optparam);
			break;
		case OPT_MASK_FILE:
			if (logtypes_to_mask(ctx.optparam,
						&conf->log_file_mask))
				PARSE_ERROR(&ctx, "invalid log type list: %s",
							ctx.optparam);
			break;
		case OPT_MASK_STDERR:
			if (logtypes_to_mask(ctx.optparam,
						&conf->log_stderr_mask))
				PARSE_ERROR(&ctx, "invalid log type list: %s",
							ctx.optparam);
			break;
		case OPT_FOREGROUND:
			conf->flags &= ~WANT_BACKGROUND;
			break;
		case OPT_CHECK:
			/* Abuse of WANT_ flags? */
			conf->flags |= WANT_CHECK;
			break;
		case OPT_FULL:
			/* Abuse of WANT_ flags? */
			conf->flags |= WANT_FULL_CHECK;
			break;
		case OPT_SPLICE:
			/* Abuse of WANT_ flags? */
			conf->flags |= WANT_SPLICE;
			break;
		case OPT_COMPACT:
			/* Abuse of WANT_ flags? */
			conf->flags |= WANT_COMPACT;
			break;
		case OPT_ALLOW_ROOT:
			/* Abuse of WANT_ flags? */
			conf->flags |= WANT_ALLOW_ROOT;
			break;
		case OPT_SINGLE_THREAD:
			/* Abuse of WANT_ flags? */
			conf->flags |= WANT_SINGLE_THREAD;
			break;
		case OPT_MODE:
			helpmode=parse_mode(ctx.optparam);
			if (helpmode == NULL)
				PARSE_ERROR(&ctx, "unknown mode %s; try "
							"\"%s help\"",
							ctx.optparam,
							g_get_prgname());
			break;
		case OPT_CHUNK_CACHE:
			if (parseuint(&conf->chunk_cache, ctx.optparam, 10))
				PARSE_ERROR(&ctx, "invalid integer value: %s",
							ctx.optparam);
			break;
		case END_OPTS:
			/* Silence compiler warning */
			break;
		}
	}

	if (ctx.curmode->type == MODE_HELP) {
		usage(helpmode);
	} else if (ctx.curmode->type == MODE_VERSION) {
		printf("OpenISR %s, Parcelkeeper revision %s\n", isr_release,
					rcs_revision);
		exit(0);
	}
	*out=conf;
	return ctx.curmode->type;
}

void cmdline_free(struct pk_config *conf)
{
	if (conf == NULL)
		return;
	g_free(conf->parcel_dir);
	g_free(conf->parcel_cfg);
	g_free(conf->keyring);
	g_free(conf->prev_keyring);
	g_free(conf->cache_file);
	g_free(conf->cache_index);
	g_free(conf->vfspath);
	g_free(conf->lockfile);
	g_free(conf->pidfile);
	g_free(conf->hoard_dir);
	g_free(conf->hoard_file);
	g_free(conf->hoard_index);
	g_free(conf->dest_dir);
	g_free(conf->log_file);
	g_free(conf->uuid);
	g_slice_free(struct pk_config, conf);
}
