
#ifndef PTYMUX_SESSION_H
#define PTYMUX_SESSION_H

#include "pty.h"
#include "buffer.h"

#include <sys/types.h>

#define MAX_SESSIONS 16
#define SESSION_NAME_SIZE 64

typedef enum {
    SESSION_UNUSED = 0,
    SESSION_RUNNING,
    SESSION_DEAD
} SessionState;

typedef struct {
    int id;
    char name[SESSION_NAME_SIZE];
    Pty pty;
    RingBuffer output;
    SessionState state;
    int exit_status;
} Session;

typedef struct {
    Session sessions[MAX_SESSIONS];
    int active_index;
    int next_id;
} SessionManager;

void session_manager_init(SessionManager *manager);
Session *session_create(SessionManager *manager, const char *name);
Session *session_get(SessionManager *manager, int id);
Session *session_active(SessionManager *manager);
int session_switch(SessionManager *manager, int id);
int session_kill(SessionManager *manager, int id);
void session_list(SessionManager *manager);
void session_reap_children(SessionManager *manager);
void session_manager_destroy(SessionManager *manager);

#endif
