#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "xbe_parser.h"
#include "aot_cache.h"

#define XBE_RETAIL_OEP_KEY 0xA8FC57AB
#define XBE_DEBUG_OEP_KEY  0x94859D4B

#define MAX_QUEUE_SIZE 20000
#define MAX_VISITED_BLOCKS 65536

extern void xemu_aot_translate_block(uint32_t start_pc, uint32_t end_pc, const uint8_t* block_buffer);

/* Step 1: Decode Entry Point using Retail or Debug key */
uint32_t xbe_get_oep(const xbe_header* header) {
    if (!header) return 0;
    // Check bit 3 of initialization flags (Debug vs Retail)
    bool is_debug = (header->init_flags & 0x00000008) != 0;
    uint32_t key = is_debug ? XBE_DEBUG_OEP_KEY : XBE_RETAIL_OEP_KEY;
    return header->entry_point ^ key;
}

/* Helper to check if address was already visited */
static bool is_visited(uint32_t pc, const uint32_t* visited, int visited_count) {
    for (int i = 0; i < visited_count; i++) {
        if (visited[i] == pc) return true;
    }
    return false;
}

/* Step 2: Build Control Flow Graph via Recursive Descent Parsing */
void aot_build_cfg(uint32_t entry_point, const uint8_t* text_buffer, uint32_t text_size, aot_progress_cb progress_cb) {
    if (!text_buffer || text_size == 0) return;

    uint32_t* queue = (uint32_t*)malloc(MAX_QUEUE_SIZE * sizeof(uint32_t));
    uint32_t* visited = (uint32_t*)malloc(MAX_VISITED_BLOCKS * sizeof(uint32_t));
    if (!queue || !visited) {
        free(queue);
        free(visited);
        return;
    }

    int head = 0;
    int tail = 0;
    int visited_count = 0;

    queue[tail++] = entry_point;
    visited[visited_count++] = entry_point;

    uint32_t base_va = 0x00010000; /* Standard XBE text base VA */

    while (head < tail && tail < MAX_QUEUE_SIZE - 4) {
        uint32_t current_pc = queue[head++];
        if (current_pc < base_va) continue;

        size_t offset = current_pc - base_va;
        if (offset >= text_size) continue;

        /* Scan instructions in basic block until branch or ret */
        size_t curr_off = offset;
        uint32_t block_start = current_pc;

        while (curr_off < text_size) {
            uint8_t opcode = text_buffer[curr_off];

            if (opcode == 0xC3 || opcode == 0xC2) {
                // Return instruction ends basic block
                curr_off += (opcode == 0xC2) ? 3 : 1;
                uint32_t block_end = base_va + (uint32_t)curr_off;
                xemu_aot_translate_block(block_start, block_end, text_buffer + offset);
                break;
            } else if (opcode == 0xEB) {
                // JMP rel8
                int8_t rel = (int8_t)text_buffer[curr_off + 1];
                uint32_t target = (base_va + (uint32_t)curr_off + 2) + rel;
                curr_off += 2;
                uint32_t block_end = base_va + (uint32_t)curr_off;
                xemu_aot_translate_block(block_start, block_end, text_buffer + offset);

                if (visited_count < MAX_VISITED_BLOCKS && !is_visited(target, visited, visited_count)) {
                    visited[visited_count++] = target;
                    queue[tail++] = target;
                }
                break;
            } else if (opcode == 0xE9 || opcode == 0xE8) {
                // JMP / CALL rel32
                if (curr_off + 4 < text_size) {
                    int32_t rel = 0;
                    memcpy(&rel, &text_buffer[curr_off + 1], 4);
                    uint32_t target = (base_va + (uint32_t)curr_off + 5) + rel;
                    curr_off += 5;
                    uint32_t block_end = base_va + (uint32_t)curr_off;
                    xemu_aot_translate_block(block_start, block_end, text_buffer + offset);

                    if (visited_count < MAX_VISITED_BLOCKS && !is_visited(target, visited, visited_count)) {
                        visited[visited_count++] = target;
                        queue[tail++] = target;
                    }
                    if (opcode == 0xE9) break; // Unconditional JMP ends block
                } else {
                    curr_off++;
                }
            } else if (opcode >= 0x70 && opcode <= 0x7F) {
                // Jcc rel8
                int8_t rel = (int8_t)text_buffer[curr_off + 1];
                uint32_t target = (base_va + (uint32_t)curr_off + 2) + rel;
                curr_off += 2;
                if (visited_count < MAX_VISITED_BLOCKS && !is_visited(target, visited, visited_count)) {
                    visited[visited_count++] = target;
                    queue[tail++] = target;
                }
            } else {
                curr_off++;
            }

            if (curr_off - offset > 1024) {
                // Cap max block size
                xemu_aot_translate_block(block_start, base_va + (uint32_t)curr_off, text_buffer + offset);
                break;
            }
        }

        if (progress_cb && (head % 50 == 0)) {
            int pct = (head * 100) / (tail > 0 ? tail : 1);
            progress_cb(pct > 100 ? 100 : pct, "Scanning CFG branches...");
        }
    }

    if (progress_cb) {
        progress_cb(100, "AOT CFG Pre-Compilation Complete!");
    }

    free(queue);
    free(visited);
}
