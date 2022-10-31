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
static int sb_wasm_op_thread_run(int);
static int sb_wasm_op_thread_done(int);
static void sb_wasm_report_intermediate(sb_stat_t *);
static void sb_wasm_report_cumulative(sb_stat_t *);

static sb_event_t sb_wasm_op_next_event(int thread_id);
static int sb_wasm_op_execute_event(sb_event_t *r, int thread_id);

static sb_operations_t wasm_ops = {
    .init = sb_wasm_op_init,
    .thread_init = sb_wasm_op_thread_init,
    .thread_done = sb_wasm_op_thread_done,
    .report_intermediate = sb_wasm_report_intermediate,
    .report_cumulative = sb_wasm_report_cumulative,
    .done = sb_wasm_op_done,
    .next_event = sb_wasm_op_next_event,
    .execute_event = sb_wasm_op_execute_event

};

/* wasm test commands */
static int sb_wasm_cmd_prepare(void);
static int sb_wasm_cmd_cleanup(void);
static int sb_wasm_cmd_help(void);

static sb_test_t sbtest CK_CC_CACHELINE;
static sb_wasm_sandbox **sandboxs CK_CC_CACHELINE;
static sb_wasm_vm *wasm_vm;
static sb_wasm_module *wasm_module;

sb_test_t *sb_load_wasm(const char *testname, const char *runtime)
{
  log_text(LOG_INFO, "load wasm using runtime: %s", runtime);
  sb_wasm_runtime_t runtime_t = wasm_runtime_name_to_type(runtime);
  switch (runtime_t)
  {
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
  if (testname != NULL)
  {
    char *tmp = strdup(testname);
    sbtest.sname = strdup(basename(tmp));
    sbtest.lname = tmp;
  }
  else
  {
    log_text(LOG_FATAL, "no wasm file name provided");
    goto error;
  }

  sbtest.ops = wasm_ops;
  wasm_vm->init();
  log_text(LOG_INFO, "load wasm module from file %s", sbtest.lname);
  wasm_module = wasm_vm->load_module(sbtest.lname);

  sandboxs = (sb_wasm_sandbox **)calloc(sb_globals.threads, sizeof(sb_wasm_sandbox *));
  if (sandboxs == NULL)
    goto error;

  return &sbtest;

error:
  return NULL;
}

void sb_wasm_done(void)
{
  if (sbtest.args != NULL)
  {
    for (size_t i = 0; sbtest.args[i].name != NULL; i++)
    {
      xfree(sbtest.args[i].name);
      xfree(sbtest.args[i].desc);
      xfree(sbtest.args[i].value);
    }

    xfree(sbtest.args);
  }

  xfree(sbtest.sname);
  xfree(sbtest.lname);
}

static sb_event_t sb_wasm_op_next_event(int thread_id)
{
  sb_event_t req;

  (void)thread_id; /* unused */

  req.type = SB_REQ_TYPE_SCRIPT;

  return req;
}

int sb_wasm_op_execute_event(sb_event_t *r, int thread_id)
{
  sb_wasm_sandbox *sandbox = sandboxs[thread_id];
  if (sandbox->call_function(sandbox->context, EVENT_FUNC, thread_id))
  {
    return 1;
  }
  return 0;
}

bool sb_wasm_custom_command_defined(const char *name) {}

int sb_wasm_report_thread_init(void) {}

void sb_wasm_report_thread_done(void *data) {}

bool sb_wasm_loaded(void)
{
  return true;
}

static int sb_wasm_op_init(void)
{
  return 0;
}
static int sb_wasm_op_done(void)
{
  return 0;
}
static int sb_wasm_op_thread_init(int thread_id)
{
  sb_wasm_sandbox *sandbox = wasm_module->create_sandbox(wasm_module->context, thread_id);
  if (sandbox == NULL)
    return 1;

  sandboxs[thread_id] = sandbox;

  return 0;
}
static int sb_wasm_op_thread_run(int thread_id)
{
  return 0;
}
static int sb_wasm_op_thread_done(int thread_id)
{
  return 0;
}
static void sb_wasm_report_intermediate(sb_stat_t *stat)
{
}
static void sb_wasm_report_cumulative(sb_stat_t *stat)
{
}
