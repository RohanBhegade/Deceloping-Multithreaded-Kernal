[BITS 32]

section .multiboot
align 4
dd 0x1BADB002
dd 0x00000003
dd -(0x1BADB002 + 0x00000003)

section .bss
align 16
boot_stack_bottom:
    resb 16384
boot_stack_top:

section .text
global _start
extern kernel_main

_start:
    cli
    mov esp, boot_stack_top
    xor ebp, ebp
    push ebx                    ; Multiboot information structure
    push eax                    ; Multiboot boot magic
    call kernel_main
.halt:
    cli
    hlt
    jmp .halt

section .note.GNU-stack noalloc noexec nowrite progbits
