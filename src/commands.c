
#include "commands.h"

#include <stdio.h>
#include <string.h>

Command commands_parse_key(int key)
{
    Command command = { COMMAND_NONE, 0 };

    if (key >= '1' && key <= '9') {
        command.type = COMMAND_SWITCH;
        command.session_id = key - '0';
        return command;
    }

    switch (key) {
        case 'n':
        case 'N':
            command.type = COMMAND_NEW;
            break;

        case 'l':
        case 'L':
            command.type = COMMAND_LIST;
            break;

        case 'k':
        case 'K':
            command.type = COMMAND_KILL;
            break;

        case '?':
            command.type = COMMAND_HELP;
            break;

        case 'q':
        case 'Q':
            command.type = COMMAND_QUIT;
            break;

        default:
            command.type = COMMAND_NONE;
            break;
    }

    return command;
}

void commands_show_help(void)
{
    printf("\r\nPtyMux Commands\r\n");
    printf("------------------------------\r\n");
    printf("Ctrl+B, n     Create a new shell\r\n");
    printf("Ctrl+B, 1-9   Switch to a session\r\n");
    printf("Ctrl+B, l     List sessions\r\n");
    printf("Ctrl+B, k     Kill the active session\r\n");
    printf("Ctrl+B, ?     Show help\r\n");
    printf("Ctrl+B, q     Quit PtyMux\r\n");
    printf("------------------------------\r\n");
    fflush(stdout);
}

int commands_execute(SessionManager *manager, Command command)
{
    if (manager == NULL) {
        fprintf(stderr, "PtyMux: invalid session manager\r\n");
        return 0;
    }

    switch (command.type) {
        case COMMAND_NEW: {
            Session *session = session_create(manager, NULL);

            if (session == NULL) {
                fprintf(stderr, "PtyMux: failed to create a new shell\r\n");
                return 0;
            }

            manager->active_index =
                (int)(session - manager->sessions);

            printf("\r\nCreated session %d (%s)\r\n",
                   session->id, session->name);
            fflush(stdout);
            break;
        }

        case COMMAND_SWITCH:
            if (session_switch(manager, command.session_id) == 0) {
                printf("\r\nSwitched to session %d\r\n",
                       command.session_id);
                fflush(stdout);
            } else {
                printf("\r\nSession %d does not exist or is not running.\r\n",
                       command.session_id);
                fflush(stdout);
            }
            break;

        case COMMAND_LIST:
            printf("\r\n");
            session_list(manager);
            fflush(stdout);
            break;

        case COMMAND_KILL: {
            Session *active = session_active(manager);

            if (active == NULL) {
                printf("\r\nNo active session to kill.\r\n");
                fflush(stdout);
                break;
            }

            int id = active->id;

            if (session_kill(manager, id) == 0) {
                printf("\r\nKilled session %d\r\n", id);
            } else {
                printf("\r\nFailed to kill session %d\r\n", id);
            }

            fflush(stdout);
            break;
        }

        case COMMAND_HELP:
            commands_show_help();
            break;

        case COMMAND_QUIT:
            return 1;

        case COMMAND_NONE:
        default:
            break;
    }

    return 0;
}
