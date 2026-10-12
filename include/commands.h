
#ifndef COMMANDS_H
#define COMMANDS_H

#include "session.h"

typedef enum {
    COMMAND_NONE,
    COMMAND_NEW,
    COMMAND_SWITCH,
    COMMAND_LIST,
    COMMAND_KILL,
    COMMAND_HELP,
    COMMAND_QUIT
} CommandType;

typedef struct {
    CommandType type;
    int session_id;
} Command;

Command commands_parse_key(int key);
int commands_execute(SessionManager *manager, Command command);
void commands_show_help(void);

#endif
