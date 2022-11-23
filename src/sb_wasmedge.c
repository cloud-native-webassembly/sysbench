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

#include <wasmedge/wasmedge.h>

#include "sb_ck_pr.h"
#include "sb_wasm.h"
#include "sb_wasmedge.h"

typedef struct {
    WasmEdge_ConfigureContext *config_context;
    WasmEdge_StoreContext *store_context;
    WasmEdge_VMContext *vm_context;
} sb_wasmedge_sandbox_context;

static WasmEdge_VMContext **contexts CK_CC_CACHELINE;

static bool sb_wasmedge_function_available(void *context, const char *fname) {
    sb_wasmedge_sandbox_context *sandbox_context = (sb_wasmedge_sandbox_context *)context;
    WasmEdge_VMContext *vm_context = sandbox_context->vm_context;
    if (func == NULL) {
        return false;
    } else {
        return true;
    }
}

static int sb_wasmedge_function_apply(void *context, const char *fname, int thread_id) {
    sb_wasmedge_sandbox_context *sandbox_context = (sb_wasmedge_sandbox_context *)context;
    WasmEdge_VMContext *vm_context = sandbox_context->vm_context;
    WasmEdge_Value params[1] = {WasmEdge_ValueGenI32(20)};
    WasmEdge_Value returns[1];
    WasmEdge_String func_name = WasmEdge_StringCreateByCString(fname);
    WasmEdge_Result res;
    res = WasmEdge_VMExecute(context, func_name, params, 1, returns, 1);
    WasmEdge_StringDelete(func_name);
    if (WasmEdge_ResultOK(res)) {
        // printf("Get the result: %d\n", WasmEdge_ValueGetI32(returns[0]));
        return SUCCESS;
    } else {
        fprintf(stderr, "call function [%s] failed: %s\n", fname, WasmEdge_ResultGetMessage(res));
        return FAILURE;
    }
}

static bool sb_wasmedge_init(void) {
    log_text(LOG_DEBUG, "start initialize wasmedge runtime");

    return true;
error:
    return false;
}

static sb_wasm_sandbox *sb_wasmedge_create_sandbox(sb_wasm_module *module, int thread_id) {
    WasmEdge_Result res;

    sb_wasmedge_sandbox_context *sandbox_context = malloc(sizeof(sb_wasmedge_sandbox_context));
    sandbox_context->config_context = WasmEdge_ConfigureCreate();
    if (sandbox_context->config_context == NULL) {
        log_text(LOG_FATAL, "wasmedge can not create configure context");
        goto error;
    }
    sandbox_context->store_context = WasmEdge_StoreCreate();
    if (sandbox_context->store_context == NULL) {
        log_text(LOG_FATAL, "wasm edge can not create store context");
        goto error;
    }
    sandbox_context->vm_context = WasmEdge_VMCreate(sandbox_context->config_context, sandbox_context->store_context);
    if (sandbox_context->vm_context == NULL) {
        log_text(LOG_FATAL, "wasm edge can not create vm context");
        goto error;
    }

    res = WasmEdge_VMLoadWasmFromBuffer(module->wasm_buffer, module->wasm_buffer_size);
    if (!WasmEdge_ResultOK(res)) {
        log_text(LOG_FATAL, "load wasm from buffer failed");
        goto error;
    }
    res = WasmEdge_VMValidate(sandbox_context->vm_context);
    if (!WasmEdge_ResultOK(res)) {
        printf("validation wasm module failed: %s\n", WasmEdge_ResultGetMessage(res));
        goto error;
    }
    res = WasmEdge_VMInstantiate(sandbox_context->vm_context);
    if (!WasmEdge_ResultOK(res)) {
        printf("instantiation wasm module failed: %s\n", WasmEdge_ResultGetMessage(res));
        goto error;
    }

    sb_wasm_sandbox *sandbox = malloc(sizeof(sb_wasm_sandbox));
    snprintf(sandbox->name, sizeof(sandbox->name), "wasmedge-sandbox-%d", thread_id);
    sandbox->context = sandbox_context;
    sandbox->function_apply = sb_wasmedge_function_apply;
    sandbox->function_available = sb_wasmedge_function_available;
    return sandbox;
error:
    if (sandbox_context != NULL) {
        if (sandbox_context->config_context != NULL) {
            WasmEdge_ConfigureDelete(module_context->config_context);
        }
        if (sandbox_context->store_context != NULL) {
            WasmEdge_StoreDelete(module_context->store_context);
        }
        if (sandbox_context->vm_context != NULL) {
            WasmEdge_VMDelete(module_context->vm_context);
        }
        free(sandbox_context);
    }

    return NULL;
}

static int sb_wasmedge_prepare(void) {
    return SUCCESS;
}

static int sb_wasmedge_destroy(void) {
    return SUCCESS;
}

static sb_wasm_runtime wasmedge_vm = {
    .runtime_type = SB_WASM_RUNTIME_WASMEDGE,
    .runtime_name = "wasmedge",
    .init = sb_wasmedge_init,
    .prepare = sb_wasmedge_prepare,
    .destroy = sb_wasmedge_destroy,
    .load_module = NULL,  // use default load method
    .create_sandbox = sb_wasmedge_create_sandbox};

sb_wasm_runtime *create_wasmedge_vm(void) {
    return &wasmedge_vm;
}