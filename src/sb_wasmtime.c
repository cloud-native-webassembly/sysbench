/* Copyright (C) 2006 MySQL AB
   Copyright (C) 2006-2018 Alexey Kopytov <akopytov@gmail.com>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <wasm.h>

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "sb_wasmtime.h"
#include "db_driver.h"
#include "sb_rand.h"
#include "sb_thread.h"
#include "sb_ck_pr.h"

#define SB_LUA_EXPORT
#include "sb_counter.h"
#undef SB_LUA_EXPORT

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

#define xfree(ptr) ({ if ((ptr) != NULL) free((void *) ptr); ptr = NULL; })

/* Interpreter context */
typedef struct
{
  db_conn_t *con; /* Database connection */
  db_driver_t *driver;
  wasmtime_context_t *context;
} sb_wasmedge_ctxt_t;

typedef struct
{
  int id;
  db_bind_type_t type;
  void *buf;
  unsigned long buflen;
  char is_null;
} sb_wasmedge_bind_t;

typedef struct
{
  const char *name;
  const unsigned char *source;
  /* Use a pointer, since _len variables are not compile-time constants */
  size_t *source_len;
} internal_script_t;

typedef enum
{
  SB_WASMEDGE_ERROR_NONE,
  SB_WASMEDGE_ERROR_RESTART_EVENT
} sb_wasmedge_error_t;

bool sb_wasmedge_more_events(int);
int sb_wasmedge_set_test_args(sb_arg_t *, size_t);

/* Python Modules */

static wasmtime_context_t **contexts CK_CC_CACHELINE;

static sb_test_t sbtest CK_CC_CACHELINE;
static TLS sb_wasmedge_ctxt_t tls_wasmedge_ctxt CK_CC_CACHELINE;

/* List of pre-loaded internal scripts */
static internal_script_t internal_scripts[] = {
    {NULL, NULL, 0}};

/* Lua test operations */

static int sb_wasmedge_op_init(void);
static int sb_wasmedge_op_done(void);
static int sb_wasmedge_op_thread_init(int);
static int sb_wasmedge_op_thread_run(int);
static int sb_wasmedge_op_thread_done(int);
static sb_event_t sb_wasmedge_op_next_event(int);
static int sb_wasmedge_op_execute_event(sb_event_t *event, int);

static sb_operations_t wasmedge_ops = {
    .init = sb_wasmedge_op_init,
    .thread_init = sb_wasmedge_op_thread_init,
    .thread_done = sb_wasmedge_op_thread_done,
    .next_event = sb_wasmedge_op_next_event,
    .execute_event = sb_wasmedge_op_execute_event,
    .report_intermediate = db_report_intermediate,
    .report_cumulative = db_report_cumulative,
    .done = sb_wasmedge_op_done};

/* Lua test commands */
static int sb_wasmedge_cmd_prepare(void);
static int sb_wasmedge_cmd_cleanup(void);
static int sb_wasmedge_cmd_help(void);

/* Initialize interpreter state */
static wasmtime_context_t *sb_wasmedge_new_module(void);

/* Close interpretet state */
static int sb_wasmedge_free_module(wasmtime_context_t *);

static void call_error(wasmtime_context_t *context, const char *name)
{
  log_text(LOG_FATAL, "[%s] function failed in module", name);
}

static bool func_available(wasmtime_context_t *context, const char *func)
{
  // TODO check function
  return false;
}

static int wasmedge_call_function(wasmtime_context_t *context, const char *fname, int thread_id)
{
  WasmEdge_Value params[1] = {WasmEdge_ValueGenI32(20)};
  WasmEdge_Value returns[1];
  WasmEdge_String func_name = WasmEdge_StringCreateByCString(fname);
  WasmEdge_Result Res;
  Res = WasmEdge_VMExecute(context, func_name, params, 1, returns, 1);
  WasmEdge_StringDelete(func_name);
  if (!WasmEdge_ResultOK(Res))
  {
    fprintf(stderr, "call function [%s] failed: %s\n", fname, WasmEdge_ResultGetMessage(Res));
    return 1;
  }
  return 0;
}

