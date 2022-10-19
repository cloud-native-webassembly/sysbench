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

#include "sb_file.h"
#include "sb_wamr.h"
#include "sb_wasm.h"
#include "wasm_c_api.h"
#include "wasm_export.h"

typedef struct
{
  uint8_t *wasm_file_buffer;
  wasm_module_t *wamr_module;
} sb_wamr_module_context;

typedef struct {
  wasm_module_inst_t *wamr_module_inst;
  wasm_exec_env_t *exec_env;
} sb_wamr_sandbox_context;

static bool sb_wamr_init(void) {
  if (!wasm_runtime_init()) {
    log_text(LOG_FATAL, "init wamr runtime failed");
    goto error;
  }
  return true;
error:
  return false;
}
static int sb_wamr_destroy(void) {
}

 sb_wasm_sandbox *sb_wamr_create_sandbox(void){};
 bool sb_wamr_function_available(void *context, const char *fname){

 }

static sb_wasm_module *sb_wamr_load_module(const char *filepath) {
  uint32_t size;
  char error_buffer[128];

  sb_wamr_module_context *module_context = malloc(sizeof(sb_wamr_module_context));

  module_context->wasm_file_buffer = sb_load_file_to_buffer(filepath, &size);
  module_context->wamr_module = wasm_runtime_load(module_context->wasm_file_buffer, size, error_buffer, sizeof(error_buffer));
  if (module_context->wamr_module == NULL) {
    log_text(LOG_FATAL, "load wasm module failed, %s", error_buffer);
    goto error;
  }
  sb_wasm_module *module = malloc(sizeof(sb_wasm_module));
  module->context = module_context;
  return module;
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
