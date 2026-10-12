
#ifndef PTYMUX_SIGNAL_HANDLER_H
#define PTYMUX_SIGNAL_HANDLER_H

#include <signal.h>

int signal_handler_init(void);
void signal_handler_cleanup(void);
int signal_resize_requested(void);
void signal_clear_resize(void);
int signal_child_event(void);
void signal_clear_child_event(void);

#endif
