/*
 * vmnetfs - virtual machine network execution virtual filesystem
 *
 * Copyright (C) 2006-2012 Carnegie Mellon University
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.  A copy of the GNU General Public License
 * should have been distributed along with this program in the file
 * COPYING.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

/* When a FUSE filesystem operation is interrupted, FUSE delivers SIGUSR1
   to the specific thread performing the operation.  If the FS operation is
   a blocking read on a stream, the signal handler must be able to
   interrupt it.  However, if the blocking read is implemented using GCond
   (and therefore pthread_cond), we have two problems:

   1. pthread_cond_wait() is not guaranteed to return after a signal.

   2. pthread_cond_broadcast() is not async-signal safe, so the signal
   handler can't use it to terminate the wait.

   We therefore implement our own condition variables on top of SIGUSR1.
   libfuse will have already installed a no-op SIGUSR1 handler. */

#include <signal.h>
#include <pthread.h>
#include "vmnetfs-private.h"

struct vmnetfs_cond {
    GMutex *lock;
    GList *threads;
};

struct cond_waiter {
    pthread_t thr;
    bool signaled;
};

struct vmnetfs_cond *_vmnetfs_cond_new(void)
{
    struct vmnetfs_cond *cond;

    cond = g_slice_new0(struct vmnetfs_cond);
    cond->lock = g_mutex_new();
    return cond;
}

void _vmnetfs_cond_free(struct vmnetfs_cond *cond)
{
    g_assert(cond->threads == NULL);
    g_mutex_free(cond->lock);
    g_slice_free(struct vmnetfs_cond, cond);
}

/* Similar to g_cond_wait().  Returns when _vmnetfs_cond_signal() or
   _vmnetfs_cond_broadcast() is called or the current FUSE request is
   interrupted.  Returns true if the request was interrupted, false
   otherwise. */
bool _vmnetfs_cond_wait(struct vmnetfs_cond *cond, GMutex *lock)
{
    struct cond_waiter waiter = {
        .thr = pthread_self(),
    };
    GList *el;
    sigset_t mask;
    sigset_t orig;

    /* Ensure we're notified of lock-protected events */
    sigfillset(&mask);
    pthread_sigmask(SIG_SETMASK, &mask, &orig);
    g_mutex_lock(cond->lock);
    el = cond->threads = g_list_prepend(cond->threads, &waiter);
    g_mutex_unlock(cond->lock);

    /* Permit lock-protected events to occur */
    g_mutex_unlock(lock);

    /* Wait for event, provided that FUSE was not already interrupted
       before we blocked signals */
    if (!_vmnetfs_interrupted()) {
        sigdelset(&mask, SIGUSR1);
        sigsuspend(&mask);
    }

    /* Clean up */
    g_mutex_lock(cond->lock);
    cond->threads = g_list_delete_link(cond->threads, el);
    g_mutex_unlock(cond->lock);
    pthread_sigmask(SIG_SETMASK, &orig, NULL);

    /* Re-acquire parent lock */
    g_mutex_lock(lock);

    return _vmnetfs_interrupted();
}

static void signal_cond(struct cond_waiter *waiter, int32_t *max)
{
    if (!waiter->signaled && *max != 0) {
        pthread_kill(waiter->thr, SIGUSR1);
        waiter->signaled = true;
        if (*max != -1) {
            --*max;
        }
    }
}

void _vmnetfs_cond_signal(struct vmnetfs_cond *cond)
{
    int32_t max = 1;

    g_mutex_lock(cond->lock);
    g_list_foreach(cond->threads, (GFunc) signal_cond, &max);
    g_mutex_unlock(cond->lock);
}

void _vmnetfs_cond_broadcast(struct vmnetfs_cond *cond)
{
    int32_t max = -1;

    g_mutex_lock(cond->lock);
    g_list_foreach(cond->threads, (GFunc) signal_cond, &max);
    g_mutex_unlock(cond->lock);
}
