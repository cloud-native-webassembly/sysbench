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
#define PY_SSIZE_T_CLEAN
#include <Python.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "sb_python.h"
#include "db_driver.h"
#include "sb_rand.h"
#include "sb_thread.h"
#include "sb_ck_pr.h"

#include "python/sysbench.py.h"

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
  PyObject *module;
} sb_python_ctxt_t;

typedef struct
{
  int id;
  db_bind_type_t type;
  void *buf;
  unsigned long buflen;
  char is_null;
} sb_python_bind_t;

typedef struct
{
  const char *name;
  const unsigned char *source;
  /* Use a pointer, since _len variables are not compile-time constants */
  size_t *source_len;
} internal_script_t;

typedef enum
{
  SB_PYTHON_ERROR_NONE,
  SB_PYTHON_ERROR_RESTART_EVENT
} sb_python_error_t;

bool sb_python_more_events(int);
int sb_python_set_test_args(sb_arg_t *, size_t);

/* Python Modules */

static PyObject **modules CK_CC_CACHELINE;

static sb_test_t sbtest CK_CC_CACHELINE;
static TLS sb_python_ctxt_t tls_python_ctxt CK_CC_CACHELINE;

/* List of pre-loaded internal scripts */
static internal_script_t internal_scripts[] = {
    {"sysbench.py", sysbench_py, &sysbench_py_len},
    {NULL, NULL, 0}};

static PyObject *pModule;

/* Lua test operations */

static int sb_python_op_init(void);
static int sb_python_op_done(void);
static int sb_python_op_thread_init(int);
static int sb_python_op_thread_run(int);
static int sb_python_op_thread_done(int);
static sb_event_t sb_python_op_next_event(int);
static int sb_python_op_execute_event(sb_event_t *event, int);

/* parse and transfer data */
PyObject *parse_string_to_bigint(char *args);

static sb_operations_t python_ops = {
    .init = sb_python_op_init,
    .thread_init = sb_python_op_thread_init,
    .thread_done = sb_python_op_thread_done,
    .next_event = sb_python_op_next_event,
    .execute_event = sb_python_op_execute_event,
    .report_intermediate = db_report_intermediate,
    .report_cumulative = db_report_cumulative,
    .done = sb_python_op_done};

/* Lua test commands */
static int sb_python_cmd_prepare(void);
static int sb_python_cmd_cleanup(void);
static int sb_python_cmd_help(void);

/* Initialize interpreter state */
static PyObject *sb_python_new_module(void);

/* Close interpretet state */
static int sb_python_free_module(PyObject *);

static void call_error(PyObject *module, const char *name)
{
  const char *const err = PyModule_GetName(module);
  log_text(LOG_FATAL, "[%s] function failed in module [%s]", name,
           err ? err : "(not a string)");
}

static bool func_available(PyObject *module, const char *func)
{
  PyObject *pFunc = PyObject_GetAttrString(module, func);
  bool available = false;
  if (pFunc != NULL)
  {
    available = PyCallable_Check(pFunc);
    Py_DECREF(pFunc);
  }
  return available;
}

static int python_call_function(PyObject *module, const char *fname, int thread_id, PyObject *pArgs)
{
  PyObject *pFunc = PyObject_GetAttrString(pModule, fname);
  if (pFunc && PyCallable_Check(pFunc))
  {
    // pass value to udf
    PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
    Py_DECREF(pArgs);
    if (pValue != NULL)
    {
      // TODO this is return value
      Py_DECREF(pValue);
    }
    else
    {
      Py_DECREF(pFunc);
      PyErr_Print();
      fprintf(stderr, "call function %s failed\n", fname);
      return 1;
    }
  }
  else
  {
    if (PyErr_Occurred())
      PyErr_Print();
    fprintf(stderr, "Cannot find function \"%s\"\n", fname);
  }
  Py_XDECREF(pFunc);
  return 0;
}

static int do_export_options(PyObject *module, bool global)
{
  return 0;
}

static int export_options(PyObject *module)
{
  if (do_export_options(module, false))
    return 1;

  return 0;
}

static int load_internal_scripts()
{
  for (internal_script_t *s = internal_scripts; s->name != NULL; s++)
  {
    PyRun_SimpleString((const char *)s->source);
  }

  return 0;
}

sb_test_t *sb_load_python(const char *testname, int argc, char *argv[])
{
  if (testname != NULL)
  {
    char *tmp = strdup(testname);
    sbtest.sname = strdup(basename(tmp));
    sbtest.lname = tmp;
  }
  else
  {
    log_text(LOG_FATAL, "no python name provided");
    goto error;
  }
  Py_Initialize();
  // PySys_SetArgv(argc, &argv); TODO: set argv
  load_internal_scripts();

  PyObject *pName = PyUnicode_DecodeFSDefault(sbtest.lname);
  pModule = PyImport_Import(pName);
  if (pModule == NULL)
  {
    log_text(LOG_FATAL, "can not import python module: %s", sbtest.sname);
    goto error;
  }

  /* Test commands */
  if (func_available(pModule, PREPARE_FUNC))
    sbtest.builtin_cmds.prepare = &sb_python_cmd_prepare;

  if (func_available(pModule, CLEANUP_FUNC))
    sbtest.builtin_cmds.cleanup = &sb_python_cmd_cleanup;

  if (func_available(pModule, HELP_FUNC))
    sbtest.builtin_cmds.help = &sb_python_cmd_help;

  /* Test operations */
  sbtest.ops = python_ops;

  if (func_available(pModule, THREAD_RUN_FUNC))
    sbtest.ops.thread_run = &sb_python_op_thread_run;

  if (sb_globals.threads != 1)
  {
    log_text(LOG_FATAL, "python script %s only support a single thread", sbtest.sname);
    goto error;
  }

  /* Allocate per-thread interpreters array */
  modules = (PyObject **)calloc(sb_globals.threads, sizeof(PyObject *));
  if (modules == NULL)
    goto error;
  modules[0] = pModule;

  return &sbtest;

error:

  sb_python_done();

  return NULL;
}

