#include "kernel.h"
#include "task/task.h"

static struct task g_tasks[TASK_MAX];
static int g_task_count = 0;
static int g_current_task = 0;

void task_init(void)
{
    for (int i = 0; i < TASK_MAX; i++) {
        g_tasks[i].id = 0;
        g_tasks[i].entry = 0;
        g_tasks[i].arg = 0;
        g_tasks[i].name = "idle";
        g_tasks[i].state = 0;
        g_tasks[i].sp = 0;
    }
    g_task_count = 0;
    g_current_task = 0;
}

int task_create(task_entry_t entry, void* arg, const char* name)
{
    if (entry == 0 || g_task_count >= TASK_MAX) {
        return -1;
    }

    struct task* task = &g_tasks[g_task_count];
    task->id = (uint32_t)(g_task_count + 1);
    task->entry = entry;
    task->arg = arg;
    task->name = name ? name : "unnamed";
    task->state = 1;
    task->sp = 0;

    g_task_count++;
    return (int)task->id;
}

void task_yield(void)
{
    if (g_task_count == 0) {
        return;
    }

    g_current_task = (g_current_task + 1) % g_task_count;
}

void task_run_scheduler(void)
{
    if (g_task_count == 0) {
        return;
    }

    int index = g_current_task;
    if (g_tasks[index].state && g_tasks[index].entry != 0) {
        g_tasks[index].entry(g_tasks[index].arg);
    }

    g_current_task = (g_current_task + 1) % g_task_count;
}

void task_exit(void)
{
    if (g_task_count == 0) {
        return;
    }

    g_tasks[g_current_task].state = 0;
}

void task_print_status(void)
{
    if (g_task_count == 0) {
        print("No tasks created\n");
        return;
    }

    print("Active tasks:\n");
    for (int i = 0; i < g_task_count; i++) {
        if (g_tasks[i].state) {
            print(g_tasks[i].name);
            print("\n");
        }
    }
}
