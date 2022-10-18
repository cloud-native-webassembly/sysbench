#include "sb_wasm.h"

sb_test_t *sb_load_wasm(const char *testname, const char *runtime, int argc, char *argv[]) {
    log_text(LOG_INFO, "load wasm using runtime: %s", runtime);
    if (!strcmp(runtime, "wamr")) {
        return sb_load_wamr(testname, argc, argv);
    } else if (!strcmp(runtime, "wasmedge")) {
        return sb_load_wasmedge(testname, argc, argv);
    } else if (!strcmp(runtime, "wasmer")) {
        return sb_load_wasmer(testname, argc, argv);
    } else if (!strcmp(runtime, "wasmtime")) {
        return sb_load_wasmtime(testname, argc, argv);
    } else {
        log_text(LOG_FATAL, "unsupported wasm runtime: %s", runtime);
        return NULL;
    }
}

void sb_wasm_done(void) {}

bool sb_wasm_custom_command_defined(const char *name) {}

int sb_wasm_report_thread_init(void) {}

void sb_wasm_report_thread_done(void *) {}

bool sb_wasm_loaded(void) {}
