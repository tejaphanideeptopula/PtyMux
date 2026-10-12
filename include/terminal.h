
#ifndef PTYMUX_TERMINAL_H
#define PTYMUX_TERMINAL_H

#include <termios.h>
#include <sys/ioctl.h>

typedef struct {
    struct termios original_termios;
    int raw_mode_enabled;
} Terminal;

int terminal_enable_raw(Terminal *terminal);
void terminal_restore(Terminal *terminal);
int terminal_get_size(unsigned short *rows, unsigned short *cols);

#endif
