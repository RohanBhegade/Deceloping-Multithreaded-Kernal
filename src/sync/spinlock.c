#include "sync/spinlock.h"

irq_state_t spinlock_lock_irqsave(spinlock_t* lock)
{
    irq_state_t flags;
    __asm__ volatile("pushfl; popl %0; cli" : "=r"(flags) :: "memory");
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        while (lock->locked) {
            __asm__ volatile("pause");
        }
    }
    return flags;
}

void spinlock_unlock_irqrestore(spinlock_t* lock, irq_state_t state)
{
    __sync_lock_release(&lock->locked);
    __asm__ volatile("pushl %0; popfl" :: "r"(state) : "memory", "cc");
}
