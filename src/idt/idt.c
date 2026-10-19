#include "idt/idt.h"
#include "io/io.h"
#include "keyboard/keyboard.h"
#include "scheduler/scheduler.h"
#include "timer/timer.h"

#define IDT_ENTRIES 256
#define KERNEL_CODE_SELECTOR 0x08
#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_COMMAND 0xA0
#define PIC_SLAVE_DATA 0xA1

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t attributes;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_pointer {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[IDT_ENTRIES];
static struct idt_pointer idtr;
extern void irq_timer(void);
extern void irq_keyboard(void);
extern void irq_yield(void);
extern void exception_stub(void);

static void idt_set_gate(uint8_t vector, void (*handler)(void), uint8_t attributes)
{
    uint32_t address = (uint32_t)handler;
    idt[vector].offset_low = (uint16_t)(address & 0xFFFFu);
    idt[vector].selector = KERNEL_CODE_SELECTOR;
    idt[vector].zero = 0;
    idt[vector].attributes = attributes;
    idt[vector].offset_high = (uint16_t)(address >> 16);
}

void idt_init(void)
{
    for (uint32_t i = 0; i < IDT_ENTRIES; ++i) {
        idt_set_gate((uint8_t)i, exception_stub, 0x8E);
    }
    idt_set_gate(32, irq_timer, 0x8E);
    idt_set_gate(33, irq_keyboard, 0x8E);
    idt_set_gate(48, irq_yield, 0x8E);
    idtr.limit = (uint16_t)(sizeof(idt) - 1);
    idtr.base = (uint32_t)idt;
    idt_load(&idtr);
}

void pic_init(void)
{
    outb(PIC_MASTER_COMMAND, 0x11);
    outb(PIC_SLAVE_COMMAND, 0x11);
    outb(PIC_MASTER_DATA, 0x20);
    outb(PIC_SLAVE_DATA, 0x28);
    outb(PIC_MASTER_DATA, 0x04);
    outb(PIC_SLAVE_DATA, 0x02);
    outb(PIC_MASTER_DATA, 0x01);
    outb(PIC_SLAVE_DATA, 0x01);
    outb(PIC_MASTER_DATA, 0xFC);
    outb(PIC_SLAVE_DATA, 0xFF);
}

struct interrupt_frame* interrupt_dispatch(struct interrupt_frame* frame)
{
    if (frame->interrupt_number == 32) {
        timer_on_interrupt();
        frame = scheduler_on_tick(frame);
        outb(PIC_MASTER_COMMAND, 0x20);
    } else if (frame->interrupt_number == 33) {
        keyboard_handle_interrupt();
        outb(PIC_MASTER_COMMAND, 0x20);
    } else if (frame->interrupt_number == 48) {
        frame = scheduler_on_tick(frame);
    }
    return frame;
}
