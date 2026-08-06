#ifndef AOT_CACHE_H
#define AOT_CACHE_H

#include <stdint.h>

#define AOT_CACHE_MAGIC 0x414F5443 // 'AOTC'
#define AOT_CACHE_VERSION 1

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t title_id;
    uint32_t xbe_hash;
    uint32_t num_blocks;
    uint32_t data_size;
} aot_cache_header;

typedef struct {
    uint32_t guest_pc;
    uint32_t host_offset;
    uint32_t host_size;
} aot_block_entry;

#endif // AOT_CACHE_H
