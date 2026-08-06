#ifndef XBE_PARSER_H
#define XBE_PARSER_H

#include <stdint.h>

typedef struct {
    uint32_t magic; /* 'XBEH' */
    uint32_t digital_signature[64];
    uint32_t base_address;
    uint32_t size_of_headers;
    uint32_t size_of_image;
    uint32_t size_of_image_header;
    uint32_t time_date_stamp;
    uint32_t certificate_address;
    uint32_t number_of_sections;
    uint32_t section_headers_address;
    
    // Original Entry Point (OEP) obfuscated by a magic key
    uint32_t entry_point; 
} xbe_header;

typedef struct {
    uint32_t section_flags;
    uint32_t virtual_address;
    uint32_t virtual_size;
    uint32_t raw_address;
    uint32_t raw_size;
    uint32_t section_name_address;
    uint32_t section_name_reference_count;
    uint32_t head_shared_page_reference_count_address;
    uint32_t tail_shared_page_reference_count_address;
    uint32_t section_digest[5];
} xbe_section;

// Function declarations
uint32_t xbe_get_oep(const xbe_header* header);
xbe_section* xbe_get_text_section(const xbe_header* header, const xbe_section* sections);

#endif // XBE_PARSER_H
