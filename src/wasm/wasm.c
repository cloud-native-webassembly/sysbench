#include <stdint.h>

int64_t wasm_addr_encode(int32_t *addr, int32_t size) {
    return ((int64_t)addr << 32) | size;
}

void wasm_addr_decode(int64_t val, void **addr, int32_t *size) {
    *addr = (void *)((val >> 32) & 0xffffffff);
    *size = val & 0xffffffff;
}

int64_t create_buffer(int32_t size) {
    int32_t *addr = malloc(size);
    return wasm_addr_encode(addr, size);
}

int event(int64_t params) {
}

int main() {
    return 0;
}