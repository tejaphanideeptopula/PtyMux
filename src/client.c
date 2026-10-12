

#include "client.h"

#include <errno.h>

int client_run(const char *socket_path)
{
    (void)socket_path;
    errno = ENOSYS;
    return -1;
}
