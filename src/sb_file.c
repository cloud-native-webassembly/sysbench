#include "sb_file.h"

uint8_t *sb_load_file_to_buffer(const char *filename) {
  FILE *file;
  uint8_t *bytes;
  long len;
  file = fopen(filename, "r");
  fseek(file, 0, SEEK_END);
  len = ftell(file);
  bytes = malloc(len);
  fseek(file, 0, SEEK_SET);
  fread(bytes, 1, len, file);
  fclose(file);
}