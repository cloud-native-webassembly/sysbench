#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "sb_socket.h"

sb_socket_buffer_t *create_socket_buffer(int sockfd, size_t buffer_size)
{
    sb_socket_buffer_t *buffer = malloc(sizeof(sb_socket_buffer_t));
    buffer->fd = sockfd;
    buffer->buf_len = buffer_size;
    buffer->buf = malloc(buffer_size * sizeof(char));
    return buffer;
}

void free_socket_buffer(sb_socket_buffer_t *buffer)
{
    if (buffer != NULL)
    {
        if (buffer->fd > 0)
            close(buffer->fd);
        buffer->fd = -1;

        if (buffer->buf != NULL)
            free(buffer->buf);
        buffer->buf = NULL;
    }
}

int create_unix_socket_pair(int (*socket_pair_fd)[2])
{
    return socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair_fd);
}