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

AC_DEFUN([AX_CHECK_WAMR], [
  AC_ARG_WITH(
    [wamr],
    AS_HELP_STRING([--with-wamr],[compile with wamr support (default is enabled)]),
    [],
    [ac_cv_enable_wamr=no]
  )
  AS_IF(
    [test "x${ac_cv_enable_wamr}" != xno],
    [
      AS_IF(
        [test "x${WAMR_HOME}" != x],
        [
          ax_wamr_progs="iwasm"
          AC_CHECK_PROGS([WAMR], [$ax_wamr_progs], [Unknown], [${WAMR_HOME}/bin])
        ],
        [
          ax_wamr_progs="iwasm"
          AC_CHECK_PROGS(
            [WAMR],
            [$ax_wamr_progs],
            [Unknown])
          WAMR_HOME=$(readlink -f $(dirname $(dirname $(which $WAMR))))
        ]
      )
      
      AS_IF(
        [test "x${WAMR}" != x],
        [
          WAMR_CFLAGS="-I${WAMR_HOME}/include"
          WAMR_LDFLAGS="-L${WAMR_HOME}/lib -Wl,-rpath=${WAMR_HOME}/lib"
          WAMR_LIBS="-liwasm"
          AC_SUBST([WAMR_HOME])
          AC_SUBST([WAMR])
          AC_SUBST([WAMR_CFLAGS])
          AC_SUBST([WAMR_LDFLAGS])
          AC_SUBST([WAMR_LIBS])
          AC_DEFINE(HAVE_WAMR,1,[Define if wamr was found])
        ],
        [
          ac_cv_enable_wamr=no
          AC_MSG_ERROR([Unable to find wamr runtime])
        ]
      )
    ])

  AM_CONDITIONAL(HAVE_WAMR, [test "x${ac_cv_enable_wamr}" != xno])
])

AC_DEFUN([AX_CHECK_WASMEDGE], [
  AC_ARG_WITH(
    [wasmedge],
    AS_HELP_STRING([--with-wasmedge],[compile with wasmedge support (default is enabled)]),
    [],
    [ac_cv_enable_wasmedge=no]
  )
  AS_IF(
    [test "x${ac_cv_enable_wasmedge}" != xno],
    [
      AS_IF(
        [test "x${WASMEDGE_HOME}" != x],
        [
          ax_wasmedge_progs="wasmedge"
          AC_CHECK_PROGS([WASMEDGE], [$ax_wasmedge_progs], [Unknown], [${WASMEDGE_HOME}/bin])
        ],
        [
          ax_wasmedge_progs="wasmedge"
          AC_CHECK_PROGS(
            [WASMEDGE],
            [$ax_wasmedge_progs],
            [Unknown])
          WASMEDGE_HOME=$(readlink -f $(dirname $(dirname $(which $WASMEDGE))))
        ]
      )
      
      AS_IF(
        [test "x${WASMEDGE}" != x],
        [
          WASMEDGE_CFLAGS="-I${WASMEDGE_HOME}/include"
          WASMEDGE_LDFLAGS="-L${WASMEDGE_HOME}/lib64 -Wl,-rpath=${WASMEDGE_HOME}/lib64"
          WASMEDGE_LIBS="-lwasmedge"
          AC_SUBST([WASMEDGE_HOME])
          AC_SUBST([WASMEDGE])
          AC_SUBST([WASMEDGE_CFLAGS])
          AC_SUBST([WASMEDGE_LDFLAGS])
          AC_SUBST([WASMEDGE_LIBS])
          AC_DEFINE(HAVE_WASMEDGE,1,[Define if wasmedge was found])
        ],
        [
          ac_cv_enable_wasmedge=no
          AC_MSG_ERROR([Unable to find wasmedge runtime])
        ]
      )
    ])

  AM_CONDITIONAL(HAVE_WASMEDGE, [test "x${ac_cv_enable_wasmedge}" != xno])
])


