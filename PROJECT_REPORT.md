# MultiThreadOS Project Report

## Abstract

MultiThreadOS is a small educational operating-system kernel for 32-bit x86. It demonstrates a Multiboot boot path, protected-mode descriptor tables, hardware timer interrupts, kernel thread stacks, preemptive scheduling, synchronization, and an interactive console. It is intended to make the timer-to-context-switch path observable and understandable rather than to provide a production operating system.

## Objectives

- Boot a freestanding kernel under GRUB Multiboot or QEMU's Multiboot ELF loader.
- Create independent kernel execution contexts with separate stacks.
- Preempt a running context using PIT IRQ0 and dispatch runnable threads round-robin.
- Show thread and scheduler diagnostics through VGA and UART.
- Compare deliberately unsynchronized counter updates with an IRQ-safe spinlock.
- Execute the build and tests under QEMU rather than simulating operating-system behavior in a host application.

## Architecture and design

The boot assembly publishes a Multiboot v1 header, reserves a bootstrap stack, and transfers the Multiboot magic/info values to C. The kernel reloads its flat GDT, installs IDT entries, remaps the master/slave PIC, configures the PIT for 100 Hz, and enables interrupts only after initialization is complete.

Each thread control block contains an ID, name, entry function, argument, state, execution count, saved interrupt-frame pointer, and an 8 KiB statically allocated stack. The console is represented as thread ID 0. Three long-running workers are created during startup. The table has eight slots; terminated slots can be reused and IDs continue increasing.

On PIT IRQ0, the assembly stub saves segment and general registers and calls the C interrupt dispatcher. The scheduler stores that frame in the current TCB, advances the runnable selection, updates states/counters, and returns the selected TCB's saved-frame address. Assembly switches ESP to that address, restores state, and returns with `iretd`. New threads start from a synthetic frame with the same layout. Voluntary yield uses a separate software interrupt vector, keeping the PIT tick count specific to hardware IRQs.

The keyboard driver queues PS/2 make codes; the UART driver provides nonblocking receive and serialized output. The console accepts input from either source. Its menu invokes scheduler, status, stack diagnostics, timer, and test functions directly.

## Synchronization experiment

The race test uses two threads that read a shared value, yield, and write back the increment. The yield between read and write intentionally creates a lost-update window; the expected demonstration output is below the mathematical total. The protected test then has two preemptible workers increment a shared value 10,000 times each under an atomic spinlock. The lock saves and restores interrupt state, preventing both competing-thread races and local interrupt reentrancy while held. The expected protected total is 20,000.

## Build and test results

The current workspace was built with system GCC's 32-bit freestanding mode, NASM, GNU ld, and QEMU:

- Clean build: `make clean && make -j2` succeeded and linked an ELF32/i386 kernel.
- `grub-file --is-x86-multiboot bin/kernel.elf` accepted the Multiboot header.
- `make run` booted the kernel in QEMU and printed the boot banner/menu over UART.
- UART-driven menu tests displayed the console and three worker TCBs; PIT ticks and context-switch counts increased.
- The race test observed 16 increments from 32 attempts; the locked test reported 20,000/20,000.
- Self-tests passed thread creation/execution, a round-robin context switch, and synchronization.

The GRUB ISO target is provided, but was not executed in this environment because `xorriso` is not installed. Direct ELF boot is what was tested. The actual emitted boot text, including current output, is documented in the README; no screenshot or ISO test is claimed.

## Limitations

- Single-core, ring-0 only, 32-bit x86; no user tasks or address-space isolation.
- Fixed-size thread table and static stacks; there is no allocator-backed thread stack reclamation or stack guard.
- The BLOCKED state is reserved, but blocking/wakeup and I/O wait queues are not implemented.
- Spinlocks are provided; a sleeping mutex is not implemented.
- No safe Multiboot memory-map allocator is wired in. Paging and the previous fixed-address heap initialization are disabled.
- Exceptions other than configured timer/keyboard IRQ paths halt instead of producing a diagnostic register dump.
- No filesystem, SMP, device abstraction layer, process ABI, or power-management support.
- PIT scheduling is fixed-rate and not calibrated as CPU time; workers are busy loops for demonstration.

## Future improvements

1. Parse Multiboot memory maps and add a physical-frame allocator before enabling paging or a dynamic heap.
2. Add guard pages/stack canaries and verify stack boundaries on context switches.
3. Implement wait queues, blocking mutexes, and keyboard input blocking.
4. Add exception-specific stubs with error-code handling and diagnostic register output.
5. Add a proper idle task and halt-based idle loop when no worker is READY.
6. Add ring-3 tasks, TSS setup, system calls, and per-process address spaces.
7. Add SMP-aware scheduler locks and inter-processor interrupts only after uniprocessor correctness is established.
8. Automate ISO creation and QEMU transcript assertions in CI with the required GRUB/xorriso packages.

## Reproduction

Install dependencies and run commands in the README. For interactive tests, start `make run`, enter menu option `10`, and review the printed thread/scheduling and synchronization results. For GDB, start `make debug`, connect to port 1234, and continue execution.
