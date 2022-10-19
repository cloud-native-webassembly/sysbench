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

#include <stdint.h>
#include <stdio.h>

#include "sb_wamr.h"
#include "wasm_c_api.h"
#include "wasm_export.h"

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "db_driver.h"
#include "sb_ck_pr.h"
#include "sb_rand.h"
#include "sb_thread.h"
#include "sb_file.h"

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

/* Interpreter instance */
typedef struct
{
  db_conn_t *con; /* Database connection */
  db_driver_t *driver;
  wasm_module_inst_t *instance;
} sb_wamr_ctxt_t;

typedef struct
{
  int id;
  db_bind_type_t type;
  void *buf;
  unsigned long buflen;
  char is_null;
} sb_wamr_bind_t;

typedef enum {
  SB_wamr_ERROR_NONE,
  SB_wamr_ERROR_RESTART_EVENT
} sb_wamr_error_t;

bool sb_wamr_more_events(int);
int sb_wamr_set_test_args(sb_arg_t *, size_t);

static wasm_module_inst_t **instances CK_CC_CACHELINE;

static sb_test_t sbtest CK_CC_CACHELINE;
static TLS sb_wamr_ctxt_t tls_wamr_ctxt CK_CC_CACHELINE;


static WasmEdge_ConfigureContext *config_cxt;
static wamr_instance_t *vm_context;

/* Lua test operations */

static int sb_wamr_op_init(void);
static int sb_wamr_op_done(void);
static int sb_wamr_op_thread_init(int);
static int sb_wamr_op_thread_run(int);
static int sb_wamr_op_thread_done(int);
static sb_event_t sb_wamr_op_next_event(int);
static int sb_wamr_op_execute_event(sb_event_t *event, int);

static sb_operations_t wamr_ops = {
    .init = sb_wamr_op_init,
    .thread_init = sb_wamr_op_thread_init,
    .thread_done = sb_wamr_op_thread_done,
    .next_event = sb_wamr_op_next_event,
    .execute_event = sb_wamr_op_execute_event,
    .report_intermediate = db_report_intermediate,
    .report_cumulative = db_report_cumulative,
    .done = sb_wamr_op_done};

/* Lua test commands */
static int sb_wamr_cmd_prepare(void);
static int sb_wamr_cmd_cleanup(void);
static int sb_wamr_cmd_help(void);

/* Initialize interpreter state */
static wamr_instance_t *sb_wamr_new_module(void);

/* Close interpretet state */
static int sb_wamr_free_module(wamr_instance_t *);

static void call_error(wamr_instance_t *instance, const char *name) {
  log_text(LOG_FATAL, "[%s] function failed in module", name);
}

static bool func_available(wamr_instance_t *instance, const char *func) {
  // TODO check function
  return false;
}

static int wamr_call_function(wamr_instance_t *instance, const char *fname, int thread_id) {
  wamr_value_t arg1;
  arg1.tag = WASM_I32;
  arg1.value.I32 = 20;
  wamr_value_t args[] = {arg1};
  wamr_value_t res1;
  wamr_value_t results[] = {res1};
  wamr_result_t call_result = wamr_instance_call(instance, fname, args, 1, results, 1);
  if (call_result == wamr_OK) {
    return 0;
  } else {
    return 1;
  }
}

static int do_export_options(wamr_instance_t *instance, bool global) {
  return 0;
}

static int export_options(wamr_instance_t *instance) {
  if (do_export_options(instance, false))
    return 1;

  return 0;
}


long wasm_len;

sb_test_t *sb_load_wamr(const char *testname, int argc, char *argv[]) {
  if (testname != NULL) {
    char *tmp = strdup(testname);
    sbtest.sname = strdup(basename(tmp));
    sbtest.lname = tmp;
  } else {
    log_text(LOG_FATAL, "no wasm name provided");
    goto error;
  }



  /* Test commands */
  if (func_available(vm_context, PREPARE_FUNC))
    sbtest.builtin_cmds.prepare = &sb_wamr_cmd_prepare;

  if (func_available(vm_context, CLEANUP_FUNC))
    sbtest.builtin_cmds.cleanup = &sb_wamr_cmd_cleanup;

  if (func_available(vm_context, HELP_FUNC))
    sbtest.builtin_cmds.help = &sb_wamr_cmd_help;

  /* Test operations */
  sbtest.ops = wamr_ops;

  if (func_available(vm_context, THREAD_RUN_FUNC))
    sbtest.ops.thread_run = &sb_wamr_op_thread_run;

  if (sb_globals.threads != 1) {
    log_text(LOG_FATAL, "wamr script %s only support a single thread", sbtest.sname);
    goto error;
  }

  /* Allocate per-thread interpreters array */
  instances = (wamr_instance_t **)calloc(sb_globals.threads, sizeof(wamr_instance_t *));
  if (instances == NULL)
    goto error;

  return &sbtest;
error:

  sb_wamr_done();

  return NULL;
}

