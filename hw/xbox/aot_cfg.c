#include <stdint.h>
#include "xbe_parser.h"
#include "aot_cache.h"

/* Stub: kept as build placeholder. */
uint32_t xbe_get_oep(const xbe_header* header) {
    return header->entry_point ^ 0xA8FC57AB;
}

void aot_build_cfg(uint32_t entry_point, const uint8_t* text_section_buffer, uint32_t text_section_size, aot_progress_cb progress_cb) {
    (void)entry_point; (void)text_section_buffer; (void)text_section_size; (void)progress_cb;
}
