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

#include <unistd.h>
#include <signal.h>
#include "defs.h"

struct pk_sigstate sigstate;

static const int ignored_signals[]={SIGUSR1, SIGUSR2, 0};
static const int caught_signals[]={SIGINT, SIGTERM, SIGHUP, 0};

int main(int argc, char **argv)
{
	struct pk_state state = {0};
	gchar *progname;
	enum mode mode;
	int completion_fd=-1;
	char ret=1;
	int sig;
	int have_cache=0;
	int have_hoard=0;
	int have_transport=0;
	int have_fuse=0;
	int have_lock=0;
	pk_err_t err;

	if (!g_thread_supported())
		g_thread_init(NULL);
	state.stats_lock = g_mutex_new();

	if (setup_signal_handlers(generic_signal_handler, caught_signals,
				ignored_signals)) {
		/* Logging isn't up yet */
		printf("Couldn't set up signal handlers\n");
		return 1;
	}

	progname = g_path_get_basename(argv[0]);
	g_set_prgname(progname);
	g_free(progname);
	mode=parse_cmdline(&state.conf, argc - 1, argv + 1);
	/* Trivial modes (usage, version) have already been handled by
	   parse_cmdline() */

	log_start(state.conf->log_file, state.conf->log_file_mask,
				state.conf->log_stderr_mask);
	pk_log(LOG_INFO, "Parcelkeeper starting in %s mode",
				state.conf->modename);

	/* We can't take the lock until we fork (if we're going to do that) */
	if (state.conf->flags & WANT_BACKGROUND)
		if (fork_and_wait(&completion_fd))
			goto shutdown;

	/* Now take the lock */
	if (state.conf->flags & WANT_LOCK) {
		err=acquire_lockfile(&state.lockfile, state.conf->lockfile);
		if (err) {
			pk_log(LOG_ERROR, "Couldn't acquire parcel lock: %s",
						pk_strerror(err));
			goto shutdown;
		} else {
			have_lock=1;
		}
	}

	/* Now that we have the lock, it's safe to create the pidfile */
	if (state.conf->flags & WANT_BACKGROUND)
		if (create_pidfile(state.conf->pidfile))
			goto shutdown;

	if (state.conf->parcel_dir != NULL) {
		if (parse_parcel_cfg(&state.parcel, state.conf->parcel_cfg))
			goto shutdown;
		if (!iu_chunk_compress_is_enabled(
					state.parcel->required_compress,
					state.conf->compress)) {
			pk_log(LOG_ERROR, "This parcel does not support the "
						"requested compression type");
			goto shutdown;
		}
	}

	sql_init();

	if (cache_init(&state))
		goto shutdown;
	else
		have_cache=1;

	if (state.conf->hoard_index != NULL) {
		if (hoard_init(&state))
			goto shutdown;
		else
			have_hoard=1;
	}

	if (state.conf->flags & WANT_TRANSPORT) {
		if (transport_init())
			goto shutdown;
		state.cpool = transport_pool_alloc(&state);
		if (state.cpool == NULL)
			goto shutdown;
		have_transport=1;
	}

	if (mode == MODE_RUN) {
		if (fuse_init(&state))
			goto shutdown;
		else
			have_fuse=1;
	}

	if (pending_signal())
		goto shutdown;

	/* Release our parent, if we've forked */
	if (completion_fd != -1) {
		close(completion_fd);
		completion_fd=-1;
	}

	if (mode == MODE_RUN) {
		fuse_run(&state);
		ret=0;
	} else if (mode == MODE_UPLOAD) {
		ret=copy_for_upload(&state);
	} else if (mode == MODE_VALIDATE) {
		ret=validate_cache(&state);
	} else if (mode == MODE_EXAMINE) {
		ret=examine_cache(&state);
		if (state.conf->hoard_dir && !ret)
			ret=examine_hoard(&state);
	} else if (mode == MODE_HOARD) {
		ret=hoard(&state);
	} else if (mode == MODE_LISTHOARD) {
		ret=list_hoard(&state);
	} else if (mode == MODE_RMHOARD) {
		ret=rmhoard(&state);
	} else if (mode == MODE_GCHOARD) {
		ret=gchoard(&state);
	} else if (mode == MODE_CHECKHOARD) {
		ret=check_hoard(&state);
	} else if (mode == MODE_REFRESH) {
		ret=hoard_refresh(&state);
	} else {
		pk_log(LOG_ERROR, "Unknown mode");
	}

shutdown:
	interrupter_clear();
	if (have_fuse)
		fuse_shutdown(&state);
	if (have_transport)
		transport_pool_free(state.cpool);
	if (have_hoard)
		hoard_shutdown(&state);
	if (have_cache)
		cache_shutdown(&state);
	if (have_lock) {
		unlink(state.conf->pidfile);  /* safe if lock held */
		release_lockfile(state.lockfile);
	}
	log_shutdown();  /* safe to call unconditionally */
	parcel_cfg_free(state.parcel);  /* likewise */
	cmdline_free(state.conf);  /* likewise */
	if (completion_fd != -1)
		(void) write(completion_fd, &ret, 1);
	sig = sigstate.signal;
	if (sig) {
		/* Make sure our exit status reflects the fact that we died
		   on a signal.  If we're backgrounded, the parent will pick
		   this up in fork_and_wait(). */
		set_signal_handler(sig, SIG_DFL);
		raise(sig);
	}
	return ret;
}
