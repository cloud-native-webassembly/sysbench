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

static bool sb_wasmedge_init(void) {
  if (!wasm_runtime_init()) {
    log_text(LOG_FATAL, "init wamr runtime failed");
    goto error;
  }
  return true;
error:
  return false;
}

static int sb_wasmedge_destroy(void) {}
static sb_wasm_module sb_wasmedge_load_module(const char *filepath) {}

static sb_wasm_vm wasmedge_vm = {
    .runtime_type = SB_WASM_RUNTIME_WASMEDGE,
    .runtime_name = "wasmedge",
    .init = sb_wasmedge_init,
    .destroy = sb_wasmedge_destroy,
    .load_module = sb_wasmedge_load_module
    
    };

static sb_wasm_module wasmedge_module = {

};

static WasmEdge_VMContext **contexts CK_CC_CACHELINE;



static int wasmedge_call_function(WasmEdge_VMContext *context, const char *fname, int thread_id) {
  WasmEdge_Value params[1] = {WasmEdge_ValueGenI32(20)};
  WasmEdge_Value returns[1];
  WasmEdge_String func_name = WasmEdge_StringCreateByCString(fname);
  WasmEdge_Result Res;
  Res = WasmEdge_VMExecute(context, func_name, params, 1, returns, 1);
  WasmEdge_StringDelete(func_name);
  if (WasmEdge_ResultOK(Res)) {
    // printf("Get the result: %d\n", WasmEdge_ValueGetI32(returns[0]));
    return 0;
  } else {
    fprintf(stderr, "call function [%s] failed: %s\n", fname, WasmEdge_ResultGetMessage(Res));
    return 1;
  }
}

static sb_wasm_module *sb_wasmedge_load_module(const char *filepath) {
}

/* Load a specified Lua script */
#define BUF_LEN 256
static WasmEdge_String FuncNames[BUF_LEN];
static WasmEdge_FunctionTypeContext *FuncTypes[BUF_LEN];

sb_test_t *sb_load_wasmedge(const char *testname, int argc, char *argv[]) {
  if (testname != NULL) {
    char *tmp = strdup(testname);
    sbtest.sname = strdup(basename(tmp));
    sbtest.lname = tmp;
  } else {
    log_text(LOG_FATAL, "no wasm name provided");
    goto error;
  }

  WasmEdge_StoreContext *StoreCxt = WasmEdge_StoreCreate();
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(NULL, StoreCxt);

  WasmEdge_VMLoadWasmFromFile(VMCxt, sbtest.lname);
  WasmEdge_VMValidate(VMCxt);
  WasmEdge_VMInstantiate(VMCxt);

  uint32_t FuncNum = WasmEdge_VMGetFunctionListLength(VMCxt);

  uint32_t RealFuncNum = WasmEdge_VMGetFunctionList(VMCxt, FuncNames, &FuncTypes, BUF_LEN);
  printf("There are %d function in module %s\n", RealFuncNum, sbtest.lname);
  for (uint32_t I = 0; I < RealFuncNum && I < BUF_LEN; I++) {
    char Buf[BUF_LEN];
    uint32_t Size = WasmEdge_StringCopy(FuncNames[I], Buf, sizeof(Buf));
    printf("Get exported function string length: %u, name: %s\n", Size, Buf);
  }
  WasmEdge_VMDelete(VMCxt);
  WasmEdge_StoreDelete(StoreCxt);
  /* Test operations */
  sbtest.ops = wasmedge_ops;

  if (sb_globals.threads != 1) {
    log_text(LOG_FATAL, "wasmedge script %s only support a single thread", sbtest.sname);
    goto error;
  }

  /* Allocate per-thread interpreters array */
  contexts = (WasmEdge_VMContext **)calloc(sb_globals.threads, sizeof(WasmEdge_VMContext *));
  if (contexts == NULL)
    goto error;

  return &sbtest;

error:
  sb_wasmedge_done();

  return NULL;
}







int sb_wasmedge_set_test_args(sb_arg_t *args, size_t len) {
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

static WasmEdge_VMContext *sb_wasmedge_new_module() {
  WasmEdge_Result Res;
  const char *name = sbtest.lname;
  WasmEdge_ConfigureContext *config_cxt = WasmEdge_ConfigureCreate();
  WasmEdge_StoreContext *store_cxt = WasmEdge_StoreCreate();
  WasmEdge_VMContext *context = WasmEdge_VMCreate(config_cxt, store_cxt);
  if (context == NULL) {
    log_text(LOG_FATAL, "can not import wasmedge module: %s", name);
    goto error;
  }
  Res = WasmEdge_VMLoadWasmFromFile(context, name);
  if (!WasmEdge_ResultOK(Res)) {
    log_text(LOG_FATAL, "load wasm from file failed: %s", name);
    goto error;
  }
  Res = WasmEdge_VMValidate(context);
  if (!WasmEdge_ResultOK(Res)) {
    printf("validation wasm module failed: %s\n", WasmEdge_ResultGetMessage(Res));
    goto error;
  }
  Res = WasmEdge_VMInstantiate(context);
  if (!WasmEdge_ResultOK(Res)) {
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





