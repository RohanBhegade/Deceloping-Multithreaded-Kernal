#include "scheduler/scheduler.h"
#include "kernel.h"
#include "timer/timer.h"

static kernel_thread_t threads[KERNEL_MAX_THREADS];
static uint32_t thread_count;
static uint32_t next_thread_id;
static int32_t current_index;
static uint32_t context_switch_count;

static void thread_bootstrap(void)
{
    kernel_thread_t* current = &threads[current_index];
    current->entry(current->arg);
    thread_exit();
}

static void copy_name(char* destination, const char* source)
{
    uint32_t i = 0;
    if (source != 0) {
        while (i < KERNEL_THREAD_NAME_LEN - 1 && source[i] != '\0') {
            destination[i] = source[i];
            ++i;
        }
    }
    destination[i] = '\0';
}

static struct interrupt_frame* make_initial_frame(kernel_thread_t* thread)
{
    uint32_t* stack = (uint32_t*)(thread->stack + KERNEL_THREAD_STACK_SIZE);
    *--stack = (uint32_t)thread_exit;          /* Synthetic return address */
    *--stack = 0x202;                         /* EFLAGS: IF enabled */
    *--stack = 0x08;                          /* Kernel CS */
    *--stack = (uint32_t)thread_bootstrap;    /* Initial EIP */
    *--stack = 0;                             /* Error code */
    *--stack = 32;                            /* Synthetic timer vector */
    *--stack = 0x10;                          /* DS */
    *--stack = 0x10;                          /* ES */
    *--stack = 0x10;                          /* FS */
    *--stack = 0x10;                          /* GS */
    *--stack = 0;                             /* EAX */
    *--stack = 0;                             /* ECX */
    *--stack = 0;                             /* EDX */
    *--stack = 0;                             /* EBX */
    *--stack = 0;                             /* ESP dummy (popad ignores it) */
    *--stack = 0;                             /* EBP */
    *--stack = 0;                             /* ESI */
    *--stack = 0;                             /* EDI */
    return (struct interrupt_frame*)stack;
}

void scheduler_init(void)
{
    for (uint32_t i = 0; i < KERNEL_MAX_THREADS; ++i) {
        threads[i].state = THREAD_TERMINATED;
        threads[i].saved_frame = 0;
        threads[i].execution_count = 0;
    }
    thread_count = 1;
    next_thread_id = 1;
    current_index = 0;
    context_switch_count = 0;
    threads[0].id = 0;
    copy_name(threads[0].name, "console");
    threads[0].entry = 0;
    threads[0].arg = 0;
    threads[0].state = THREAD_RUNNING;
}

int scheduler_create(const char* name, thread_entry_t entry, void* arg)
{
    if (entry == 0) {
        return -1;
    }
    uint32_t flags;
    __asm__ volatile("pushfl; popl %0; cli" : "=r"(flags) :: "memory");
    uint32_t slot = 1;
    while (slot < thread_count && threads[slot].state != THREAD_TERMINATED) {
        ++slot;
    }
    if (slot == thread_count) {
        if (thread_count >= KERNEL_MAX_THREADS) {
            __asm__ volatile("pushl %0; popfl" :: "r"(flags) : "memory", "cc");
            return -1;
        }
        ++thread_count;
    }
    kernel_thread_t* thread = &threads[slot];
    thread->id = next_thread_id++;
    copy_name(thread->name, name);
    thread->entry = entry;
    thread->arg = arg;
    thread->execution_count = 0;
    thread->state = THREAD_READY;
    thread->saved_frame = make_initial_frame(thread);
    __asm__ volatile("pushl %0; popfl" :: "r"(flags) : "memory", "cc");
    return (int)thread->id;
}

struct interrupt_frame* scheduler_on_tick(struct interrupt_frame* frame)
{
    kernel_thread_t* previous = &threads[current_index];
    previous->saved_frame = frame;
    ++previous->execution_count;
    if (previous->state == THREAD_RUNNING) {
        previous->state = THREAD_READY;
    }

    uint32_t next_index = (uint32_t)current_index;
    for (uint32_t offset = 1; offset <= thread_count; ++offset) {
        uint32_t candidate = ((uint32_t)current_index + offset) % thread_count;
        if (threads[candidate].state == THREAD_READY) {
            next_index = candidate;
            break;
        }
    }

    if (next_index != (uint32_t)current_index) {
        current_index = (int32_t)next_index;
        ++context_switch_count;
    }
    threads[current_index].state = THREAD_RUNNING;
    return threads[current_index].saved_frame;
}

void thread_yield(void)
{
    __asm__ volatile("int $0x30" ::: "memory");
}

int scheduler_resume_one(void)
{
    for (uint32_t i = 1; i < thread_count; ++i) {
        if (threads[i].state == THREAD_BLOCKED) {
            threads[i].state = THREAD_READY;
            return 1;
        }
    }
    return 0;
}

void thread_exit(void)
{
    __asm__ volatile("cli" ::: "memory");
    threads[current_index].state = THREAD_TERMINATED;
    __asm__ volatile("int $0x30" ::: "memory");
    for (;;) {
        __asm__ volatile("cli; hlt" ::: "memory");
    }
}

static const char* state_name(enum thread_state state)
{
    switch (state) {
        case THREAD_READY: return "READY";
        case THREAD_RUNNING: return "RUNNING";
        case THREAD_BLOCKED: return "BLOCKED";
        default: return "TERMINATED";
    }
}

int scheduler_thread_count(void)
{
    return (int)thread_count;
}

const kernel_thread_t* scheduler_thread_at(int index)
{
    return index < 0 || (uint32_t)index >= thread_count ? 0 : &threads[index];
}

const kernel_thread_t* scheduler_current_thread(void)
{
    return &threads[current_index];
}

uint32_t scheduler_context_switches(void)
{
    return context_switch_count;
}

uint32_t scheduler_ticks(void)
{
    return timer_ticks();
}

void scheduler_print_status(void)
{
    print("ID  NAME             STATE       RUNS\n");
    for (uint32_t i = 0; i < thread_count; ++i) {
        print_u32(threads[i].id);
        print("  ");
        print(threads[i].name);
        print("  ");
        print(state_name(threads[i].state));
        print("  ");
        print_u32(threads[i].execution_count);
        print("\n");
    }
}

void scheduler_print_stats(void)
{
    print("Timer ticks: ");
    print_u32(timer_ticks());
    print("\nContext switches: ");
    print_u32(context_switch_count);
    print("\nThreads tracked: ");
    print_u32(thread_count);
    print("\n");
}
