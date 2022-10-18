#include "sb_wasm.h"
#include "sb_wasm.h"
#include "sb_wasm.h"
#include "sb_wasm.h"

sb_test_t *sb_load_wasm(const char *testname, const char *runtime, int argc, char *argv[]);

void sb_wasm_done(void);

bool sb_wasm_custom_command_defined(const char *name);

int sb_wasm_report_thread_init(void);

void sb_wasm_report_thread_done(void *);

bool sb_wasm_loaded(void);

