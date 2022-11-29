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

typedef enum {
    SB_WASM_RUNTIME_UNKNOWN,
    SB_WASM_RUNTIME_WAMR,
    SB_WASM_RUNTIME_WASMEDGE,
    SB_WASM_RUNTIME_WASMER,
    SB_WASM_RUNTIME_WASMTIME
} sb_wasm_runtime_t;

typedef enum {
    SB_WASM_ERROR_NONE,
    SB_WASM_ERROR_RESTART_EVENT
} sb_wasm_error_t;

typedef struct
{
} sb_wasm_function;

typedef struct
{
} sb_wasm_sandbox_context;

typedef int (*sb_wasm_function_apply_func)(sb_wasm_sandbox_context *context, const char *fname, int thread_id, int64_t *carrier);
typedef bool (*sb_wasm_function_available_func)(sb_wasm_sandbox_context *context, const char *fname);

typedef struct
{
    char name[24];
    int32_t *heap_base;
    int32_t *buffer_addr;
    sb_wasm_sandbox_context *context;
    sb_wasm_function_apply_func function_apply;
    sb_wasm_function_available_func function_available;
} sb_wasm_sandbox; /* sandbox is an instance of module */

typedef struct
{
    uint8_t *file_buffer;
    uint32_t file_size;
    uint32_t stack_size;
    uint32_t heap_size;
    uint32_t max_thread_num;
    uint32_t buffer_size;
} sb_wasm_module; /* module is an instance of module file */

typedef bool (*sb_wasm_init_func)(void);
typedef int (*sb_wasm_prepare_func)(void);
typedef int (*sb_wasm_destroy_func)(void);
typedef sb_wasm_module *(*sb_wasm_load_module_func)(const char *filepath);
typedef sb_wasm_sandbox *(*sb_wasm_create_sandbox_func)(sb_wasm_module *module, int thread_id);
typedef struct
{
    sb_wasm_runtime_t runtime_type;
    const char *runtime_name;
    sb_wasm_init_func init;
    sb_wasm_prepare_func prepare;
    sb_wasm_destroy_func destroy;
    sb_wasm_load_module_func load_module;
    sb_wasm_create_sandbox_func create_sandbox;
} sb_wasm_runtime;

int64_t wasm_addr_encode(int32_t *base, int32_t *addr, int32_t size);

void wasm_addr_decode(int32_t *base, int64_t val, void **addr, int32_t *size);

sb_wasm_runtime_t wasm_runtime_name_to_type(const char *runtime);

sb_test_t *sb_load_wasm(const char *testname, const char *runtime);

void sb_wasm_done(void);

int sb_wasm_report_thread_init(void);

void sb_wasm_report_thread_done(void *);

bool sb_wasm_loaded(void);

#define WASM_STACK_SIZE 8092
#define WASM_HEAP_SIZE 1024 * 1024
#define WASM_MAX_THREAD_NUM 16
#define WASM_BUFFER_SIZE 1024 * 1024

#endif