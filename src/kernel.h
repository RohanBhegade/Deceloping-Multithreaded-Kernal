#ifndef KERNEL_H
#define KERNEL_H

#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002u

void kernel_main(uint32_t magic, uint32_t multiboot_info);
void print(const char* text);
void print_u32(uint32_t value);
void console_clear(void);
void kernel_panic(const char* message);

#endif