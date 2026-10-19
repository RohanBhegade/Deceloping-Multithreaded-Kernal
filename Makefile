CROSS ?= i686-elf-
CC := $(shell command -v $(CROSS)gcc 2>/dev/null || command -v gcc)
LD := $(shell command -v $(CROSS)ld 2>/dev/null || command -v ld)
NASM ?= nasm
GRUB_MKRESCUE ?= grub-mkrescue

HOST_GCC := $(if $(findstring /gcc,$(CC)),1,0)
ifeq ($(HOST_GCC),1)
ARCH_FLAGS := -m32
LINK_FLAGS := -m32 -no-pie
else
ARCH_FLAGS :=
LINK_FLAGS :=
endif

CFLAGS := $(ARCH_FLAGS) -std=gnu99 -ffreestanding -fno-builtin -fno-stack-protector -fno-pic -fno-pie -mno-sse -mno-mmx -msoft-float -Wall -Wextra -Werror -O2 -g -Isrc
ASFLAGS := -f elf32 -g
LDFLAGS := $(LINK_FLAGS) -nostdlib -Wl,-T,src/linker.ld -Wl,--build-id=none

C_SOURCES := src/kernel.c src/gdt/gdt.c src/idt/idt.c src/serial/serial.c src/keyboard/keyboard.c src/timer/timer.c src/scheduler/scheduler.c src/sync/spinlock.c src/menu/menu.c src/tests/selftest.c
ASM_SOURCES := src/kernel.asm src/gdt/gdt.asm src/idt/idt.asm src/io/io.asm
OBJECTS := $(patsubst src/%.c,build/%.o,$(C_SOURCES)) $(patsubst src/%.asm,build/%.asm.o,$(ASM_SOURCES))

.PHONY: all clean iso run debug check-toolchain
all: bin/kernel.elf

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

build/%.asm.o: src/%.asm
	@mkdir -p $(dir $@)
	$(NASM) $(ASFLAGS) $< -o $@

bin/kernel.elf: $(OBJECTS) src/linker.ld
	@mkdir -p bin
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

iso: bin/kernel.elf
	@mkdir -p build/isofiles/boot/grub
	cp bin/kernel.elf build/isofiles/boot/kernel.elf
	cp grub.cfg build/isofiles/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o bin/multithreados.iso build/isofiles

run: bin/kernel.elf
	qemu-system-i386 -kernel bin/kernel.elf -serial stdio -monitor none -no-reboot

debug: bin/kernel.elf
	qemu-system-i386 -kernel bin/kernel.elf -serial stdio -monitor none -no-reboot -s -S

check-toolchain:
	@echo "Compiler: $(CC)"
	@$(CC) --version | head -n 1
	@command -v $(NASM) >/dev/null || (echo "Missing NASM"; exit 1)
	@command -v $(GRUB_MKRESCUE) >/dev/null || (echo "Missing grub-mkrescue"; exit 1)

clean:
	rm -rf build bin/kernel.elf bin/multithreados.iso
