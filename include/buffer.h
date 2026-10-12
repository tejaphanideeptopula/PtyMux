
#ifndef PTYMUX_BUFFER_H
#define PTYMUX_BUFFER_H

#include <stddef.h>

#define BUFFER_CAPACITY 65536

typedef struct {
    char data[BUFFER_CAPACITY];
    size_t head;
    size_t size;
} RingBuffer;

void buffer_init(RingBuffer *buffer);
size_t buffer_write(RingBuffer *buffer, const char *data, size_t length);
size_t buffer_read(RingBuffer *buffer, char *output, size_t length);
size_t buffer_size(const RingBuffer *buffer);
void buffer_clear(RingBuffer *buffer);

#endif
