
#ifndef PTYMUX_PROTOCOL_H
#define PTYMUX_PROTOCOL_H

#include <stdint.h>

#define PTYMUX_SOCKET_PATH "/tmp/ptymux.sock"
#define PTYMUX_PROTOCOL_VERSION 1
#define PTYMUX_MAX_MESSAGE 4096

typedef enum {
    PROTOCOL_ATTACH = 1,
    PROTOCOL_INPUT,
    PROTOCOL_OUTPUT,
    PROTOCOL_RESIZE,
    PROTOCOL_DETACH,
    PROTOCOL_LIST,
    PROTOCOL_KILL,
    PROTOCOL_ERROR
} ProtocolType;

typedef struct {
    uint32_t version;
    uint32_t type;
    uint32_t length;
} ProtocolHeader;

#endif
