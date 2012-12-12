#ifndef QEMU_CLOUDLET_H
#define QEMU_CLOUDLET_H

#include "qemu-common.h"

extern QEMUFile *qemu_memfile;

int cloudlet_init(const char *logfile_path);
int cloudlet_end(void);

int printlog(const char* format, ...);

#endif /* QEMU_CLOUDLET_H */

