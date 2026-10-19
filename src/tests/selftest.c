#include "tests/selftest.h"
#include "kernel.h"
#include "scheduler/scheduler.h"
#include "sync/spinlock.h"

#define SYNC_TEST_ITERATIONS 10000u
#define RACE_TEST_ITERATIONS 16u

static volatile uint32_t selftest_probe_count;
static volatile uint32_t race_test_counter;
static volatile uint32_t race_test_workers_done;
static volatile uint32_t sync_test_counter;
static volatile uint32_t sync_test_workers_done;
static spinlock_t sync_test_lock = SPINLOCK_INITIALIZER;

static void probe_thread(void* argument)
{
    (void)argument;
    ++selftest_probe_count;
}

static void sync_worker(void* argument)
{
    (void)argument;
    for (uint32_t i = 0; i < SYNC_TEST_ITERATIONS; ++i) {
        irq_state_t state = spinlock_lock_irqsave(&sync_test_lock);
        ++sync_test_counter;
        spinlock_unlock_irqrestore(&sync_test_lock, state);
    }
    irq_state_t state = spinlock_lock_irqsave(&sync_test_lock);
    ++sync_test_workers_done;
    spinlock_unlock_irqrestore(&sync_test_lock, state);
}

static void race_worker(void* argument)
{
    (void)argument;
    for (uint32_t i = 0; i < RACE_TEST_ITERATIONS; ++i) {
        uint32_t value = race_test_counter;
        thread_yield();
        race_test_counter = value + 1;
    }
    irq_state_t state = spinlock_lock_irqsave(&sync_test_lock);
    ++race_test_workers_done;
    spinlock_unlock_irqrestore(&sync_test_lock, state);
}

void synchronization_test_run(void)
{
    race_test_counter = 0;
    race_test_workers_done = 0;
    int race_first = scheduler_create("race-A", race_worker, 0);
    int race_second = scheduler_create("race-B", race_worker, 0);
    if (race_first < 0 || race_second < 0) {
        print("RACE TEST: unable to start; thread table is full\n");
        return;
    }
    print("Race demonstration: two threads perform split read/yield/write increments without a lock...\n");
    while (race_test_workers_done < 2) {
        thread_yield();
    }
    print("Unsynchronized value: ");
    print_u32(race_test_counter);
    print(" (expected ");
    print_u32(RACE_TEST_ITERATIONS * 2);
    print(")\n");

    sync_test_counter = 0;
    sync_test_workers_done = 0;
    int first = scheduler_create("sync-A", sync_worker, 0);
    int second = scheduler_create("sync-B", sync_worker, 0);
    if (first < 0 || second < 0) {
        print("SYNC TEST: FAIL - thread table is full\n");
        return;
    }
    print("Running two preempted workers, each incrementing the shared value ");
    print_u32(SYNC_TEST_ITERATIONS);
    print(" times under an IRQ-safe spinlock...\n");
    while (sync_test_workers_done < 2) {
        thread_yield();
    }
    uint32_t expected = SYNC_TEST_ITERATIONS * 2;
    print("Shared value: ");
    print_u32(sync_test_counter);
    print(" (expected ");
    print_u32(expected);
    print(")\n");
    print(sync_test_counter == expected ? "SYNC TEST: PASS\n" : "SYNC TEST: FAIL\n");
}

void kernel_selftests_run(void)
{
    print("Kernel self-tests starting...\n");
    selftest_probe_count = 0;
    uint32_t before_switches = scheduler_context_switches();
    int id = scheduler_create("selftest", probe_thread, 0);
    if (id < 0) {
        print("Thread creation: FAIL - thread table full\n");
        return;
    }
    while (selftest_probe_count == 0) {
        thread_yield();
    }
    print("Thread creation and execution: PASS (thread ID ");
    print_u32((uint32_t)id);
    print(")\n");
    print(scheduler_context_switches() > before_switches ?
          "Round-robin context switching: PASS\n" :
          "Round-robin context switching: FAIL\n");
    synchronization_test_run();
    print("Kernel self-tests complete.\n");
}
