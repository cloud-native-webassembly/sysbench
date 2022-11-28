#include "sb_file.h"

uint8_t *sb_load_file_to_buffer(const char *filename, uint32_t *size) {
    FILE *file;
    uint8_t *bytes;
    file = fopen(filename, "r");
    if (file == NULL) {
        fprintf(stderr, "can not open file (%s) for read.\n", filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    *size = ftell(file);
    bytes = malloc(*size);
    if (bytes == NULL) {
        fprintf(stderr, "can not allocate (%d) memory for read file %s.\n", *size, filename);
        return NULL;
    }

    fseek(file, 0, SEEK_SET);
    fread(bytes, 1, *size, file);
    fclose(file);
    return bytes;
}