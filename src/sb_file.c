#include "sb_file.h"

uint8_t *sb_load_file_to_buffer(const char *filename, uint32_t *size) {
  FILE *file;
  uint8_t *bytes;
  file = fopen(filename, "r");
  fseek(file, 0, SEEK_END);
  *size = ftell(file);
  bytes = malloc(*size);
  fseek(file, 0, SEEK_SET);
  fread(bytes, 1, *size, file);
  fclose(file);
}