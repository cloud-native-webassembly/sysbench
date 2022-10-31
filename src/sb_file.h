#ifndef SB_FILE_H
#define SB_FILE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

uint8_t *sb_load_file_to_buffer(const char *, uint32_t *);

#endif