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

BUILT_SOURCES = sb_wasm_test.c

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = $(BUILT_SOURCES:.h=)

SUFFIXES = .c


WASM_OPTS = -nostdlib -z stack-size=8192 -Wl,--initial-memory=65536 -Wl,--no-entry -Wl,--allow-undefined 

sb_wasm_test.wasm: sb_wasm_test.c
	clang --target=wasm32-wasi --sysroot=${WASI_SDK_HOME}/share/wasi-sysroot -o sb_wasm_test.wasm sb_wasm_test.c
