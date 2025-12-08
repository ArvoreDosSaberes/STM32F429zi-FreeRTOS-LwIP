#ifndef COMPAT_SYS_TYPES_H
#define COMPAT_SYS_TYPES_H

/*
 * Reexporta o <sys/types.h> real da toolchain para preservar os tipos
 * padrão (clock_t, dev_t, mode_t, pid_t, etc.).
 */
#include_next <sys/types.h>

#endif /* COMPAT_SYS_TYPES_H */
