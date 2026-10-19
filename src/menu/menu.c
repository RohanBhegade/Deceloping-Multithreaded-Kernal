#include "menu/menu.h"
#include "kernel.h"
#include "io/io.h"
#include "keyboard/keyboard.h"
#include "scheduler/scheduler.h"
#include "tests/selftest.h"
#include "timer/timer.h"

static volatile uint32_t menu_test_thread_runs;

static void menu_test_thread(void* argument)
{
    uint32_t id = (uint32_t)argument;
    for (uint32_t i = 0; i < 200000u; ++i) {
        ++menu_test_thread_runs;
        __asm__ volatile("pause");
    }
    print("Test thread ");
    print_u32(id);
    print(" finished; local loop count=200000\n");
}

void menu_initialize(void)
{
    menu_test_thread_runs = 0;
}

void menu_init(void)
{
    menu_initialize();
}

void menu_print_main(void)
{
    print("\n========== MultiThreadOS Console ==========\n");
    print(" 1. Display kernel information\n");
    print(" 2. List all threads\n");
    print(" 3. Create a new test thread\n");
    print(" 4. Start/resume a runnable thread\n");
    print(" 5. Display scheduler statistics\n");
    print(" 6. Run shared-counter synchronization test\n");
    print(" 7. Demonstrate round-robin scheduling\n");
    print(" 8. Display timer tick count\n");
    print(" 9. Show memory/thread-stack information\n");
    print("10. Run kernel self-tests\n");
    print("11. Display help\n");
    print("12. Shut down/reboot emulator\n");
    print("Select an option and press Enter: ");
}

static void print_kernel_information(void)
{
    print("MultiThreadOS - Educational Kernel\nArchitecture: x86 32-bit protected mode\n");
    print("Boot: GRUB Multiboot v1\nScheduler: PIT preemptive round-robin\n");
    print("Timer frequency: ");
    print_u32(PIT_FREQUENCY);
    print(" Hz\nKernel threads tracked: ");
    print_u32((uint32_t)scheduler_thread_count());
    print("\n");
}

static void create_test_thread(void)
{
    menu_test_thread_runs = 0;
    int id = scheduler_create("menu-test", menu_test_thread, (void*)scheduler_thread_count());
    if (id < 0) {
        print("ERROR: no reusable thread slot is available.\n");
        return;
    }
    print("Created READY test thread with ID ");
    print_u32((uint32_t)id);
    print(". It will run preemptively and then terminate.\n");
}

static void show_memory_information(void)
{
    print("Static kernel thread stacks (bytes each): ");
    print_u32(KERNEL_THREAD_STACK_SIZE);
    print("\nThread ID | State | Stack low .. high\n");
    for (int i = 0; i < scheduler_thread_count(); ++i) {
        const kernel_thread_t* thread = scheduler_thread_at(i);
        print_u32(thread->id);
        print(" | ");
        switch (thread->state) {
            case THREAD_READY: print("READY"); break;
            case THREAD_RUNNING: print("RUNNING"); break;
            case THREAD_BLOCKED: print("BLOCKED"); break;
            default: print("TERMINATED"); break;
        }
        print(" | ");
        print_u32((uint32_t)thread->stack);
        print(" .. ");
        print_u32((uint32_t)(thread->stack + KERNEL_THREAD_STACK_SIZE));
        print("\n");
    }
    print("Thread storage is statically reserved in kernel BSS; no user-mode heap is enabled.\n");
}

static void print_help(void)
{
    print("Enter an option number (1-12), then press Enter.\n");
    print("The menu accepts PS/2 keyboard input in graphical QEMU and UART input\n");
    print("when QEMU is started with -serial stdio. Threads run independently\n");
    print("while this console waits; the PIT preempts the current thread every 10 ms.\n");
    print("Created jobs are READY immediately; option 4 resumes BLOCKED threads,\n");
    print("or voluntarily yields when none are blocked.\n");
}

static void reboot_emulator(void)
{
    print("Requesting QEMU shutdown; falling back to the keyboard-controller reset.\n");
    outw(0x604, 0x2000);
    uint8_t status;
    do {
        status = insb(0x64);
    } while (status & 0x02);
    outb(0x64, 0xFE);
    for (;;) {
        __asm__ volatile("cli; hlt");
    }
}

void menu_execute(int choice)
{
    switch (choice) {
        case 1:
            print_kernel_information();
            break;
        case 2:
            scheduler_print_status();
            break;
        case 3:
            create_test_thread();
            break;
        case 4:
            if (scheduler_resume_one()) {
                print("A BLOCKED thread was moved to READY.\n");
            } else {
                print("No BLOCKED thread exists in this build; yielding to the next READY thread.\n");
                thread_yield();
            }
            break;
        case 5:
            scheduler_print_stats();
            break;
        case 6:
            synchronization_test_run();
            break;
        case 7:
            print("Round-robin demonstration: yielding for 12 scheduling decisions.\n");
            for (uint32_t i = 0; i < 12; ++i) {
                thread_yield();
            }
            scheduler_print_status();
            scheduler_print_stats();
            break;
        case 8:
            print("PIT timer ticks: ");
            print_u32(timer_ticks());
            print(" (100 Hz nominal)\n");
            break;
        case 9:
            show_memory_information();
            break;
        case 10:
            kernel_selftests_run();
            break;
        case 11:
            print_help();
            break;
        case 12:
            reboot_emulator();
            break;
        default:
            print("Invalid option. Enter a number from 1 to 12.\n");
            break;
    }
}

static char read_console_char(void)
{
    char character = keyboard_read_char();
    return character != 0 ? character : keyboard_serial_read_char();
}

void menu_run(void)
{
    char line[4];
    uint32_t length = 0;
    menu_print_main();
    for (;;) {
        char character = read_console_char();
        if (character == 0) {
            __asm__ volatile("pause");
            continue;
        }
        if (character == '\r') {
            character = '\n';
        }
        if (character == '\n') {
            print("\n");
            if (length == 0) {
                menu_print_main();
                continue;
            }
            int choice = 0;
            for (uint32_t i = 0; i < length; ++i) {
                if (line[i] < '0' || line[i] > '9') {
                    choice = -1;
                    break;
                }
                choice = choice * 10 + (line[i] - '0');
            }
            length = 0;
            menu_execute(choice);
            menu_print_main();
            continue;
        }
        if (character == '\b' || character == 0x7F) {
            if (length != 0) {
                --length;
                print("\b \b");
            }
            continue;
        }
        if (character >= '0' && character <= '9' && length < sizeof(line) - 1) {
            line[length++] = character;
            char echo[2] = { character, '\0' };
            print(echo);
        }
    }
}
