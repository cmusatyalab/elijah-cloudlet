/*
 * cloudletcacheFS - Cloudlet Cachcing emulation FS
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

#define DEBUG_MAIN
#ifdef DEBUG_MAIN
#define DPRINTF(fmt, ...) \
    do { \
    	fprintf(stdout, "[DEBUG][main] " fmt, ## __VA_ARGS__); \
    	fprintf(stdout, "\n"); fflush(stdout); \
    } while (0) 
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif


static bool handle_stdin(struct cachefs *fs, const char *oneline, GError **err)
{
	// check end signal
	if (strcmp(oneline, "q") == 0){
		fprintf(stdout, "[FUSE] Receive quit message\n");
		fflush(stdout);
		return false;
	}
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
        	DPRINTF("FUSE TERMINATED: Invalid stdin format\n");
        	break;
		}
        g_free(buf);
    }
out:
    //image_close(fs->disk);
    //if (fs->memory != NULL) {
    //    image_close(fs->memory);
    //}
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

static void fuse_main()
{
    struct cachefs *fs;
    GThread *loop_thread = NULL;
    GIOChannel *chan_in;
    GIOChannel *chan_out;
    GIOFlags flags;
    GError *err = NULL;

    /* Initialize */
    if (!g_thread_supported()) {
        g_thread_init(NULL);
    }

    /* open io channel*/
    chan_in = g_io_channel_unix_new(0);
    chan_out = g_io_channel_unix_new(1);

    /* Set up fuse */
    fs = g_slice_new0(struct cachefs);
    _cachefs_fuse_new(fs, &err);
    if (err) {
        fprintf(stdout, "%s\n", err->message);
        goto out;
    }

    /* Start main loop thread */
    loop_thread = g_thread_create(glib_loop_thread, fs, TRUE, &err);
    if (err) {
        fprintf(stdout, "%s\n", err->message);
        goto out;
    }

    /* Add watch for stdin being closed */
    flags = g_io_channel_get_flags(chan_in);
    g_io_channel_set_flags(chan_in, flags | G_IO_FLAG_NONBLOCK, &err);
    if (err) {
        fprintf(stdout, "%s\n", err->message);
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
    _cachefs_fuse_free(fs);
    g_slice_free(struct cachefs, fs);
    g_io_channel_unref(chan_in);
}

static void setsignal(int signum, void (*handler)(int))
{
    const struct sigaction sa = {
        .sa_handler = handler,
        .sa_flags = SA_RESTART,
    };

    sigaction(signum, &sa, NULL);
}


int main(int argc G_GNUC_UNUSED, char **argv G_GNUC_UNUSED)
{
    setsignal(SIGINT, SIG_IGN);
    fuse_main();
    return 0;
}
