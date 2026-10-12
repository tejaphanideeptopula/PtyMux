
#include "signal_handler.h"

#include <errno.h>
#include <string.h>

static volatile sig_atomic_t resize_flag = 0;
static volatile sig_atomic_t child_flag = 0;

static struct sigaction old_winch;
static struct sigaction old_chld;
static int handlers_installed = 0;

static void handle_winch(int signo)
{
    (void)signo;
    resize_flag = 1;
}

static void handle_chld(int signo)
{
    (void)signo;
    child_flag = 1;
}

int signal_handler_init(void)
{
    if (handlers_installed)
        return 0;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_winch;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    if (sigaction(SIGWINCH, &sa, &old_winch) == -1)
        return -1;

    sa.sa_handler = handle_chld;

    if (sigaction(SIGCHLD, &sa, &old_chld) == -1) {
        int saved_errno = errno;
        (void)sigaction(SIGWINCH, &old_winch, NULL);
        errno = saved_errno;
        return -1;
    }

    resize_flag = 0;
    child_flag = 0;
    handlers_installed = 1;

    return 0;
}

void signal_handler_cleanup(void)
{
    if (!handlers_installed)
        return;

    (void)sigaction(SIGWINCH, &old_winch, NULL);
    (void)sigaction(SIGCHLD, &old_chld, NULL);

    handlers_installed = 0;
}

int signal_resize_requested(void)
{
    return resize_flag != 0;
}

void signal_clear_resize(void)
{
    resize_flag = 0;
}

int signal_child_event(void)
{
    return child_flag != 0;
}

void signal_clear_child_event(void)
{
    child_flag = 0;
}
