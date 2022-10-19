#ifndef SB_FILE_H
#define SB_FILE_H

#include <stdint.h>
#include <stdio.h>

uint8_t *sb_load_file_to_buffer(const char *filename);

#endif