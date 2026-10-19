#include "gdt/gdt.h"

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_pointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[3];
static struct gdt_pointer gdtr;
extern void gdt_load(const struct gdt_pointer* pointer);

static void set_entry(uint32_t index, uint8_t access, uint8_t flags)
{
    gdt[index].limit_low = 0xFFFF;
    gdt[index].base_low = 0;
    gdt[index].base_middle = 0;
    gdt[index].access = access;
    gdt[index].granularity = (uint8_t)(0x0F | flags);
    gdt[index].base_high = 0;
}

void gdt_init(void)
{
    set_entry(0, 0, 0);
    set_entry(1, 0x9A, 0xC0);
    set_entry(2, 0x92, 0xC0);
    gdtr.limit = (uint16_t)(sizeof(gdt) - 1);
    gdtr.base = (uint32_t)gdt;
    gdt_load(&gdtr);
}
