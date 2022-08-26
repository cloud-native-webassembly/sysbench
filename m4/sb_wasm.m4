# Copyright (C) 2016 liuqiming <liuqiming1985@gmail.com>
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

# ---------------------------------------------------------------------------
# Macro: SB_WASM
# ---------------------------------------------------------------------------
# modified https://github.com/libyal/libsmraw/blob/main/m4/python.m4

AC_DEFUN([AX_PROG_WASM],
  [AS_IF(
    [test "x${WASM_HOME}" != x],
    [
      ax_wasm_progs="wasmedge wasmtime wasmer"
      AC_CHECK_PROGS(
        [WASM],
        [$ax_wasm_progs],
        [Unknown],
        [${WASM_HOME}/bin])
    ],
    [
      ax_wasm_progs="wasmedge wasmtime wasmer"
      AC_CHECK_PROGS(
        [WASM],
        [$ax_wasm_progs],
        [Unknown])
      WASM_HOME=$(readlink -f $(dirname $(dirname $(which $WASM))))
    ])

  AS_IF(
    [test "x${WASM}" != x],
    [AC_SUBST([WASM])],
    [AC_MSG_ERROR(
      [Unable to find wasm runtime])
    ])
  ])

AC_DEFUN([AX_WASM_CHECK],
  [AX_PROG_WASM
  AS_CASE(
      [$WASM],
      [wasmer],[
        WASM_CFLAGS="-I${WASM_HOME}/include"
        WASM_LDFLAGS="-L${WASM_HOME}/lib"
        WASM_LIBS="-lwasmer"
        AC_SUBST([WASM_CFLAGS])
        AC_SUBST([WASM_LDFLAGS])
        AC_SUBST([WASM_LIBS])
      ],
      [wasmtime],[
        WASM_CFLAGS="-I${WASM_HOME}/include"
        WASM_LDFLAGS="-L${WASM_HOME}/lib -lpthread -ldl -lm"
        WASM_LIBS="-lwasmtime"
        AC_SUBST([WASM_CFLAGS])
        AC_SUBST([WASM_LDFLAGS])
        AC_SUBST([WASM_LIBS])
      ],
      [wasmedge],[
        WASM_CFLAGS="-I${WASM_HOME}/include"
        WASM_LDFLAGS="-L${WASM_HOME}/lib -lstdc++ -lgcc_s -pthread -ldl -lutil -lm"
        WASM_LIBS="-lwasmedge_c"
        AC_SUBST([WASM_CFLAGS])
        AC_SUBST([WASM_LDFLAGS])
        AC_SUBST([WASM_LIBS])
      ],
      [*],[
        ac_cv_enable_wasm=no
      ]
  )
  AM_CONDITIONAL(USE_WASMER,[test "x${WASM}" = xwasmer])
  AM_CONDITIONAL(USE_WASMTIME,[test "x${WASM}" = xwasmtime])
  AM_CONDITIONAL(USE_WASMEDGE,[test "x${WASM}" = xwasmedge])
])

AC_DEFUN([AX_WASM_CHECK_ENABLE],
  [
  AC_ARG_WITH([wasm],
              AS_HELP_STRING([--with-wasm],
                            [compile with wasm support (default is enabled)]),
              [], [ac_cv_enable_wasm=yes])
  AS_IF(
    [test "x${ac_cv_enable_wasm}" != xno],
    [AX_WASM_CHECK])

  AM_CONDITIONAL(
    HAVE_WASM,
    [test "x${ac_cv_enable_wasm}" != xno])
  ])

AC_DEFUN([SB_WASM], [
AX_WASM_CHECK_ENABLE
])
