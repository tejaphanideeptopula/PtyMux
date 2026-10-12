
#include "terminal.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

int terminal_enable_raw(Terminal *terminal)
{
    if (terminal == NULL) {
        errno = EINVAL;
        return -1;
    }

    terminal->raw_mode_enabled = 0;

    if (!isatty(STDIN_FILENO)) {
        errno = ENOTTY;
        return -1;
    }

    if (tcgetattr(STDIN_FILENO, &terminal->original_termios) == -1)
        return -1;

    struct termios raw = terminal->original_termios;

    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    /*
     * Keep OPOST enabled so the terminal processes output newlines.
     */
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        return -1;

    terminal->raw_mode_enabled = 1;
    return 0;
}

void terminal_restore(Terminal *terminal)
{
    if (terminal == NULL || !terminal->raw_mode_enabled)
        return;

    (void)tcsetattr(STDIN_FILENO, TCSAFLUSH,
                    &terminal->original_termios);

    terminal->raw_mode_enabled = 0;
}

int terminal_get_size(unsigned short *rows, unsigned short *cols)
{
    if (rows == NULL || cols == NULL) {
        errno = EINVAL;
        return -1;
    }

    struct winsize ws;
    memset(&ws, 0, sizeof(ws));

    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == -1)
        return -1;

    if (ws.ws_row == 0 || ws.ws_col == 0) {
        errno = EINVAL;
        return -1;
    }

    *rows = ws.ws_row;
    *cols = ws.ws_col;

    return 0;
}
