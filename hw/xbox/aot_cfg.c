#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "xbe_parser.h"

#define LOG_TAG "xemu-aot-cfg"
#define LOGI(...) printf(LOG_TAG " " __VA_ARGS__)

/* Decrypts the obfuscated Original Entry Point (OEP) using the XBE XOR keys */
uint32_t xbe_get_oep(const xbe_header* header) {
    return header->entry_point ^ 0xA8FC57AB; // Retail Key
}

extern void xemu_aot_translate_block(uint32_t start_pc, uint32_t end_pc, const uint8_t* block_buffer);

/* 
 * A minimalistic custom x86 basic block scanner.
 * In a production build, this would be replaced with Capstone.
 * For now, we scan raw bytes for branch instructions to build the CFG.
 */
void aot_build_cfg(uint32_t entry_point, const uint8_t* text_section_buffer, uint32_t text_section_size) {
    LOGI("Starting Control Flow Graph analysis at OEP: 0x%08X\n", entry_point);
    LOGI("Scanning %u bytes of .text segment...\n", text_section_size);
    
    uint32_t current_block_start = entry_point;
    uint32_t block_count = 0;
    
    for (uint32_t i = 0; i < text_section_size; i++) {
        uint8_t opcode = text_section_buffer[i];
        
        // Very basic x86 instruction length/branch decoding
        bool is_terminator = false;
        const char* term_type = "";
        
        if (opcode == 0xE8) { // CALL rel32
            is_terminator = true;
            term_type = "CALL";
            i += 4; 
        } else if (opcode == 0xE9) { // JMP rel32
            is_terminator = true;
            term_type = "JMP";
            i += 4;
        } else if (opcode == 0xEB) { // JMP rel8
            is_terminator = true;
            term_type = "JMP_SHORT";
            i += 1;
        } else if (opcode == 0xC3 || opcode == 0xC2) { // RET
            is_terminator = true;
            term_type = "RET";
        } else if ((opcode >= 0x70 && opcode <= 0x7F) || (opcode == 0x0F && i + 1 < text_section_size && text_section_buffer[i+1] >= 0x80 && text_section_buffer[i+1] <= 0x8F)) {
            // JCC rel8 or rel32
            is_terminator = true;
            term_type = "JCC";
            if (opcode == 0x0F) {
                i += 5; // 0x0F + opcode + rel32
            } else {
                i += 1; // opcode + rel8
            }
        }
        
        if (is_terminator) {
            uint32_t block_end = entry_point + i;
            
            // Log and Translate the first few blocks
            if (block_count < 10) {
                LOGI("Discovered Basic Block %u: [0x%08X - 0x%08X] Terminator: %s\n", block_count, current_block_start, block_end, term_type);
                
                uint32_t buffer_offset = current_block_start - entry_point;
                xemu_aot_translate_block(current_block_start, block_end, text_section_buffer + buffer_offset);
            }
            
            current_block_start = block_end + 1;
            block_count++;
        }
    }
    
    LOGI("CFG Generation Complete. Extracted %u Basic Blocks.\n", block_count);
}
