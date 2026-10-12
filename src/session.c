
#include "session.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

void session_manager_init(SessionManager *manager)
{
    if (manager == NULL)
        return;

    memset(manager, 0, sizeof(*manager));

    manager->active_index = -1;
    manager->next_id = 1;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        manager->sessions[i].pty.master_fd = -1;
        manager->sessions[i].pty.child_pid = -1;
        manager->sessions[i].state = SESSION_UNUSED;
    }
}

Session *session_create(SessionManager *manager, const char *name)
{
    if (manager == NULL) {
        errno = EINVAL;
        return NULL;
    }

    int index = -1;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        if (manager->sessions[i].state != SESSION_RUNNING) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        errno = ENOSPC;
        return NULL;
    }

    Session *session = &manager->sessions[index];

    if (session->pty.master_fd >= 0 ||
        session->pty.child_pid > 0) {
        errno = EBUSY;
        return NULL;
    }

    memset(session, 0, sizeof(*session));

    session->pty.master_fd = -1;
    session->pty.child_pid = -1;

    session->id = manager->next_id++;

    if (name != NULL && name[0] != '\0') {
        snprintf(session->name, sizeof(session->name), "%s", name);
    } else {
        snprintf(session->name, sizeof(session->name),
                 "session-%d", session->id);
    }

    buffer_init(&session->output);

    if (pty_spawn(&session->pty, NULL) == -1) {
        session->state = SESSION_UNUSED;
        return NULL;
    }

    session->state = SESSION_RUNNING;
    session->exit_status = 0;

    if (manager->active_index == -1)
        manager->active_index = index;

    return session;
}

Session *session_get(SessionManager *manager, int id)
{
    if (manager == NULL)
        return NULL;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        Session *session = &manager->sessions[i];

        if (session->state == SESSION_RUNNING &&
            session->id == id)
            return session;
    }

    return NULL;
}

Session *session_active(SessionManager *manager)
{
    if (manager == NULL ||
        manager->active_index < 0 ||
        manager->active_index >= MAX_SESSIONS)
        return NULL;

    Session *session = &manager->sessions[manager->active_index];

    return session->state == SESSION_RUNNING ? session : NULL;
}

int session_switch(SessionManager *manager, int id)
{
    if (manager == NULL) {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < MAX_SESSIONS; i++) {
        Session *session = &manager->sessions[i];

        if (session->state == SESSION_RUNNING &&
            session->id == id) {
            manager->active_index = i;
            return 0;
        }
    }

    errno = ENOENT;
    return -1;
}

int session_kill(SessionManager *manager, int id)
{
    Session *session = session_get(manager, id);

    if (session == NULL) {
        errno = ENOENT;
        return -1;
    }

    pty_close(&session->pty);
    session->state = SESSION_UNUSED;
    buffer_clear(&session->output);

    if (manager->active_index >= 0 &&
        &manager->sessions[manager->active_index] == session) {
        manager->active_index = -1;

        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (manager->sessions[i].state == SESSION_RUNNING) {
                manager->active_index = i;
                break;
            }
        }
    }

    return 0;
}

void session_list(SessionManager *manager)
{
    if (manager == NULL)
        return;

    puts("\nID   NAME                     STATE");

    for (int i = 0; i < MAX_SESSIONS; i++) {
        Session *session = &manager->sessions[i];

        if (session->state == SESSION_RUNNING) {
            printf("%-4d %-24s %s%s\n",
                   session->id,
                   session->name,
                   "RUNNING",
                   manager->active_index == i ? "  <ACTIVE>" : "");
        }
    }
}

void session_reap_children(SessionManager *manager)
{
    if (manager == NULL)
        return;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        Session *session = &manager->sessions[i];

        if (session->state != SESSION_RUNNING ||
            session->pty.child_pid <= 0)
            continue;

        int status;
        pid_t result = waitpid(session->pty.child_pid, &status, WNOHANG);

        if (result == session->pty.child_pid) {
            session->exit_status = status;
            close(session->pty.master_fd);
            session->pty.master_fd = -1;
            session->pty.child_pid = -1;
            session->state = SESSION_DEAD;

            if (manager->active_index == i)
                manager->active_index = -1;
        }
    }

    if (manager->active_index == -1) {
        for (int i = 0; i < MAX_SESSIONS; i++) {
            if (manager->sessions[i].state == SESSION_RUNNING) {
                manager->active_index = i;
                break;
            }
        }
    }
}

void session_manager_destroy(SessionManager *manager)
{
    if (manager == NULL)
        return;

    for (int i = 0; i < MAX_SESSIONS; i++) {
        Session *session = &manager->sessions[i];

        if (session->pty.master_fd >= 0 ||
            session->pty.child_pid > 0)
            pty_close(&session->pty);

        buffer_clear(&session->output);
        session->state = SESSION_UNUSED;
    }

    manager->active_index = -1;
}
