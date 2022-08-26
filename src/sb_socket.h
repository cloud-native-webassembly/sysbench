#ifndef SB_SOCKET_H
#define SB_SOCKET_H
#include <stdio.h>

typedef struct sb_socket_buffer
{
    int fd;
    char *buf;
    size_t buf_len;
} sb_socket_buffer_t;

sb_socket_buffer_t *create_socket_buffer(int sockfd, size_t buffer_size);

void free_socket_buffer(sb_socket_buffer_t *buffer);

int create_unix_socket_pair(int (*socket_pair_fd)[2]);
#endif