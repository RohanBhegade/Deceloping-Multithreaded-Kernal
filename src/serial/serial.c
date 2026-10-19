#include "serial/serial.h"
#include "io/io.h"

#define SERIAL_PORT 0x3F8

void serial_init(void)
{
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x80);
    outb(SERIAL_PORT + 0, 0x01);
    outb(SERIAL_PORT + 1, 0x00);
    outb(SERIAL_PORT + 3, 0x03);
    outb(SERIAL_PORT + 2, 0xC7);
    outb(SERIAL_PORT + 4, 0x0B);
}

void serial_write_char(char c)
{
    while ((insb(0x3FD) & 0x20) == 0) {
    }
    outb(0x3F8, (unsigned char)c);
}

void serial_write_string(const char* str)
{
    while (*str) {
        serial_write_char(*str);
        str++;
    }
}

int serial_try_read_char(char* character)
{
    if ((insb(SERIAL_PORT + 5) & 0x01) == 0) {
        return 0;
    }
    *character = (char)insb(SERIAL_PORT);
    return 1;
}
