#include "sb_wasm.h"

sb_wasm_runtime_t wasm_runtime_name_to_type(const char *runtime) {
  if (!strcmp(runtime, "wamr")) {
    return SB_WASM_RUNTIME_WAMR;
  } else if (!strcmp(runtime, "wasmedge")) {
    return SB_WASM_RUNTIME_WASMEDGE;
  } else if (!strcmp(runtime, "wasmer")) {
    return SB_WASM_RUNTIME_WASMER;
  } else if (!strcmp(runtime, "wasmtime")) {
    return SB_WASM_RUNTIME_WASMTIME;
  } else {
    return SB_WASM_RUNTIME_UNKNOWN;
  }
}

sb_test_t *sb_load_wasm(const char *testname, const char *runtime, int argc, char *argv[]) {
  log_text(LOG_INFO, "load wasm using runtime: %s", runtime);
  sb_wasm_runtime_t runtime_t = wasm_runtime_name_to_type(runtime);
  switch (runtime_t) {
    case SB_WASM_RUNTIME_WAMR:
      return sb_load_wamr(testname, argc, argv);
      break;
    case SB_WASM_RUNTIME_WASMEDGE:
      return sb_load_wasmedge(testname, argc, argv);
      break;
    case SB_WASM_RUNTIME_WASMER:
      return sb_load_wasmer(testname, argc, argv);
      break;
    case SB_WASM_RUNTIME_WASMTIME:
      return sb_load_wasmtime(testname, argc, argv);
      break;
    default:
      log_text(LOG_FATAL, "unsupported wasm runtime: %s", runtime);
      return NULL;
      break;
  }
}

void sb_wasm_done(void) {
}

bool sb_wasm_custom_command_defined(const char *name) {}

int sb_wasm_report_thread_init(void) {}

void sb_wasm_report_thread_done(void *data) {}

bool sb_wasm_loaded(void) {}
