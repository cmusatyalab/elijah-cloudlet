/*
 * cloudletcacheFS - Cloudlet Cachcing emulation FS
 *
 * copyright (c) 2011-2013 carnegie mellon university
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
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "cachefs-private.h"


static bool handle_stdin(struct cachefs *fs, const char *oneline, GError **err)
{
	// check end signal
    gchar **fetch_info = g_strsplit(oneline, ":", 0);
    if ((*fetch_info == NULL) || (*(fetch_info +1) == NULL)){
        _cachefs_write_debug("[main] Wrong stdinput : %s", oneline);
        return false;
    }
}

static bool parse_uint(const char *str, unsigned int *ret_int)
{
    char *endptr;
    uint64_t ret;

    ret = g_ascii_strtoull(str, &endptr, 10);
    if (*str == 0 || *endptr != 0) {
        return false;
    }
    *ret_int = (unsigned int)ret;
    return true;
}

static gboolean read_stdin(GIOChannel *source,
        GIOCondition cond G_GNUC_UNUSED, void *data)
{
    struct cachefs *fs = data;
    char *buf;
    gsize terminator;
    bool success_stdin;
    GError *err = NULL;

    /* See if stdin has been closed. */
    while (1) {
    	switch (g_io_channel_read_line(source, &buf, NULL, &terminator, &err)) {
    	    case G_IO_STATUS_ERROR:
    	        return TRUE;
    	    case G_IO_STATUS_NORMAL:
    	        buf[terminator] = 0;
    	        break;
    	    case G_IO_STATUS_EOF:
    	        goto out;
    	    case G_IO_STATUS_AGAIN:
    	        return TRUE;
    	    default:
    	        g_assert_not_reached();
    	        break;
    	    }

		success_stdin = handle_stdin(fs, buf, &err);
        if (!success_stdin) {
        	_cachefs_write_error("[main] FUSE TERMINATED: Invalid stdin format");
        	g_free(buf);
        	break;
		}
        g_free(buf);
    }
out:
    _cachefs_fuse_terminate(fs);
    return FALSE;
}

static gboolean shutdown_callback(void *data)
{
    struct cachefs *fs = data;

    g_main_loop_quit(fs->glib_loop);
    return FALSE;
}

static void *glib_loop_thread(void *data)
{
    struct cachefs *fs = data;
    fs->glib_loop = g_main_loop_new(NULL, TRUE);
    g_main_loop_run(fs->glib_loop);
    g_main_loop_unref(fs->glib_loop);
    fs->glib_loop = NULL;
    return NULL;
}

/* fuse main */
static bool fuse_main(int argc, char **argv)
{
    struct cachefs *fs;
    bool parse_ret = false;
    GThread *loop_thread = NULL;
    GThread *redis_subscribe_thread = NULL;
    GIOChannel *chan_in;
    //GIOChannel *chan_out;
    GIOFlags flags;
    GError *err = NULL;

    /* Initialize */
    fs = g_slice_new0(struct cachefs);
    fs->cache_root = g_strdup(argv[1]);
    fs->uri_root = g_strdup(argv[2]);
    fs->redis_ip = g_strdup(argv[3]);
    parse_ret = parse_uint(argv[4], &(fs->redis_port));
    fs->redis_req_channel = g_strdup(argv[5]);
    fs->redis_res_channel = g_strdup(argv[6]);
    if (parse_ret == false){
    	_cachefs_write_error("[main] Invalid redis port number : %s, (%d)\n", \
    			argv[4], fs->redis_port);
        return EXIT_FAILURE;
    }

	if (_cachefs_init_pipe_communication() == false){
    	_cachefs_write_error("[main] Invalid redis port number : %s\n", argv[4]);
        return EXIT_FAILURE;
	}

    if (!g_thread_supported()) {
        g_thread_init(NULL);
    }
    if (!_redis_init(fs)){
    	_cachefs_write_error("[main] Cannot connect to redis\n");
    	return EXIT_FAILURE;
	}

    /* open io channel*/
    chan_in = g_io_channel_unix_new(0);
    //chan_out = g_io_channel_unix_new(1);

    /* Set up fuse */
    _cachefs_fuse_new(fs, &err);
    if (err) {
        fprintf(stdout, "%s\n", err->message);
        goto out;
    }

    /* Start main loop thread */
    loop_thread = g_thread_create(glib_loop_thread, fs, TRUE, &err);
    if (err) {
    	_cachefs_write_error("%s\n", err->message);
        goto out;
    }

    /* Add watch for stdin being closed */
    flags = g_io_channel_get_flags(chan_in);
    g_io_channel_set_flags(chan_in, flags | G_IO_FLAG_NONBLOCK, &err);
    if (err) {
    	_cachefs_write_error("%s\n", err->message);
        g_io_channel_unref(chan_in);
        goto out;
    }
    g_io_add_watch(chan_in, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL, read_stdin, fs);

    /* Started successfully. */
    fprintf(stdout, "%s\n", fs->mountpoint);
    fflush(stdout);
    _cachefs_fuse_run(fs);

out:
    /* Shut down */
    if (err != NULL) {
        g_clear_error(&err);
    }
    if (loop_thread != NULL) {
        g_idle_add(shutdown_callback, fs);
        g_thread_join(loop_thread);
    }
    if (redis_subscribe_thread != NULL){
    	g_idle_add(shutdown_callback, fs);
    	g_thread_join(redis_subscribe_thread);
    }
    _cachefs_fuse_free(fs);
    g_slice_free(struct cachefs, fs);
    g_io_channel_unref(chan_in);
    _redis_close();

	_cachefs_write_debug("[main] gracefully closed");
    _cachefs_close_pipe_communication();
    return EXIT_SUCCESS;
}

static void setsignal(int signum, void (*handler)(int))
{
    const struct sigaction sa = {
        .sa_handler = handler,
        .sa_flags = SA_RESTART,
    };

    sigaction(signum, &sa, NULL);
}

void static print_usage(char **argv)
{
    fprintf(stdout, "$ prog [/path/to/cache_root] [uri_root] [REDIS_IP] [REDIS_PORT] [REDIS_REQ_CHANNEL] [REDIS_RES_CHANNLE]\n");
    return;
}

int main(int argc, char **argv)
{
    if (argc != 7){
        print_usage(argv);
        return EXIT_FAILURE;
    }

    setsignal(SIGINT, SIG_IGN);
    return fuse_main(argc, argv);
}
