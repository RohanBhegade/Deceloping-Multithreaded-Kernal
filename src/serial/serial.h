#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

void serial_init(void);
void serial_write_char(char c);
void serial_write_string(const char* str);
int serial_try_read_char(char* character);

#endif
