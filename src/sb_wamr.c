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
#include "sb_util.h"
#include "sb_wamr.h"
#include "sb_wasm.h"
#include "wasm_c_api.h"
#include "wasm_export.h"

typedef struct
{
    uint8_t *wamr_file_buffer;
    uint32_t wamr_buffer_size;
} sb_wamr_module_context;

typedef struct
{
    wasm_module_t wamr_module;
    wasm_module_inst_t wamr_module_inst;
    wasm_exec_env_t wamr_exec_env;
} sb_wamr_sandbox_context;

static bool sb_wamr_init(void) {
    log_text(LOG_DEBUG, "initialize wamr runtime success");

    return true;
}

static int sb_wamr_prepare(void) {
    return SUCCESS;
}

static int sb_wamr_destroy(void) {
    return SUCCESS;
}

static int sb_wamr_function_apply(sb_wasm_sandbox_context *context, const char *fname, int thread_id, int64_t *carrier) {
    sb_wamr_sandbox_context *sandbox_context = (sb_wamr_sandbox_context *)context;
    wasm_val_t args[1], results[1];
    args[0].kind = WASM_I64;
    args[0].of.i64 = *carrier;

    wasm_function_inst_t func = wasm_runtime_lookup_function(sandbox_context->wamr_module_inst, fname, NULL);
    if (func == NULL) {
        log_text(LOG_FATAL, "function %s not found in wasm module", fname);
        return FAILURE;
    } else {
        /* call the WASM function */
        if (wasm_runtime_call_wasm_a(sandbox_context->wamr_exec_env, func, 1, results, 1, args)) {
            if (results[0].kind != WASM_I64) {
                log_text(LOG_FATAL, "function %s does not return a WASM_I64 value", fname);
                return FAILURE;
            } else {
                *carrier = results[0].of.i64;
                return SUCCESS;
            }
        } else {
            /* exception is thrown if call fails */
            log_text(LOG_FATAL, "%s\n", wasm_runtime_get_exception(sandbox_context->wamr_module_inst));
            return FAILURE;
        }
    }
}

static bool sb_wamr_function_available(sb_wasm_sandbox_context *context, const char *fname) {
    sb_wamr_sandbox_context *sandbox_context = (sb_wamr_sandbox_context *)context;
    wasm_function_inst_t func = wasm_runtime_lookup_function(sandbox_context->wamr_module_inst, fname, NULL);
    if (func == NULL) {
        return false;
    } else {
        return true;
    }
}

static sb_wasm_sandbox *sb_wamr_create_sandbox(sb_wasm_module *module, int thread_id) {
    char error_buffer[128];
    RuntimeInitArgs init_args;
    void *heap_buf = malloc(module->heap_size);
    if (heap_buf == NULL) {
        log_text(LOG_FATAL, "cannot alloc heap (%d) for wamr sandbox (%d)", module->heap_size, thread_id);
        goto error;
    }

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = heap_buf;
    init_args.mem_alloc_option.pool.heap_size = module->heap_size;

    /* initialize runtime environment with user configurations*/
    if (!wasm_runtime_full_init(&init_args)) {
        log_text(LOG_FATAL, "init wamr runtime failed");
        goto error;
    }
    wasm_module_t wamr_module = wasm_runtime_load(module->file_buffer,
                                                  module->file_size,
                                                  error_buffer, sizeof(error_buffer));
    if (wamr_module == NULL) {
        log_text(LOG_FATAL, "load wamr module failed [%s]", error_buffer);
        goto error;
    }

    wasm_module_inst_t wamr_module_inst = wasm_runtime_instantiate(wamr_module,
                                                                   module->stack_size, module->heap_size,
                                                                   error_buffer, sizeof(error_buffer));
    if (wamr_module_inst == NULL) {
        log_text(LOG_FATAL, "instantiate wasm module failed, %s", error_buffer);
        goto error;
    }
    wasm_exec_env_t wamr_exec_env = wasm_runtime_create_exec_env(wamr_module_inst, module->stack_size);
    if (wamr_exec_env == NULL) {
        log_text(LOG_FATAL, "create wasm execute environment failed");
        goto error;
    }

    sb_wamr_sandbox_context *sandbox_context = malloc(sizeof(sb_wamr_sandbox_context));
    sandbox_context->wamr_module = wamr_module;
    sandbox_context->wamr_module_inst = wamr_module_inst;
    sandbox_context->wamr_exec_env = wamr_exec_env;

    sb_wasm_sandbox *sandbox = malloc(sizeof(sb_wasm_sandbox));
    snprintf(sandbox->name, sizeof(sandbox->name), "wamr-sandbox-%d", thread_id);
    sandbox->context = (sb_wasm_sandbox_context *)sandbox_context;
    sandbox->function_apply = sb_wamr_function_apply;
    sandbox->function_available = sb_wamr_function_available;
    return sandbox;
error:
    return NULL;
};

static sb_wasm_runtime wamr_vm = {
    .runtime_type = SB_WASM_RUNTIME_WAMR,
    .runtime_name = "wamr",
    .init = sb_wamr_init,
    .prepare = sb_wamr_prepare,
    .destroy = sb_wamr_destroy,
    .load_module = NULL,  // use default load method
    .create_sandbox = sb_wamr_create_sandbox};

sb_wasm_runtime *create_wamr_runtime(void) {
    return &wamr_vm;
}
