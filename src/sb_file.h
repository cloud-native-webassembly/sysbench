#ifndef SB_FILE_H
#define SB_FILE_H

#include <stdio.h>

typedef struct sb_file_buffer
{
    char *filename;
    int fd;
    char *buf;
    size_t buf_len;
} sb_file_buffer_t;

sb_file_buffer_t *create_file_buffer(const char *filename, size_t buffer_size);

void free_file_buffer(sb_file_buffer_t *buffer);

int reset_file_buffer(sb_file_buffer_t *buffer);
#endif