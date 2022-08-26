#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "sb_file.h"

sb_file_buffer_t *create_file_buffer(const char *filename, size_t buffer_size)
{
    sb_file_buffer_t *buffer = malloc(sizeof(sb_file_buffer_t));
    buffer->fd = open(filename, O_RDONLY);
    if (buffer->fd > 0)
    {
        buffer->filename = strdup(filename);
        buffer->buf_len = buffer_size;
        buffer->buf = malloc(buffer_size * sizeof(char));
        return buffer;
    }
    else
    {
        free(buffer);
        return NULL;
    }
}

void free_file_buffer(sb_file_buffer_t *buffer)
{
    if (buffer != NULL)
    {
        if (buffer->fd > 0)
            close(buffer->fd);
        buffer->fd = -1;
        if (buffer->filename != NULL)
            free(buffer->filename);
        buffer->filename = NULL;
        if (buffer->buf != NULL)
            free(buffer->buf);
        buffer->buf = NULL;
    }
}

int reset_file_buffer(sb_file_buffer_t *buffer)
{
    if (buffer != NULL)
    {
        if (buffer->fd > 0)
            close(buffer->fd);
        buffer->fd = -1;

        if (buffer->buf != NULL)
            free(buffer->buf);
        buffer->buf = NULL;

        buffer->fd = open(buffer->filename, O_RDONLY);
        if (buffer->fd > 0)
        {
            buffer->buf = malloc(buffer->buf_len * sizeof(char));
            if (buffer->buf != NULL)
            {
                return 0;
            }
        }
    }
    return -1;
}