AC_DEFUN([AX_CHECK_WASMER], [
  AC_ARG_WITH(
    [wasmer],
    AS_HELP_STRING([--with-wasmer],[compile with wasmer support (default is enabled)]),
    [],
    [ac_cv_enable_wasmer=no]
  )
  AS_IF(
    [test "x${ac_cv_enable_wasmer}" != xno],
    [
      AS_IF(
        [test "x${WASMER_HOME}" != x],
        [
          ax_wasmer_progs="wasmer"
          AC_CHECK_PROGS([WASMER], [$ax_wasmer_progs], [Unknown], [${WASMER_HOME}/bin])
        ],
        [
          ax_wasmer_progs="wasmer"
          AC_CHECK_PROGS(
            [WASMER],
            [$ax_wasmer_progs],
            [Unknown])
          WASMER_HOME=$(readlink -f $(dirname $(dirname $(which $WASMER))))
        ]
      )
      
      AS_IF(
        [test "x${WASMER}" != x],
        [
          WASMER_CFLAGS="-I${WASMER_HOME}/include"
          WASMER_LDFLAGS="-L${WASMER_HOME}/lib"
          WASMER_LIBS="-lwasmer"
          AC_SUBST([WASMER_HOME])
          AC_SUBST([WASMER])
          AC_SUBST([WASMER_CFLAGS])
          AC_SUBST([WASMER_LDFLAGS])
          AC_SUBST([WASMER_LIBS])
          AC_DEFINE(HAVE_WASMER,1,[Define if wasmer was found])
        ],
        [
          ac_cv_enable_wasmer=no
          AC_MSG_ERROR([Unable to find wasmer runtime])
        ]
      )
    ])

  AM_CONDITIONAL(HAVE_WASMER, [test "x${ac_cv_enable_wasmer}" != xno])
])


AC_DEFUN([AX_CHECK_WASMTIME], [
  AC_ARG_WITH(
    [wasmtime],
    AS_HELP_STRING([--with-wasmtime],[compile with wasmtime support (default is enabled)]),
    [],
    [ac_cv_enable_wasmtime=no]
  )
  AS_IF(
    [test "x${ac_cv_enable_wasmtime}" != xno],
    [
      AS_IF(
        [test "x${WASMTIME_HOME}" != x],
        [
          ax_wasmtime_progs="wasmtime"
          AC_CHECK_PROGS([WASMTIME], [$ax_wasmtime_progs], [Unknown], [${WASMTIME_HOME}/bin])
        ],
        [
          ax_wasmtime_progs="wasmtime"
          AC_CHECK_PROGS(
            [WASMTIME],
            [$ax_wasmtime_progs],
            [Unknown])
          WASMTIME_HOME=$(readlink -f $(dirname $(dirname $(which $WASMTIME))))
        ]
      )

      AS_IF(
        [test "x${WASMTIME}" != x],
        [
          WASMTIME_CFLAGS="-I${WASMTIME_HOME}/include"
          WASMTIME_LDFLAGS="-L${WASMTIME_HOME}/lib -lpthread -ldl -lm"
          WASMTIME_LIBS="-lwasmtime"
          AC_SUBST([WAMR_HOME])
          AC_SUBST([WASMTIME])
          AC_SUBST([WASMTIME_CFLAGS])
          AC_SUBST([WASMTIME_LDFLAGS])
          AC_SUBST([WASMTIME_LIBS])
          AC_DEFINE(HAVE_WASMTIME,1,[Define if wasmtime was found])
        ],
        [
          ac_cv_enable_wasmtime=no
          AC_MSG_ERROR([Unable to find wasmtime runtime])
        ]
      )
    ])

  AM_CONDITIONAL(HAVE_WASMTIME, [test "x${ac_cv_enable_wasmtime}" != xno])
])

AC_DEFUN([SB_WASM], [
  ac_cv_enable_wasm=no
  AX_CHECK_WAMR
  AX_CHECK_WASMEDGE
  AX_CHECK_WASMER
  AX_CHECK_WASMTIME
  AC_ARG_WITH(
    [wasm],
    AS_HELP_STRING([--without-wasm],[compile with wasmtime support (default is enabled)]),
    [],
    [ac_cv_enable_wasm=yes]
  )
  AC_DEFINE(HAVE_WASM,1,[Define if any wasm runtime was found])
  AM_CONDITIONAL(HAVE_WASM, [test "x${ac_cv_enable_wasm}" != xno])
])
