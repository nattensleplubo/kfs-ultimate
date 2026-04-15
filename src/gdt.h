#ifndef GDT_H

# define GDT_H

#include <stdint.h>

// A single GDT entry (segment descritor), packed to exactly 8 bytes
typedef struct __attribute__((packed)) {
    uint16_t    limit_low;      // bits 0-15 of limit
    uint16_t    base_low;       // bits 0-15 of base
    uint8_t     base_mid;       // bits 16-23 of base
    uint8_t     access;         // access byte
    uint8_t     granularity;    // high nibble = flags, low nibble = limit 16-19
    uint8_t     base_high;      // bits 24-31 of base
}   t_gdt_entry;

// What lgdt receives: size of table -1 and it's address
typedef struct __attribute__((packed)) {
    uint16_t    limit;
    uint16_t    base;
}   t_gdt_ptr;

void    gdt_init(void);

#endif