void sb_wamr_done(void) {
  xfree(instances);

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

int sb_wamr_op_init(void) {
  if (export_options(vm_context))
    return 1;

  if (func_available(vm_context, INIT_FUNC)) {
    if (wamr_call_function(vm_context, INIT_FUNC, -1)) {
      call_error(vm_context, INIT_FUNC);
      return 1;
    }
  }

  return 0;
}

int sb_wamr_op_thread_init(int thread_id) {
  wamr_instance_t *instance = sb_wamr_new_module();
  if (instance == NULL)
    return 1;

  instances[thread_id] = instance;

  if (export_options(instance))
    return 1;

  if (func_available(instance, THREAD_INIT_FUNC)) {
    if (wamr_call_function(instance, THREAD_INIT_FUNC, thread_id)) {
      call_error(instance, THREAD_INIT_FUNC);
      return 1;
    }
  }

  return 0;
}

int sb_wamr_op_thread_run(int thread_id) {
  wamr_instance_t *const instance = instances[thread_id];

  if (func_available(instance, THREAD_RUN_FUNC)) {
    if (wamr_call_function(instance, THREAD_RUN_FUNC, thread_id)) {
      call_error(instance, THREAD_RUN_FUNC);
      return 1;
    }
  }

  return 0;
}

int sb_wamr_op_thread_done(int thread_id) {
  wamr_instance_t *const instance = instances[thread_id];
  if (func_available(instance, THREAD_RUN_FUNC)) {
    if (wamr_call_function(instance, THREAD_RUN_FUNC, thread_id)) {
      call_error(instance, THREAD_RUN_FUNC);
      return 1;
    }
  }

  sb_wamr_free_module(instance);
  return 0;
}

int sb_wamr_op_done(void) {
  if (func_available(vm_context, DONE_FUNC)) {
    if (wamr_call_function(vm_context, DONE_FUNC, -1)) {
      call_error(vm_context, DONE_FUNC);
      return 1;
    }
  }
  sb_wamr_done();

  return 0;
}

inline sb_event_t sb_wamr_op_next_event(int thread_id) {
  sb_event_t req;

  (void)thread_id; /* unused */

  req.type = SB_REQ_TYPE_SCRIPT;

  return req;
}

int sb_wamr_op_execute_event(sb_event_t *r, int thread_id) {
  wamr_instance_t *const instance = instances[thread_id];
  if (wamr_call_function(instance, EVENT_FUNC, thread_id)) {
    call_error(instance, EVENT_FUNC);
    return 1;
  }
  return 0;
}

int sb_wamr_set_test_args(sb_arg_t *args, size_t len) {
  sbtest.args = malloc((len + 1) * sizeof(sb_arg_t));

  for (size_t i = 0; i < len; i++) {
    sbtest.args[i].name = strdup(args[i].name);
    sbtest.args[i].desc = strdup(args[i].desc);
    sbtest.args[i].type = args[i].type;

    sbtest.args[i].value = args[i].value != NULL ? strdup(args[i].value) : NULL;
    sbtest.args[i].validate = args[i].validate;
  }

  sbtest.args[len] = (sb_arg_t){.name = NULL};

  return 0;
}

static wamr_instance_t *sb_wamr_new_module() {
  const char *name = sbtest.lname;

  wamr_import_t imports[] = {};
  wamr_instance_t *instance = NULL;
  wamr_result_t instantiation_result = wamr_instantiate(&instance, bytes, len, imports, 0);

  assert(instantiation_result == wamr_OK);

  wamr_instance_t *instance = WasmEdge_VMCreate(config_cxt, NULL);
  if (instance == NULL) {
    log_text(LOG_FATAL, "can not import wamr module: %s", name);
    goto error;
  }
  Res = WasmEdge_VMLoadWasmFromFile(instance, name);
  if (!WasmEdge_ResultOK(Res)) {
    log_text(LOG_FATAL, "load wasm from file failed: %s", name);
    goto error;
  }
  Res = WasmEdge_VMValidate(instance);
  if (!WasmEdge_ResultOK(Res)) {
    printf("validation wasm module failed: %s\n", WasmEdge_ResultGetMessage(Res));
    goto error;
  }
  Res = WasmEdge_VMInstantiate(instance);
  if (!WasmEdge_ResultOK(Res)) {
    printf("instantiation wasm module failed: %s\n", WasmEdge_ResultGetMessage(Res));
    goto error;
  }

  return instance;
error:
  return NULL;
}

/* Close interpreter state */

int sb_wamr_free_module(wamr_instance_t *instance) {
  return 0;
}

/* Execute a given command */
static int execute_command(const char *cmd) {
  return 0;
}

/* Prepare command */

int sb_wamr_cmd_prepare(void) {
  return execute_command(PREPARE_FUNC);
}

/* Cleanup command */

int sb_wamr_cmd_cleanup(void) {
  return execute_command(CLEANUP_FUNC);
}

/* Help command */

int sb_wamr_cmd_help(void) {
  return execute_command(HELP_FUNC);
}

/* Check if a specified hook exists */

bool sb_wamr_loaded(void) {
  return vm_context != NULL;
}

static void *cmd_worker_thread(void *arg) {
  sb_thread_ctxt_t *ctxt = (sb_thread_ctxt_t *)arg;

  sb_tls_thread_id = ctxt->id;

  /* Initialize thread-local RNG state */
  sb_rand_thread_init();

  wamr_instance_t *const instance = sb_wamr_new_module();

  if (instance == NULL) {
    log_text(LOG_FATAL, "failed to create a thread to execute command");
    return NULL;
  }

  sb_wamr_free_module(instance);

  return NULL;
}

int sb_wamr_report_thread_init(void) {
  if (tls_wamr_ctxt.instance == NULL) {
    sb_wamr_new_module();
    export_options(tls_wamr_ctxt.instance);
  }

  return 0;
}

void sb_wamr_report_thread_done(void *arg) {
  (void)arg; /* unused */

  if (sb_wamr_loaded())
    sb_wamr_free_module(tls_wamr_ctxt.instance);
}
