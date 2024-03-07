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

CLANG = ${WASI_SDK_HOME}/bin/clang
WASI_SDK_OPTS = --target=wasm32-wasi --sysroot=${WASI_SDK_HOME}/share/wasi-sysroot
WASM_OPTS = -fPIC -z stack-size=8192 -Wl,--initial-memory=65536 -Wl,--no-entry
WASM_EXPORTS = -Wl,--export=event


if HAVE_WASM
bin_PROGRAMS = \
    build_user_id \
    normalize_with_base \
    pass_data \
    point_polygen_distance \
    split_string \
    string_to_double \
    string_to_int

build_user_id_SOURCES = build_user_id.c
normalize_with_base_SOURCES = normalize_with_base.c
pass_data_SOURCES = pass_data.c
point_polygen_distance_SOURCES = point_polygen_distance.c
split_string_SOURCES = split_string.c
string_to_double_SOURCES = string_to_double.c
string_to_int_SOURCES = string_to_int.c

%: %.c
  $(CLANG) ${WASI_SDK_OPTS} ${WASM_OPTS} ${WASM_EXPORTS} -o $@ $^

pass_data: pass_data.c
  $(CLANG) ${WASI_SDK_OPTS} ${WASM_OPTS} ${WASM_EXPORTS} -Wl,--export=create_buffer -o $@ $^

endif