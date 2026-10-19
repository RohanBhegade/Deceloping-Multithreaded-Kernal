#include "kernel.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "io/io.h"
#include "keyboard/keyboard.h"
#include "menu/menu.h"
#include "scheduler/scheduler.h"
#include "serial/serial.h"
#include "sync/spinlock.h"
#include "timer/timer.h"

static volatile uint16_t* const video_memory = (uint16_t*)0xB8000;
static uint16_t terminal_row;
static uint16_t terminal_column;
static spinlock_t console_lock = SPINLOCK_INITIALIZER;
static volatile uint32_t worker_progress[3];

static void terminal_scroll(void)
{
    for (uint32_t row = 1; row < VGA_HEIGHT; ++row) {
        for (uint32_t column = 0; column < VGA_WIDTH; ++column) {
            video_memory[(row - 1) * VGA_WIDTH + column] = video_memory[row * VGA_WIDTH + column];
        }
    }
    for (uint32_t column = 0; column < VGA_WIDTH; ++column) {
        video_memory[(VGA_HEIGHT - 1) * VGA_WIDTH + column] = (uint16_t)(0x0F00 | ' ');
    }
    terminal_row = VGA_HEIGHT - 1;
}

static void terminal_putchar(char c)
{
    if (c == '\r') {
        return;
    }
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row >= VGA_HEIGHT) {
            terminal_scroll();
        }
        return;
    }
    video_memory[terminal_row * VGA_WIDTH + terminal_column] = (uint16_t)(0x0F00 | (uint8_t)c);
    if (++terminal_column >= VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row >= VGA_HEIGHT) {
            terminal_scroll();
        }
    }
}

void console_clear(void)
{
    irq_state_t flags = spinlock_lock_irqsave(&console_lock);
    terminal_row = 0;
    terminal_column = 0;
    for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; ++i) {
        video_memory[i] = (uint16_t)(0x0F00 | ' ');
    }
    spinlock_unlock_irqrestore(&console_lock, flags);
}

void print(const char* text)
{
    if (text == 0) {
        return;
    }
    irq_state_t flags = spinlock_lock_irqsave(&console_lock);
    while (*text) {
        if (*text == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*text);
        terminal_putchar(*text++);
    }
    spinlock_unlock_irqrestore(&console_lock, flags);
}

void print_u32(uint32_t value)
{
    char digits[11];
    uint32_t count = 0;
    do {
        digits[count++] = (char)('0' + value % 10u);
        value /= 10u;
    } while (value != 0);
    char output[11];
    uint32_t i = 0;
    while (count != 0) {
        output[i++] = digits[--count];
    }
    output[i] = '\0';
    print(output);
}

void kernel_panic(const char* message)
{
    __asm__ volatile("cli" ::: "memory");
    print("\nKERNEL PANIC: ");
    print(message);
    print("\nSystem halted.\n");
    for (;;) {
        __asm__ volatile("hlt");
    }
}

static void demonstration_worker(void* argument)
{
    uint32_t worker = (uint32_t)argument;
    for (;;) {
        ++worker_progress[worker];
        __asm__ volatile("pause");
    }
}

void kernel_main(uint32_t magic, uint32_t multiboot_info)
{
    (void)multiboot_info;
    serial_init();
    console_clear();
    print("MultiThreadOS - Educational Kernel\n");
    print("Architecture: x86 32-bit\n");
    print("Scheduler: Preemptive Round Robin\n");
    print("Timer: PIT\n");
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        kernel_panic("Invalid Multiboot boot magic");
    }

    gdt_init();
    idt_init();
    keyboard_init();
    scheduler_init();
    timer_init(PIT_FREQUENCY);
    pic_init();

    if (scheduler_create("worker-A", demonstration_worker, (void*)0) < 0 ||
        scheduler_create("worker-B", demonstration_worker, (void*)1) < 0 ||
        scheduler_create("worker-C", demonstration_worker, (void*)2) < 0) {
        kernel_panic("Unable to create initial demonstration threads");
    }

    print("Status: Kernel Initialized\n\n");
    menu_initialize();
    __asm__ volatile("sti" ::: "memory");
    menu_run();
    kernel_panic("Kernel console unexpectedly returned");
}