void sb_python_done(void)
{
  // sb_python_free_module(pModule);
  pModule = NULL;

  xfree(modules);

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

int sb_python_op_init(void)
{
  if (export_options(pModule))
    return 1;

  if (func_available(pModule, INIT_FUNC))
  {
    PyObject *pArgs = PyTuple_New(0);
    if (python_call_function(pModule, INIT_FUNC, -1, pArgs))
    {
      call_error(pModule, INIT_FUNC);
      return 1;
    }
  }

  if (!func_available(pModule, EVENT_FUNC))
  {
    log_text(LOG_FATAL, "cannot find the event() function in %s",
             sbtest.sname);
    return 1;
  }

  return 0;
}

int sb_python_op_thread_init(int thread_id)
{
  PyObject *module;

  module = sb_python_new_module();
  if (module == NULL)
    return 1;

  modules[thread_id] = module;

  if (export_options(module))
    return 1;

  if (func_available(module, THREAD_INIT_FUNC))
  {
    PyObject *pArgs = PyTuple_New(0);
    if (python_call_function(module, THREAD_INIT_FUNC, thread_id, pArgs))
    {
      call_error(module, THREAD_INIT_FUNC);
      return 1;
    }
  }

  return 0;
}

int sb_python_op_thread_run(int thread_id)
{
  PyObject *const module = modules[thread_id];

  if (func_available(module, THREAD_RUN_FUNC))
  {
    PyObject *pArgs = PyTuple_New(0);
    if (python_call_function(module, THREAD_RUN_FUNC, thread_id, pArgs))
    {
      call_error(module, THREAD_RUN_FUNC);
      return 1;
    }
  }

  return 0;
}

int sb_python_op_thread_done(int thread_id)
{
  PyObject *const module = modules[thread_id];
  if (func_available(module, THREAD_RUN_FUNC))
  {
    PyObject *pArgs = PyTuple_New(0);
    if (python_call_function(module, THREAD_RUN_FUNC, thread_id, pArgs))
    {
      call_error(module, THREAD_RUN_FUNC);
      return 1;
    }
  }

  sb_python_free_module(module);
  return 0;
}

int sb_python_op_done(void)
{
  if (func_available(pModule, DONE_FUNC))
  {
    PyObject *pArgs = PyTuple_New(0);
    if (python_call_function(pModule, DONE_FUNC, -1, pArgs))
    {
      call_error(pModule, DONE_FUNC);
      return 1;
    }
  }
  sb_python_done();

  return 0;
}

inline sb_event_t sb_python_op_next_event(int thread_id)
{
  sb_event_t req;

  (void)thread_id; /* unused */

  req.type = SB_REQ_TYPE_SCRIPT;

  return req;
}

int sb_python_op_execute_event(sb_event_t *r, int thread_id)
{
  PyObject *const module = modules[thread_id];
  sb_test_t *test = r->test;

  PyObject *pArgs;
  if (!strcmp(test->sname, "string_to_bigint"))
  {
    pArgs = parse_string_to_bigint(r->args);
  }
  else
  {
    log_text(LOG_FATAL, "UDF %s not supported now!", test->sname);
    return 1;
  }

  if (python_call_function(module, EVENT_FUNC, thread_id, pArgs))
  {
    call_error(module, EVENT_FUNC);
    return 1;
  }
  return 0;
}

int sb_python_set_test_args(sb_arg_t *args, size_t len)
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

static PyObject *sb_python_new_module(void)
{
  PyObject *module = pModule; // single thread single module

  tls_python_ctxt.module = module;

  return module;
}

/* Close interpreter state */

int sb_python_free_module(PyObject *module)
{
  return 0;
}

/* Execute a given command */
static int execute_command(const char *cmd)
{
  return 0;
}

/* Prepare command */

int sb_python_cmd_prepare(void)
{
  return execute_command(PREPARE_FUNC);
}

/* Cleanup command */

int sb_python_cmd_cleanup(void)
{
  return execute_command(CLEANUP_FUNC);
}

/* Help command */

int sb_python_cmd_help(void)
{
  return execute_command(HELP_FUNC);
}

/* Check if a specified hook exists */

bool sb_python_loaded(void)
{
  return pModule != NULL;
}

int sb_python_report_thread_init(void)
{
  if (tls_python_ctxt.module == NULL)
  {
    sb_python_new_module();
    export_options(tls_python_ctxt.module);
  }

  return 0;
}

void sb_python_report_thread_done(void *arg)
{
  (void)arg; /* unused */

  if (sb_python_loaded())
    sb_python_free_module(tls_python_ctxt.module);
}

PyObject *parse_string_to_bigint(char *args)
{
  PyObject *pArgs = PyTuple_New(1);
  PyObject *pString = Py_BuildValue("s", args);
  PyTuple_SetItem(pArgs, 0, pString);
  return pArgs;
}