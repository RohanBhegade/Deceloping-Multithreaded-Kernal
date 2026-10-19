#include "timer/timer.h"
#include "io/io.h"

#define PIT_INPUT_FREQUENCY 1193182u
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43

static volatile uint32_t ticks;

void timer_init(uint32_t frequency)
{
    if (frequency == 0 || frequency > PIT_INPUT_FREQUENCY) {
        frequency = PIT_FREQUENCY;
    }
    uint16_t divisor = (uint16_t)(PIT_INPUT_FREQUENCY / frequency);
    ticks = 0;
    outb(PIT_COMMAND, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)(divisor >> 8));
}

void timer_on_interrupt(void)
{
    ++ticks;
}

uint32_t timer_ticks(void)
{
    return ticks;
}