static int do_export_options(wasmtime_context_t *context, bool global)
{
  return 0;
}

static int export_options(wasmtime_context_t *context)
{
  if (do_export_options(context, false))
    return 1;

  return 0;
}

/* Load a specified Lua script */

sb_test_t *sb_load_wasmtime(const char *testname, int argc, char *argv[])
{
  if (testname != NULL)
  {
    char *tmp = strdup(testname);
    sbtest.sname = strdup(basename(tmp));
    sbtest.lname = tmp;
  }
  else
  {
    log_text(LOG_FATAL, "no wasm name provided");
    goto error;
  }

  wasm_engine_t *engine = wasm_engine_new();
  wasmtime_store_t *store = wasmtime_store_new(engine, NULL, NULL);
  wasmtime_context_t *context = wasmtime_store_context(store);

  FILE *file = fopen(sbtest.lname, "r");
  if (!file)
  {
    log_text(LOG_FATAL, "load file from %s failed", sbtest.lname);
    goto error;
  }

  wasmtime_module_delete(module);
  wasmtime_store_delete(store);
  wasm_engine_delete(engine);
  /* Test operations */
  sbtest.ops = wasmedge_ops;

  if (sb_globals.threads != 1)
  {
    log_text(LOG_FATAL, "wasmedge script %s only support a single thread", sbtest.sname);
    goto error;
  }

  /* Allocate per-thread interpreters array */
  contexts = (wasmtime_context_t **)calloc(sb_globals.threads, sizeof(wasmtime_context_t *));
  if (contexts == NULL)
    goto error;

  return &sbtest;
error:

  sb_wasmtime_done();

  return NULL;
}

