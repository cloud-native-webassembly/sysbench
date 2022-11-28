# Copyright (C) 2016-2018 Alexey Kopytov <akopytov@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA

export LD_LIBRARY_PATH=${LLVM_HOME}/lib64:${GCC_HOME}/lib64:${LD_LIBRARY_PATH}
export PATH=${LLVM_HOME}/bin:${GCC_HOME}/bin:${PATH}
CLANG := ${WASI_SDK_HOME}/bin/clang
WASM_OPTS := --target=wasm32-wasi -fPIC --sysroot=${WASI_SDK_HOME}/share/wasi-sysroot

BUILT_SOURCES = udf.c wasm.c

udf.wasm: udf.c
	clang ${WASM_OPTS} \
      -nostdlib \
      -z stack-size=8192 \
      -Wl,--initial-memory=65536 \
      -Wl,--no-entry \
      -Wl,--allow-undefined \
      -Wl,--export=event \
      -Wl,--export=buffer \
      -o udf.wasm udf.c