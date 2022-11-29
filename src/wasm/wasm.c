#include <malloc.h>
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

int64_t event(int64_t carrier) {
    void *addr;
    int32_t size;
    wasm_addr_decode(carrier, &addr, &size);
    // we assume it parameter is a int32_t array;
    int32_t *data = (int32_t *)addr;
    int32_t *sum = data + size + 1;
    for (int i = 0; i < size; i++) {
        *sum += data[i];
    }
    return wasm_addr_encode(sum, sizeof(int32_t));
}

int main() {
    return 0;
}