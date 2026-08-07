#include "qemu/osdep.h"
#include <stdio.h>
#include "aot_cache.h"

/* Stub: the real TB cache system in accel/tcg/tb-cache-hints.c
 * handles all translation block caching. This file is kept as a
 * build placeholder. */
void xemu_aot_scan_xbe_fd(int fd, aot_progress_cb progress_cb) {
    (void)fd;
    (void)progress_cb;
}
