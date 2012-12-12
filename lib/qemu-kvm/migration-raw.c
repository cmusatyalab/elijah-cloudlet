/*
 * QEMU live migration via generic fd
 *
 * Copyright Red Hat, Inc. 2009
 *
 * Authors:
 *  Chris Lalancette <clalance@redhat.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 *
 * Contributions after 2012-01-13 are licensed under the terms of the
 * GNU GPL, version 2 or (at your option) any later version.
 */

#include "qemu-common.h"
#include "qemu_socket.h"
#include "migration.h"
#include "monitor.h"
#include "qemu-char.h"
#include "buffered_file.h"
#include "block.h"
#include "qemu_socket.h"
#include "cloudlet/qemu-cloudlet.h"

#define DEBUG_MIGRATION_RAW

#ifdef DEBUG_MIGRATION_RAW
#define DPRINTF(fmt, ...) \
    do { printf("migration-raw: " fmt, ## __VA_ARGS__); } while (0)
#else
#define DPRINTF(fmt, ...) \
    do { } while (0)
#endif

static int raw_errno(MigrationState *s)
{
    return errno;
}

static int raw_write(MigrationState *s, const void * buf, size_t size)
{
    return write(s->fd, buf, size);
}

static int raw_close(MigrationState *s)
{
    struct stat st;
    int ret;

    DPRINTF("raw_close\n");
    if (s->fd != -1) {
        ret = fstat(s->fd, &st);
        if (ret == 0 && S_ISREG(st.st_mode)) {
            /*
             * If the file handle is a regular file make sure the
             * data is flushed to disk before signaling success.
             */
            ret = fsync(s->fd);
            if (ret != 0) {
                ret = -errno;
                perror("migration-fd: fsync");
                return ret;
            }
        }
        ret = close(s->fd);
        s->fd = -1;
        if (ret != 0) {
            ret = -errno;
            perror("migration-raw: close");
            return ret;
        }
    }
    return 0;
}

int raw_start_outgoing_migration(MigrationState *s, const char *fdname)
{
	DPRINTF("raw_migration: start migration at %s\n", fdname);
	// for already created file
    s->fd = monitor_get_fd(cur_mon, fdname);
    if (s->fd == -1) {
		s->fd = open(fdname, O_CREAT | O_WRONLY | O_TRUNC, 00644);
		if (s->fd == -1) {
			DPRINTF("raw_migration: failed to open file\n");
			goto err_after_get_fd;
		}

	}

	if (fcntl(s->fd, F_SETFL, O_NONBLOCK) == -1) {
		DPRINTF("Unable to set nonblocking mode on file descriptor\n");
		goto err_after_open;
	}

    s->get_error = raw_errno;
    s->write = raw_write;
    s->close = raw_close;

    migrate_fd_connect_raw(s);
    return 0;

err_after_open:
    close(s->fd);
err_after_get_fd:
    return -1;
}

static void raw_accept_incoming_migration(void *opaque)
{
    QEMUFile *f = opaque;

    process_incoming_migration(f);
    qemu_set_fd_handler2(qemu_stdio_fd(f), NULL, NULL, NULL, NULL);
    qemu_memfile = f;
    // qemu_fclose(f);
}

int raw_start_incoming_migration(const char *infd)
{
    int fd;
    int val;
    QEMUFile *f;
    
    DPRINTF("First attempt to start an incoming migration via fd to support libvirt\n");
    val = strtol(infd, NULL, 0);
    if ((errno == ERANGE && (val == INT_MAX|| val == INT_MIN)) || (val == 0)) {
        DPRINTF("Unable to apply qemu wrapper to file descriptor, fd:%d\n", val);
		DPRINTF("Attempting to start an incoming migration via raw\n");
		fd = open(infd, O_RDWR);
    }else{
        fd = val;
    }

	f = qemu_fdopen(fd, "rb");
	if(f == NULL) {
		DPRINTF("Unable to apply qemu wrapper to file descriptor\n");
		return -errno;
	}

	// read ahead external header file, e.g. libvirt header
	// to have mmap file for memory
	long start_offset = lseek(fd, 0, SEEK_CUR);
	DPRINTF("Migration file start at %ld\n", start_offset);
	qemu_fseek(f, start_offset, SEEK_CUR);

    set_use_raw(f, 1);

    qemu_set_fd_handler2(fd, NULL, raw_accept_incoming_migration, NULL, f);

    return 0;
}
