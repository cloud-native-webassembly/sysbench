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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "sb_wasm.h"

#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif

#include "db_driver.h"
#include "sb_ck_pr.h"
#include "sb_rand.h"
#include "sb_thread.h"
#include "sb_wasmtime.h"

static int sb_wasmtime_init(void) {}
static int sb_wasmtime_destroy(void) {}
static sb_wasm_module sb_wasmtime_load_module(const char *filepath) {}

static sb_wasm_vm wasmtime_vm = {
    .runtime_type = SB_WASM_RUNTIME_WASMTIME,
    .runtime_name = "wasmtime",
    .init = sb_wasmtime_init,
    .destroy = sb_wasmtime_destroy,
    .load_module = sb_wasmtime_load_module

};
