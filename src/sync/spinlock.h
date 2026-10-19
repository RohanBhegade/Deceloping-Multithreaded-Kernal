#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <stdint.h>

typedef struct spinlock {
    volatile uint32_t locked;
} spinlock_t;

typedef uint32_t irq_state_t;

#define SPINLOCK_INITIALIZER { 0 }

irq_state_t spinlock_lock_irqsave(spinlock_t* lock);
void spinlock_unlock_irqrestore(spinlock_t* lock, irq_state_t state);

#endif
