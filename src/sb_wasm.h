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

#ifndef SB_WASM_H
#define SB_WASM_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sysbench.h"

typedef enum
{
  SB_WASM_RUNTIME_UNKNOWN,
  SB_WASM_RUNTIME_WAMR,
  SB_WASM_RUNTIME_WASMEDGE,
  SB_WASM_RUNTIME_WASMER,
  SB_WASM_RUNTIME_WASMTIME
} sb_wasm_runtime_t;

typedef enum
{
  SB_WASM_ERROR_NONE,
  SB_WASM_ERROR_RESTART_EVENT
} sb_wasm_error_t;

typedef struct
{
} sb_wasm_function;

typedef int sb_wasm_call_function(void *context, const char *fname, int thread_id);

typedef struct
{
  void *context;
  sb_wasm_call_function *call_function;
} sb_wasm_sandbox; /* sandbox is an instance of module */

typedef sb_wasm_sandbox *sb_wasm_create_sandbox(void *context, int thread_id);
typedef bool sb_wasm_function_available(void *context, const char *fname);

typedef struct
{
  void *context;
  sb_wasm_create_sandbox *create_sandbox;
  sb_wasm_function_available *function_available;
} sb_wasm_module; /* module is an instance of module file */

typedef bool sb_wasm_init(void);
typedef int sb_wasm_destroy(void);
typedef sb_wasm_module *sb_wasm_load_module(const char *filepath);

typedef struct
{
  sb_wasm_runtime_t runtime_type;
  const char *runtime_name;
  sb_wasm_init *init;
  sb_wasm_destroy *destroy;
  sb_wasm_load_module *load_module;
} sb_wasm_vm;

inline sb_wasm_runtime_t wasm_runtime_name_to_type(const char *runtime)
{
  if (!strcmp(runtime, "wamr"))
  {
    return SB_WASM_RUNTIME_WAMR;
  }
  else if (!strcmp(runtime, "wasmedge"))
  {
    return SB_WASM_RUNTIME_WASMEDGE;
  }
  else if (!strcmp(runtime, "wasmer"))
  {
    return SB_WASM_RUNTIME_WASMER;
  }
  else if (!strcmp(runtime, "wasmtime"))
  {
    return SB_WASM_RUNTIME_WASMTIME;
  }
  else
  {
    return SB_WASM_RUNTIME_UNKNOWN;
  }
}

sb_test_t *sb_load_wasm(const char *testname, const char *runtime);

void sb_wasm_done(void);

bool sb_wasm_custom_command_defined(const char *name);

int sb_wasm_report_thread_init(void);

void sb_wasm_report_thread_done(void *);

bool sb_wasm_loaded(void);

#endif