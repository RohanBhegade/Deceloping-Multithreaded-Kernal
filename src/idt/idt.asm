[BITS 32]
section .text

extern interrupt_dispatch

global irq_common
global exception_stub
global idt_load
global irq_timer
global irq_keyboard
global irq_yield

idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

irq_timer:
    push dword 0
    push dword 32
    jmp irq_common

irq_keyboard:
    push dword 0
    push dword 33
    jmp irq_common

irq_yield:
    push dword 0
    push dword 48
    jmp irq_common

irq_common:
    push ds
    push es
    push fs
    push gs
    pushad
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    push esp
    call interrupt_dispatch
    add esp, 4
    mov esp, eax
    popad
    pop gs
    pop fs
    pop es
    pop ds
    add esp, 8
    iretd

exception_stub:
    cli
.exception_halt:
    hlt
    jmp .exception_halt

section .note.GNU-stack noalloc noexec nowrite progbits

