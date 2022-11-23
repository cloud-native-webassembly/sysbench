#include "sb_wasm.h"

#ifdef HAVE_WAMR
#include "sb_wamr.h"
#endif

#ifdef HAVE_WASMEDGE
#include "sb_wasmedge.h"
#endif

#ifdef HAVE_WASMER
#include "sb_wasmer.h"
#endif

#ifdef HAVE_WASMTIME
#include "sb_wasmtime.h"
#endif

#define EVENT_FUNC "event"
#define PREPARE_FUNC "prepare"
#define CLEANUP_FUNC "cleanup"
#define HELP_FUNC "help"
#define THREAD_INIT_FUNC "thread_init"
#define THREAD_DONE_FUNC "thread_done"
#define THREAD_RUN_FUNC "thread_run"
#define INIT_FUNC "init"
#define DONE_FUNC "done"
#define REPORT_INTERMEDIATE_HOOK "report_intermediate"
#define REPORT_CUMULATIVE_HOOK "report_cumulative"

static int sb_wasm_op_init(void);
static int sb_wasm_op_done(void);
static int sb_wasm_op_thread_init(int);

static sb_event_t sb_wasm_op_next_event(int thread_id);
static int sb_wasm_op_execute_event(sb_event_t *r, int thread_id);

static sb_operations_t wasm_ops = {
    .init = sb_wasm_op_init,
    .thread_init = sb_wasm_op_thread_init,
    .done = sb_wasm_op_done,
    .next_event = sb_wasm_op_next_event,
    .execute_event = sb_wasm_op_execute_event};

static sb_test_t sbtest CK_CC_CACHELINE;
static sb_wasm_sandbox **sandboxs CK_CC_CACHELINE;
static sb_wasm_runtime *wasm_vm;
static sb_wasm_module *wasm_module;

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

sb_test_t *sb_load_wasm(const char *testname, const char *runtime) {
    log_text(LOG_DEBUG, "load wasm using runtime: %s", runtime);
    sb_wasm_runtime_t runtime_t = wasm_runtime_name_to_type(runtime);
    switch (runtime_t) {
#ifdef HAVE_WAMR
        case SB_WASM_RUNTIME_WAMR:
            wasm_vm = create_wamr_vm();
            break;
#endif
#ifdef HAVE_WASMEDGE
        case SB_WASM_RUNTIME_WASMEDGE:
            wasm_vm = create_wasmedge_vm();
            break;
#endif
#ifdef HAVE_WASMER
        case SB_WASM_RUNTIME_WASMER:
            wasm_vm = create_wasmer_vm();
            break;
#endif
#ifdef HAVE_WASMTIME
        case SB_WASM_RUNTIME_WASMTIME:
            wasm_vm = create_wasmtime_vm();
            break;
#endif
        default:
            log_text(LOG_FATAL, "unsupported wasm runtime: %s", runtime);
            goto error;
            break;
    }
    if (testname != NULL) {
        char *tmp = strdup(testname);
        sbtest.sname = strdup(basename(tmp));
        sbtest.lname = tmp;
    } else {
        log_text(LOG_FATAL, "no wasm file name provided");
        goto error;
    }

    sbtest.ops = wasm_ops;
    return &sbtest;

error:
    return NULL;
}

void sb_wasm_done(void) {
    if (sbtest.args != NULL) {
        for (size_t i = 0; sbtest.args[i].name != NULL; i++) {
            xfree(sbtest.args[i].name);
            xfree(sbtest.args[i].desc);
            xfree(sbtest.args[i].value);
        }

        xfree(sbtest.args);
    }

    xfree(sbtest.sname);
    xfree(sbtest.lname);
}

static sb_event_t sb_wasm_op_next_event(int thread_id) {
    sb_event_t req;

    (void)thread_id; /* unused */

    req.type = SB_REQ_TYPE_SCRIPT;

    return req;
}

int sb_wasm_op_execute_event(sb_event_t *r, int thread_id) {
    if (r != NULL) {
        sb_wasm_sandbox *sandbox = sandboxs[thread_id];
        return sandbox->function_apply(sandbox->context, EVENT_FUNC, thread_id);
    } else {
        return FAILURE;
    }
}

bool sb_wasm_loaded(void) {
    return true;
}

static sb_wasm_module *sb_wasm_load_module(const char *filepath) {
    uint32_t wasm_buffer_size = 0;
    uint8_t *wasm_buffer = sb_load_file_to_buffer(filepath, &wasm_buffer_size);
    if (wasm_buffer == NULL) {
        log_text(LOG_FATAL, "load wasm module file[%s] into buffer failed", filepath);
        goto error;
    }

    log_text(LOG_INFO, "load %d bytes from %s", wasm_buffer_size, filepath);

    sb_wasm_module *wasm_module = malloc(sizeof(sb_wasm_module));
    wasm_module->wasm_buffer = wasm_buffer;
    wasm_module->wasm_buffer_size = wasm_buffer_size;
    return wasm_module;
error:
    return NULL;
}

static int sb_wasm_op_init(void) {
    if (!wasm_vm->init()) {
        log_text(LOG_FATAL, "init wasm vm failed");
        goto error;
    }
    log_text(LOG_INFO, "load wasm module from file %s", sbtest.lname);
    if (wasm_vm->load_module != NULL) {
        // use runtime customized load module
        wasm_module = wasm_vm->load_module(sbtest.lname);
    } else {
        wasm_module = sb_wasm_load_module(sbtest.lname);
    }
    if (wasm_module == NULL) {
        log_text(LOG_FATAL, "load wasm module failed");
        goto error;
    }

    sandboxs = (sb_wasm_sandbox **)calloc(sb_globals.threads, sizeof(sb_wasm_sandbox *));
    if (sandboxs == NULL)
        goto error;
    return SUCCESS;
error:
    return FAILURE;
}
static int sb_wasm_op_done(void) {
    return SUCCESS;
}
static int sb_wasm_op_thread_init(int thread_id) {
    sb_wasm_sandbox *sandbox = wasm_vm->create_sandbox(wasm_module, thread_id);
    if (sandbox == NULL) {
        log_text(LOG_FATAL, "create wasm sandbox for thread %d failed", thread_id);
        return FAILURE;
    }
    sandboxs[thread_id] = sandbox;

    return SUCCESS;
}
