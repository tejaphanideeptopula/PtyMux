
#include "pty.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int pty_spawn(Pty *pty, const char *shell)
{
    if (pty == NULL) {
        errno = EINVAL;
        return -1;
    }

    pty->master_fd = -1;
    pty->child_pid = -1;

    int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (master_fd == -1) {
        return -1;
    }

    if (grantpt(master_fd) == -1 || unlockpt(master_fd) == -1) {
        int saved_errno = errno;
        close(master_fd);
        errno = saved_errno;
        return -1;
    }

    char *slave_name = ptsname(master_fd);
    if (slave_name == NULL) {
        int saved_errno = errno;
        close(master_fd);
        errno = saved_errno;
        return -1;
    }

    char *slave_path = strdup(slave_name);
    if (slave_path == NULL) {
        close(master_fd);
        return -1;
    }

    const char *selected_shell = shell;

    if (selected_shell == NULL || selected_shell[0] == '\0') {
        selected_shell = getenv("SHELL");
    }

    if (selected_shell == NULL || selected_shell[0] == '\0') {
        selected_shell = "/bin/bash";
    }

    char *shell_path = strdup(selected_shell);
    if (shell_path == NULL) {
        free(slave_path);
        close(master_fd);
        return -1;
    }

    pid_t pid = fork();

    if (pid == -1) {
        int saved_errno = errno;
        free(shell_path);
        free(slave_path);
        close(master_fd);
        errno = saved_errno;
        return -1;
    }

    if (pid == 0) {
        close(master_fd);

        if (setsid() == -1) {
            perror("setsid");
            _exit(127);
        }

        int slave_fd = open(slave_path, O_RDWR);
        if (slave_fd == -1) {
            perror("open PTY slave");
            _exit(127);
        }

        if (ioctl(slave_fd, TIOCSCTTY, 0) == -1) {
            perror("TIOCSCTTY");
            _exit(127);
        }

        struct winsize ws;
        memset(&ws, 0, sizeof(ws));
        ws.ws_row = 24;
        ws.ws_col = 80;

        if (ioctl(slave_fd, TIOCSWINSZ, &ws) == -1) {
            perror("TIOCSWINSZ");
            _exit(127);
        }

        if (dup2(slave_fd, STDIN_FILENO) == -1 ||
            dup2(slave_fd, STDOUT_FILENO) == -1 ||
            dup2(slave_fd, STDERR_FILENO) == -1) {
            perror("dup2");
            _exit(127);
        }

        if (slave_fd > STDERR_FILENO) {
            close(slave_fd);
        }

        setenv("PS1", "ptymux$ ", 1);
        setenv("TERM", "xterm-256color", 1);
        setenv("PTYMUX_ACTIVE", "1", 1);

        execl(shell_path, shell_path, "--norc", "-i", (char *)NULL);

        perror("exec shell");
        _exit(127);
    }

    free(shell_path);
    free(slave_path);

    pty->master_fd = master_fd;
    pty->child_pid = pid;

    return 0;
}

int pty_resize(int master_fd, unsigned short rows, unsigned short cols)
{
    if (master_fd < 0) {
        errno = EINVAL;
        return -1;
    }

    struct winsize ws;
    memset(&ws, 0, sizeof(ws));

    ws.ws_row = rows;
    ws.ws_col = cols;

    return ioctl(master_fd, TIOCSWINSZ, &ws);
}

void pty_close(Pty *pty)
{
    if (pty == NULL) {
        return;
    }

    if (pty->master_fd >= 0) {
        close(pty->master_fd);
        pty->master_fd = -1;
    }

    if (pty->child_pid > 0) {
        pid_t pid = pty->child_pid;

        if (kill(pid, SIGHUP) == -1 && errno != ESRCH) {
            perror("kill");
        }

        while (waitpid(pid, NULL, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        pty->child_pid = -1;
    }
}
