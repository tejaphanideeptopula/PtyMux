
#include "multiplexer.h"
#include "terminal.h"
#include "commands.h"
#include "signal_handler.h"
#include "buffer.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>

#define PREFIX_KEY 2
#define IO_BUFFER_SIZE 4096

static int write_all(int fd, const char *data, size_t length)
{
    size_t written = 0;

    while (written < length) {
        ssize_t n = write(fd, data + written, length - written);

        if (n > 0) {
            written += (size_t)n;
        } else if (n == -1 && errno == EINTR) {
            continue;
        } else {
            return -1;
        }
    }

    return 0;
}

static void show_banner(void)
{
    static const char banner[] =
        "\033[2J\033[H"
        "=====================================\r\n"
        "               PtyMux                \r\n"
        "      A Terminal Multiplexer in C    \r\n"
        "=====================================\r\n"
        "Ctrl+B, n     Create a new shell\r\n"
        "Ctrl+B, 1-9   Switch sessions\r\n"
        "Ctrl+B, l     List sessions\r\n"
        "Ctrl+B, k     Kill active session\r\n"
        "Ctrl+B, ?     Show help\r\n"
        "Ctrl+B, q     Quit PtyMux\r\n"
        "=====================================\r\n\r\n";

    (void)write_all(STDOUT_FILENO, banner, sizeof(banner) - 1);
}

static void resize_sessions(SessionManager *manager)
{
    unsigned short rows;
    unsigned short cols;

    if (terminal_get_size(&rows, &cols) == -1)
        return;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        Session *session = &manager->sessions[i];

        if (session->state == SESSION_RUNNING &&
            session->pty.master_fd >= 0) {
            (void)pty_resize(session->pty.master_fd, rows, cols);
        }
    }
}

static void repaint_active(SessionManager *manager)
{
    Session *active = session_active(manager);

    static const char clear[] = "\033[2J\033[H";
    (void)write_all(STDOUT_FILENO, clear, sizeof(clear) - 1);

    if (active == NULL) {
        static const char message[] =
            "No active shell. Press Ctrl+B, then n.\r\n";
        (void)write_all(STDOUT_FILENO, message, sizeof(message) - 1);
        return;
    }

    RingBuffer *buffer = &active->output;
    size_t position = buffer->head;
    size_t remaining = buffer->size;

    while (remaining > 0) {
        size_t length = BUFFER_CAPACITY - position;

        if (length > remaining)
            length = remaining;

        if (write_all(STDOUT_FILENO,
                      buffer->data + position, length) == -1)
            return;

        position = (position + length) % BUFFER_CAPACITY;
        remaining -= length;
    }
}

int multiplexer_run(SessionManager *manager)
{
    if (manager == NULL) {
        errno = EINVAL;
        return -1;
    }

    Terminal terminal = {0};

    if (terminal_enable_raw(&terminal) == -1) {
        perror("PtyMux: terminal raw mode");
        return -1;
    }

    if (signal_handler_init() == -1) {
        perror("PtyMux: signal initialization");
        terminal_restore(&terminal);
        return -1;
    }

    resize_sessions(manager);
    show_banner();

    int prefix_pending = 0;
    int previous_active = -1;
    int result = 0;

    while (1) {
        session_reap_children(manager);

        if (signal_resize_requested()) {
            signal_clear_resize();
            resize_sessions(manager);
            repaint_active(manager);
            previous_active = manager->active_index;
        }

        if (signal_child_event())
            signal_clear_child_event();

        if (manager->active_index != previous_active) {
            repaint_active(manager);
            previous_active = manager->active_index;
        }

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int maxfd = STDIN_FILENO;

        for (int i = 0; i < MAX_SESSIONS; i++) {
            Session *session = &manager->sessions[i];
            int fd = session->pty.master_fd;

            if (session->state == SESSION_RUNNING &&
                fd >= 0 && fd < FD_SETSIZE) {
                FD_SET(fd, &readfds);

                if (fd > maxfd)
                    maxfd = fd;
            }
        }

        int ready = select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (ready == -1) {
            if (errno == EINTR)
                continue;

            perror("PtyMux: select");
            result = -1;
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            unsigned char input[IO_BUFFER_SIZE];
            ssize_t count = read(STDIN_FILENO, input, sizeof(input));

            if (count == 0)
                break;

            if (count == -1) {
                if (errno == EINTR)
                    continue;

                perror("PtyMux: terminal read");
                result = -1;
                break;
            }

            for (ssize_t j = 0; j < count; j++) {
                int key = input[j];

                if (!prefix_pending && key == PREFIX_KEY) {
                    prefix_pending = 1;
                    continue;
                }

                if (prefix_pending) {
                    prefix_pending = 0;

                    if (key == PREFIX_KEY) {
                        Session *active = session_active(manager);

                        if (active != NULL && active->pty.master_fd >= 0) {
                            unsigned char prefix = PREFIX_KEY;
                            (void)write_all(active->pty.master_fd,
                                            (const char *)&prefix, 1);
                        }
                        continue;
                    }

                    int old_active = manager->active_index;
                    Command command = commands_parse_key(key);

                    commands_execute(manager, command);

                    if (command.type == COMMAND_QUIT)
                        goto finished;

                    if (command.type == COMMAND_NEW)
                        resize_sessions(manager);

                    if (manager->active_index != old_active) {
                        repaint_active(manager);
                        previous_active = manager->active_index;
                    }

                    continue;
                }

                Session *active = session_active(manager);

                if (active != NULL && active->pty.master_fd >= 0) {
                    if (write_all(active->pty.master_fd,
                                  (const char *)&input[j], 1) == -1 &&
                        errno != EIO && errno != EAGAIN) {
                        perror("PtyMux: PTY write");
                    }
                }
            }
        }

        for (int i = 0; i < MAX_SESSIONS; i++) {
            Session *session = &manager->sessions[i];
            int fd = session->pty.master_fd;

            if (session->state != SESSION_RUNNING ||
                fd < 0 || fd >= FD_SETSIZE ||
                !FD_ISSET(fd, &readfds))
                continue;

            char output[IO_BUFFER_SIZE];
            ssize_t count = read(fd, output, sizeof(output));

            if (count > 0) {
                (void)buffer_write(&session->output,
                                   output, (size_t)count);

                if (i == manager->active_index) {
                    if (write_all(STDOUT_FILENO, output,
                                  (size_t)count) == -1) {
                        result = -1;
                        goto finished;
                    }
                }
            } else if (count == 0 ||
                       (count == -1 &&
                        (errno == EIO || errno == EAGAIN))) {
                if (count == 0 || errno == EIO) {
                    close(fd);
                    session->pty.master_fd = -1;
                }
            } else if (errno != EINTR) {
                perror("PtyMux: PTY read");
                close(fd);
                session->pty.master_fd = -1;
            }
        }
    }

finished:
    signal_handler_cleanup();
    terminal_restore(&terminal);

    return result;
}
