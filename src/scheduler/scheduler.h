#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>
#include "idt/idt.h"

#define KERNEL_MAX_THREADS 8
#define KERNEL_THREAD_NAME_LEN 16
#define KERNEL_THREAD_STACK_SIZE 8192

enum thread_state {
    THREAD_READY = 0,
    THREAD_RUNNING = 1,
    THREAD_BLOCKED = 2,
    THREAD_TERMINATED = 3
};

typedef void (*thread_entry_t)(void*);

typedef struct kernel_thread {
    uint32_t id;
    char name[KERNEL_THREAD_NAME_LEN];
    thread_entry_t entry;
    void* arg;
    enum thread_state state;
    struct interrupt_frame* saved_frame;
    uint32_t execution_count;
    uint8_t stack[KERNEL_THREAD_STACK_SIZE] __attribute__((aligned(16)));
} kernel_thread_t;

void scheduler_init(void);
int scheduler_create(const char* name, thread_entry_t entry, void* arg);
struct interrupt_frame* scheduler_on_tick(struct interrupt_frame* frame);
void scheduler_print_status(void);
void scheduler_print_stats(void);
int scheduler_thread_count(void);
const kernel_thread_t* scheduler_thread_at(int index);
uint32_t scheduler_context_switches(void);
uint32_t scheduler_ticks(void);
void thread_yield(void);
void thread_exit(void);
int scheduler_resume_one(void);
const kernel_thread_t* scheduler_current_thread(void);

#endif
