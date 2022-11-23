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

static bool sb_wasmedge_init(void);
static int sb_wasmedge_destroy(void) {}
static sb_wasm_module *sb_wasmedge_load_module(const char *filepath);

static sb_wasm_vm wasmedge_vm = {
    .runtime_type = SB_WASM_RUNTIME_WASMEDGE,
    .runtime_name = "wasmedge",
    .init = sb_wasmedge_init,
    .destroy = sb_wasmedge_destroy,
    .load_module = sb_wasmedge_load_module

};

typedef struct
{
  uint8_t *wasm_file_buffer;
  WasmEdge_ConfigureContext *config_context;
  WasmEdge_StoreContext *store_context;
  WasmEdge_VMContext *vm_context;
} sb_wasmedge_module_context;

typedef struct {

} sb_wasmedge_sandbox_context;

static WasmEdge_VMContext **contexts CK_CC_CACHELINE;

static int wasmedge_call_function(WasmEdge_VMContext *context, const char *fname, int thread_id) {
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

static sb_wasm_module *sb_wasmedge_load_module(const char *filepath) {
  WasmEdge_Result res;
  sb_wasmedge_module_context *module_context = malloc(sizeof(sb_wasmedge_module_context));
  if (module_context == NULL) {
    log_text(LOG_FATAL, "can not create module_context");
    goto error;
  }
  module_context->config_context = WasmEdge_ConfigureCreate();
  if (module_context->config_context == NULL) {
    log_text(LOG_FATAL, "wasm edge can not create configure context");
    goto error;
  }
  module_context->store_context = WasmEdge_StoreCreate();
  if (module_context->store_context == NULL) {
    log_text(LOG_FATAL, "wasm edge can not create store context");
    goto error;
  }
  module_context->vm_context = WasmEdge_VMCreate(module_context->config_context, module_context->store_context);
  if (module_context->vm_context == NULL) {
    log_text(LOG_FATAL, "wasm edge can not create vm context");
    goto error;
  }

  res = WasmEdge_VMLoadWasmFromFile(module_context->vm_context, filepath);
  if (!WasmEdge_ResultOK(res)) {
    log_text(LOG_FATAL, "load wasm from file failed: %s", filepath);
    goto error;
  }
  res = WasmEdge_VMValidate(module_context->vm_context);
  if (!WasmEdge_ResultOK(res)) {
    printf("validation wasm module failed: %s\n", WasmEdge_ResultGetMessage(res));
    goto error;
  }
  res = WasmEdge_VMInstantiate(module_context->vm_context);
  if (!WasmEdge_ResultOK(res)) {
    printf("instantiation wasm module failed: %s\n", WasmEdge_ResultGetMessage(res));
    goto error;
  }

  sb_wasm_module *module = malloc(sizeof(sb_wasm_module));
  module->context = module_context;
  return module;
error:
  if (module_context != NULL) {
    if (module_context->config_context != NULL) {
      WasmEdge_ConfigureDelete(module_context->config_context);
    }
    if (module_context->store_context != NULL) {
      WasmEdge_StoreDelete(module_context->store_context);
    }
    if (module_context->vm_context != NULL) {
      WasmEdge_VMDelete(module_context->vm_context);
    }
    free(module_context);
  }

  return NULL;
}
