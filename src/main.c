
#include "session.h"
#include "multiplexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        fprintf(stderr,
                "PtyMux must run inside an interactive terminal.\n");
        return EXIT_FAILURE;
    }

    SessionManager manager;
    session_manager_init(&manager);

    Session *first = session_create(&manager, "main");

    if (first == NULL) {
        perror("PtyMux: unable to create initial shell");
        session_manager_destroy(&manager);
        return EXIT_FAILURE;
    }

    int result = multiplexer_run(&manager);

    session_manager_destroy(&manager);

    return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
