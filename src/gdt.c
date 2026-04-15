#include "gdt.h"

// Access byte flags
#define GDT_PRESENT     0x80    // segment is present
#define GDT_RING0       0x00    // privilege level 0
#define GDT_CODE_DATA   0x10    // code/data type (not a system segment)
#define GDT_EXECUTABLE  0x08    // a code segment
#define GDT_READWRITE   0x02    // readable (code) / writable (data)

// Granularity byte flags (high nibble)
#define GDT_GRANULARITY 0x80    // limit is in 4KB pages (so 0xFFFFF * 4KB = 4GB)
#define GDT_SIZE_32     0x40    // 32-bit protected mode segment

static t_gdt_entry  gdt[3] __attribute__((section(".gdt")));
static t_gdt_ptr    gdt_ptr;

static void gdt_set_entry(int i, uint32_t base, uint32_t limit, 
    uint8_t access, uint8_t gran) {
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_mid    = (base >> 16) & 0xFF;
    gdt[i].base_high   = (base >> 24) & 0xFF;

    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].granularity = (gran & 0xF0) | ((limit >> 16) & 0x0F);

    gdt[i].access      = access;
}

// Defined in gdt.asm — does the lgdt + far jump + segment reload
extern void gdt_flush(uint32_t gdt_ptr_addr);

void gdt_init(void) {
    gdt_ptr.limit = sizeof(gdt) - 1;
    gdt_ptr.base  = (uint32_t)&gdt;

    gdt_set_entry(0, 0, 0, 0, 0);  // null descriptor

    // Code segment: base=0, limit=4GB, ring0, executable, readable
    gdt_set_entry(1, 0, 0xFFFFF,
        GDT_PRESENT | GDT_RING0 | GDT_CODE_DATA | GDT_EXECUTABLE | GDT_READWRITE,
        GDT_GRANULARITY | GDT_SIZE_32);

    // Data segment: base=0, limit=4GB, ring0, writable
    gdt_set_entry(2, 0, 0xFFFFF,
        GDT_PRESENT | GDT_RING0 | GDT_CODE_DATA | GDT_READWRITE,
        GDT_GRANULARITY | GDT_SIZE_32);

    gdt_flush((uint32_t)&gdt_ptr);
}