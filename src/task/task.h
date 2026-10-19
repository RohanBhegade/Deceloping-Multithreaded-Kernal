#ifndef TASK_H
#define TASK_H

#include <stddef.h>
#include <stdint.h>

#define TASK_MAX 8

typedef void (*task_entry_t)(void*);

struct task {
    uint32_t id;
    task_entry_t entry;
    void* arg;
    const char* name;
    int state;
    uint32_t stack[256];
    uint32_t sp;
};

void task_init(void);
int task_create(task_entry_t entry, void* arg, const char* name);
void task_yield(void);
void task_run_scheduler(void);
void task_exit(void);
void task_print_status(void);

#endif
