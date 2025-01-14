# Copyright (C) 2004 MySQL AB
# Copyright (C) 2004-2017 Alexey Kopytov <akopytov@gmail.com>
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

SUBDIRS = drivers tests lua .

AM_CPPFLAGS += -DDATADIR=\"$(pkgdatadir)\" -DLIBDIR=\"$(pkglibdir)\"

bin_PROGRAMS = sysbench

# The following check will be extended as new database drivers will be added
if USE_MYSQL
mysql_ldadd = drivers/mysql/libsbmysql.a $(MYSQL_LIBS)
endif

if USE_PGSQL
pgsql_ldadd = drivers/pgsql/libsbpgsql.a $(PGSQL_LIBS)
endif

sysbench_SOURCES = sysbench.c sysbench.h sb_timer.c sb_timer.h \
sb_options.c sb_options.h sb_logger.c sb_logger.h sb_list.h db_driver.h \
db_driver.c sb_histogram.c sb_histogram.h sb_rand.c sb_rand.h \
sb_thread.c sb_thread.h sb_barrier.c sb_barrier.h sb_lua.c \
sb_ck_pr.h \
sb_lua.h sb_util.h sb_util.c sb_counter.h sb_counter.c \
sb_file.h sb_file.c \
lua/internal/sysbench.lua.h lua/internal/sysbench.sql.lua.h \
lua/internal/sysbench.rand.lua.h lua/internal/sysbench.cmdline.lua.h  \
lua/internal/sysbench.histogram.lua.h \
xoroshiro128plus.h

sysbench_LDADD = tests/fileio/libsbfileio.a tests/threads/libsbthreads.a \
    tests/memory/libsbmemory.a tests/cpu/libsbcpu.a \
    tests/mutex/libsbmutex.a \
    $(mysql_ldadd) $(pgsql_ldadd) \
    $(LUAJIT_LIBS) $(CK_LIBS)

sysbench_LDFLAGS = $(mysql_ldflags) $(pgsql_ldflags) $(LUAJIT_LDFLAGS)  

if HAVE_PYTHON
sysbench_SOURCES += sb_python.c sb_python.h python/sysbench.py.h
sysbench_LDADD   += $(PYTHON_LIBS)
sysbench_LDFLAGS += $(PYTHON_LDFLAGS)
endif 

if HAVE_WASM
sysbench_SOURCES += sb_wasm.c sb_wasm.h
endif

if HAVE_WAMR
sysbench_SOURCES += sb_wamr.c sb_wamr.h
sysbench_LDADD   += $(WAMR_LIBS)
sysbench_LDFLAGS += $(WAMR_LDFLAGS)
endif

if HAVE_WASMEDGE
sysbench_SOURCES += sb_wasmedge.c sb_wasmedge.h
sysbench_LDADD   += $(WASMEDGE_LIBS)
sysbench_LDFLAGS += $(WASMEDGE_LDFLAGS)
endif

if HAVE_WASMER
sysbench_SOURCES += sb_wasmer.c sb_wasmer.h
sysbench_LDADD   += $(WASMER_LIBS)
sysbench_LDFLAGS += $(WASMER_LDFLAGS)
endif

if HAVE_WASMTIME
sysbench_SOURCES += sb_wasmtime.c sb_wasmtime.h
sysbench_LDADD   += $(WASMTIME_LIBS)
sysbench_LDFLAGS += $(WASMTIME_LDFLAGS)
endif


