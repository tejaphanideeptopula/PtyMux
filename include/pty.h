
#ifndef PTYMUX_PTY_H
#define PTYMUX_PTY_H

#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>

typedef struct {
    int master_fd;
    pid_t child_pid;
} Pty;

int pty_spawn(Pty *pty, const char *shell);
int pty_resize(int master_fd, unsigned short rows, unsigned short cols);
void pty_close(Pty *pty);

#endif