void sb_wasmtime_done(void)
{
  xfree(contexts);

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

int sb_wasmedge_op_init(void)
{
  return 0;
}

int sb_wasmedge_op_thread_init(int thread_id)
{
  wasmtime_context_t *context = sb_wasmedge_new_module();
  if (context == NULL)
    return 1;

  contexts[thread_id] = context;

  if (export_options(context))
    return 1;

  if (func_available(context, THREAD_INIT_FUNC))
  {
    if (wasmedge_call_function(context, THREAD_INIT_FUNC, thread_id))
    {
      call_error(context, THREAD_INIT_FUNC);
      return 1;
    }
  }

  return 0;
}

int sb_wasmedge_op_thread_run(int thread_id)
{
  wasmtime_context_t *const context = contexts[thread_id];

  if (func_available(context, THREAD_RUN_FUNC))
  {
    if (wasmedge_call_function(context, THREAD_RUN_FUNC, thread_id))
    {
      call_error(context, THREAD_RUN_FUNC);
      return 1;
    }
  }

  return 0;
}

int sb_wasmedge_op_thread_done(int thread_id)
{
  wasmtime_context_t *const context = contexts[thread_id];
  if (func_available(context, THREAD_RUN_FUNC))
  {
    if (wasmedge_call_function(context, THREAD_RUN_FUNC, thread_id))
    {
      call_error(context, THREAD_RUN_FUNC);
      return 1;
    }
  }

  sb_wasmedge_free_module(context);
  return 0;
}

int sb_wasmedge_op_done(void)
{
  sb_wasmtime_done();

  return 0;
}

inline sb_event_t sb_wasmedge_op_next_event(int thread_id)
{
  sb_event_t req;

  (void)thread_id; /* unused */

  req.type = SB_REQ_TYPE_SCRIPT;

  return req;
}

int sb_wasmedge_op_execute_event(sb_event_t *r, int thread_id)
{
  wasmtime_context_t *const context = contexts[thread_id];
  if (wasmedge_call_function(context, EVENT_FUNC, thread_id))
  {
    call_error(context, EVENT_FUNC);
    return 1;
  }
  return 0;
}

int sb_wasmedge_set_test_args(sb_arg_t *args, size_t len)
{
  sbtest.args = malloc((len + 1) * sizeof(sb_arg_t));

  for (size_t i = 0; i < len; i++)
  {
    sbtest.args[i].name = strdup(args[i].name);
    sbtest.args[i].desc = strdup(args[i].desc);
    sbtest.args[i].type = args[i].type;

    sbtest.args[i].value = args[i].value != NULL ? strdup(args[i].value) : NULL;
    sbtest.args[i].validate = args[i].validate;
  }

  sbtest.args[len] = (sb_arg_t){.name = NULL};

  return 0;
}

static wasmtime_context_t *sb_wasmedge_new_module()
{
  WasmEdge_Result Res;
  const char *name = sbtest.lname;
  WasmEdge_ConfigureContext *config_cxt = WasmEdge_ConfigureCreate();
  WasmEdge_StoreContext *store_cxt = WasmEdge_StoreCreate();
  wasmtime_context_t *context = WasmEdge_VMCreate(config_cxt, store_cxt);
  if (context == NULL)
  {
    log_text(LOG_FATAL, "can not import wasmedge module: %s", name);
    goto error;
  }
  Res = WasmEdge_VMLoadWasmFromFile(context, name);
  if (!WasmEdge_ResultOK(Res))
  {
    log_text(LOG_FATAL, "load wasm from file failed: %s", name);
    goto error;
  }
  Res = WasmEdge_VMValidate(context);
  if (!WasmEdge_ResultOK(Res))
  {
    printf("validation wasm module failed: %s\n", WasmEdge_ResultGetMessage(Res));
    goto error;
  }
  Res = WasmEdge_VMInstantiate(context);
  if (!WasmEdge_ResultOK(Res))
  {
    printf("instantiation wasm module failed: %s\n", WasmEdge_ResultGetMessage(Res));
    goto error;
  }

  return context;
error:
  WasmEdge_VMDelete(context);
  WasmEdge_StoreDelete(store_cxt);
  WasmEdge_ConfigureDelete(config_cxt);
  return NULL;
}

/* Close interpreter state */

int sb_wasmedge_free_module(wasmtime_context_t *context)
{
  return 0;
}

/* Execute a given command */
static int execute_command(const char *cmd)
{
  return 0;
}

/* Prepare command */

int sb_wasmedge_cmd_prepare(void)
{
  return execute_command(PREPARE_FUNC);
}

/* Cleanup command */

int sb_wasmedge_cmd_cleanup(void)
{
  return execute_command(CLEANUP_FUNC);
}

/* Help command */

int sb_wasmedge_cmd_help(void)
{
  return execute_command(HELP_FUNC);
}

/* Check if a specified hook exists */

bool sb_wasmtime_loaded(void)
{
  return true;
}

static void *cmd_worker_thread(void *arg)
{
  sb_thread_ctxt_t *ctxt = (sb_thread_ctxt_t *)arg;

  sb_tls_thread_id = ctxt->id;

  /* Initialize thread-local RNG state */
  sb_rand_thread_init();

  wasmtime_context_t *const context = sb_wasmedge_new_module();

  if (context == NULL)
  {
    log_text(LOG_FATAL, "failed to create a thread to execute command");
    return NULL;
  }

  sb_wasmedge_free_module(context);

  return NULL;
}

int sb_wasmtime_report_thread_init(void)
{
  if (tls_wasmedge_ctxt.context == NULL)
  {
    sb_wasmedge_new_module();
    export_options(tls_wasmedge_ctxt.context);
  }

  return 0;
}

void sb_wasmtime_report_thread_done(void *arg)
{
  (void)arg; /* unused */

  if (sb_wasmtime_loaded())
    sb_wasmedge_free_module(tls_wasmedge_ctxt.context);
}
