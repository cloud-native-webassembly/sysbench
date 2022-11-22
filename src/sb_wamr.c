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
#include <stdlib.h>

#include "sb_file.h"
#include "sb_wamr.h"
#include "sb_wasm.h"
#include "wasm_c_api.h"
#include "wasm_export.h"

typedef struct
{
    uint8_t *wamr_file_buffer;
    wasm_module_t wamr_module;
    wasm_module_inst_t wamr_module_inst;
    wasm_exec_env_t wamr_exec_env;
} sb_wamr_module_context;

typedef struct
{
    wasm_module_inst_t wamr_module_inst;
    wasm_exec_env_t wamr_exec_env;
} sb_wamr_sandbox_context;

static RuntimeInitArgs init_args;

static bool sb_wamr_init(void) {
    log_text(LOG_DEBUG, "start initialize wamr runtime");
    const char *wamr_heap_size_var = getenv("WAMR_HEAP_SIZE");
    long wamr_heap_size;
    if (wamr_heap_size_var == NULL) {
        wamr_heap_size = WAMR_DEFAULT_HEAP_SIZE;
    } else {
        wamr_heap_size = atol(wamr_heap_size_var);
    }
    log_text(LOG_DEBUG, "wamr heap size: %ld", wamr_heap_size);
    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = malloc(wamr_heap_size);
    init_args.mem_alloc_option.pool.heap_size = wamr_heap_size;

    const char *wamr_max_thread_var = getenv("WAMR_MAX_THREAD_NUM");
    int wamr_max_thread_num;
    if (wamr_max_thread_var == NULL) {
        wamr_max_thread_num = WAMR_DEFAULT_THREAD_NUM;
    } else {
        wamr_max_thread_num = atoi(wamr_max_thread_var);
    }
    init_args.max_thread_num = wamr_max_thread_num;
    log_text(LOG_DEBUG, "wamr max thread num: %ld", wamr_max_thread_num);

    /* initialize runtime environment with user configurations*/
    if (!wasm_runtime_full_init(&init_args)) {
        log_text(LOG_FATAL, "init wamr runtime failed");
        goto error;
    }
    return true;
error:
    return false;
}
static int sb_wamr_destroy(void) {
    return 0;
}

static int sb_wamr_call_function(void *context, const char *fname, int thread_id) {
    sb_wamr_sandbox_context *sandbox_context = (sb_wamr_sandbox_context *)context;
    wasm_val_t args[1], results[1];
    args[0].kind = WASM_I32;
    args[0].of.i32 = thread_id;

    wasm_function_inst_t func = wasm_runtime_lookup_function(sandbox_context->wamr_module_inst, fname, NULL);
    if (func == NULL) {
        log_text(LOG_FATAL, "function %s not found in wasm module\n", fname);
        return -1;
    } else {
        /* call the WASM function */
        if (wasm_runtime_call_wasm_a(sandbox_context->wamr_exec_env, func, 1, results, 1, args)) {
            return 0;
        } else {
            /* exception is thrown if call fails */
            log_text(LOG_FATAL, "%s\n", wasm_runtime_get_exception(sandbox_context->wamr_module_inst));
            return -1;
        }
    }
}

sb_wasm_sandbox *sb_wamr_create_sandbox(void *context, int thread_id) {
    char error_buffer[128];
    sb_wamr_module_context *module_context = (sb_wamr_module_context *)context;
    sb_wamr_sandbox_context *sandbox_context = malloc(sizeof(sb_wamr_sandbox_context));
    sandbox_context->wamr_exec_env = wasm_runtime_spawn_exec_env(module_context->wamr_exec_env);
    sandbox_context->wamr_module_inst = get_module_inst(sandbox_context->wamr_exec_env);
    sb_wasm_sandbox *sandbox = malloc(sizeof(sb_wasm_sandbox));
    snprintf(sandbox->name, sizeof(sandbox->name), "wamr-sandbox-%d", thread_id);
    sandbox->context = sandbox_context;
    sandbox->call_function = sb_wamr_call_function;
    return sandbox;
error:
    return NULL;
};

static bool sb_wamr_function_available(void *context, const char *fname) {
    sb_wamr_module_context *module_context = (sb_wamr_module_context *)context;
    wasm_function_inst_t func = wasm_runtime_lookup_function(module_context->wamr_module_inst, fname, NULL);
    if (func == NULL) {
        return false;
    } else {
        return true;
    }
}

static sb_wasm_module *sb_wamr_load_module(const char *filepath) {
    uint32_t size;
    char error_buffer[128];

    sb_wamr_module_context *module_context = malloc(sizeof(sb_wamr_module_context));

    uint8_t *wamr_file_buffer = sb_load_file_to_buffer(filepath, &size);
    if (wamr_file_buffer == NULL) {
        log_text(LOG_FATAL, "load wasm module file[%s] into buffer failed", filepath);
        goto error;
    }

    log_text(LOG_INFO, "load %d bytes from %s", size, filepath);

    wasm_module_t wamr_module = wasm_runtime_load(wamr_file_buffer, size, error_buffer, sizeof(error_buffer));
    if (wamr_module == NULL) {
        log_text(LOG_FATAL, "load wamr module failed [%s]", error_buffer);
        goto error;
    }

    wasm_module_inst_t wamr_module_inst = wasm_runtime_instantiate(wamr_module,
                                                                   WAMR_DEFAULT_STACK_SIZE, WAMR_DEFAULT_HEAP_SIZE,
                                                                   error_buffer, sizeof(error_buffer));
    if (wamr_module_inst == NULL) {
        log_text(LOG_FATAL, "instantiate wasm module failed, %s", error_buffer);
        goto error;
    }
    wasm_exec_env_t wamr_exec_env = wasm_runtime_create_exec_env(wamr_module_inst, WAMR_DEFAULT_STACK_SIZE);
    if (wamr_exec_env == NULL) {
        log_text(LOG_FATAL, "create wasm execute environment failed");
        goto error;
    }

    module_context->wamr_file_buffer = wamr_file_buffer;
    module_context->wamr_module = wamr_module;
    module_context->wamr_module_inst = wamr_module_inst;
    module_context->wamr_exec_env = wamr_exec_env;

    sb_wasm_module *wasm_module = malloc(sizeof(sb_wasm_module));
    wasm_module->context = module_context;
    wasm_module->create_sandbox = sb_wamr_create_sandbox;
    wasm_module->function_available = sb_wamr_function_available;
    return wasm_module;
error:
    return NULL;
}

static sb_wasm_vm wamr_vm = {
    .runtime_type = SB_WASM_RUNTIME_WAMR,
    .runtime_name = "wamr",
    .init = sb_wamr_init,
    .destroy = sb_wamr_destroy,
    .load_module = sb_wamr_load_module

};

sb_wasm_vm *create_wamr_vm(void) {
    return &wamr_vm;
}
