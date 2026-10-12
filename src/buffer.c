
#include "buffer.h"

#include <string.h>

void buffer_init(RingBuffer *buffer)
{
    if (buffer == NULL)
        return;

    buffer->head = 0;
    buffer->size = 0;
}

size_t buffer_write(RingBuffer *buffer, const char *data, size_t length)
{
    if (buffer == NULL || data == NULL || length == 0)
        return 0;

    size_t available = BUFFER_CAPACITY - buffer->size;

    if (length > available)
        length = available;

    size_t tail = (buffer->head + buffer->size) % BUFFER_CAPACITY;
    size_t first = BUFFER_CAPACITY - tail;

    if (first > length)
        first = length;

    memcpy(buffer->data + tail, data, first);

    if (length > first)
        memcpy(buffer->data, data + first, length - first);

    buffer->size += length;

    return length;
}

size_t buffer_read(RingBuffer *buffer, char *output, size_t length)
{
    if (buffer == NULL || output == NULL || length == 0)
        return 0;

    if (length > buffer->size)
        length = buffer->size;

    size_t first = BUFFER_CAPACITY - buffer->head;

    if (first > length)
        first = length;

    memcpy(output, buffer->data + buffer->head, first);

    if (length > first)
        memcpy(output + first, buffer->data, length - first);

    buffer->head = (buffer->head + length) % BUFFER_CAPACITY;
    buffer->size -= length;

    return length;
}

size_t buffer_size(const RingBuffer *buffer)
{
    return buffer == NULL ? 0 : buffer->size;
}

void buffer_clear(RingBuffer *buffer)
{
    if (buffer == NULL)
        return;

    buffer->head = 0;
    buffer->size = 0;
}
