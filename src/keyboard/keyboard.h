#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_init(void);
char keyboard_read_char(void);
void keyboard_handle_interrupt(void);
char keyboard_serial_read_char(void);

#endif
