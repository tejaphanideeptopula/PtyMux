
#include "server.h"

#include <errno.h>

int server_run(const char *socket_path)
{
    (void)socket_path;
    errno = ENOSYS;
    return -1;
}
