#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

#define PIT_FREQUENCY 100u

void timer_init(uint32_t frequency);
void timer_on_interrupt(void);
uint32_t timer_ticks(void);

#endif
