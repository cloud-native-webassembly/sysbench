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
# Macro: SB_PYTHON
# ---------------------------------------------------------------------------
# modified https://github.com/libyal/libsmraw/blob/main/m4/python.m4

AC_DEFUN([AX_PROG_PYTHON], [
    ax_python_progs="python3";
    ax_python_config="python3-config"
    AS_IF(
      [test "x${PYTHON_HOME}" != x],
      [
        AC_CHECK_PROGS([PYTHON], [$ax_python_progs], [Unknown], [${PYTHON_HOME}/bin])
        AC_CHECK_PROGS([PYTHON_CONFIG], [$ax_python_config], [Unknown], [${PYTHON_HOME}/bin])
      ],
      [
        AC_CHECK_PROGS([PYTHON], [$ax_python_progs], [Unknown])
        PYTHON_HOME=$(readlink -f $(dirname $(dirname $(which $PYTHON))))
        AC_CHECK_PROGS([PYTHON_CONFIG], [$ax_python_config], [Unknown])
      ]
    )
    
  AC_SUBST([PYTHON_HOME])
  AC_SUBST([PYTHON], [$ax_python_progs])
  AC_SUBST([PYTHON_CONFIG], [$ax_python_config])]
)

AC_DEFUN([AX_PYTHON_CHECK], [
  AX_PROG_PYTHON
  ax_prog_python_version=`${PYTHON} -c "import sys; sys.stdout.write('%d.%d' % (sys.version_info[[0]], sys.version_info[[1]]))" 2>/dev/null`;
  AS_IF([test "x${PYTHON_CONFIG}" != x], [
    PYTHON_INCLUDES=`${PYTHON_CONFIG} --includes 2>/dev/null`;
    AC_SUBST([PYTHON_INCLUDES])

    PYTHON_LIBS="-L`${PYTHON_CONFIG} --configdir` -lpython${ax_prog_python_version}"
    AC_SUBST([PYTHON_LIBS])

    PYTHON_CFLAGS=`${PYTHON_CONFIG} --cflags 2>/dev/null`;
    AC_SUBST([PYTHON_CFLAGS])

    dnl Check for Python libraries
    PYTHON_LDFLAGS=`${PYTHON_CONFIG} --ldflags 2>/dev/null`;
    AC_SUBST([PYTHON_LDFLAGS])

    dnl For CygWin add the -no-undefined linker flag
    AS_CASE(
      [$host_os],
      [cygwin*],[PYTHON_LDFLAGS="${PYTHON_LDFLAGS} -no-undefined"],
      [*],[])

    dnl Check for the existence of Python.h
    BACKUP_CPPFLAGS="${CPPFLAGS}"
    CPPFLAGS="${CPPFLAGS} ${PYTHON_INCLUDES}"

    AC_CHECK_HEADERS(
      [Python.h],
      [ac_cv_header_python_h=yes],
      [ac_cv_header_python_h=no])

    CPPFLAGS="${BACKUP_CPPFLAGS}"
  ])

  AS_IF([test "x${ac_cv_header_python_h}" != xyes],
    [ac_cv_enable_python=no],
    [ac_cv_enable_python=${ax_prog_python_version}
    
    dnl Check for Python prefix
    AS_IF(
      [test "x${ac_cv_with_pyprefix}" = x || test "x${ac_cv_with_pyprefix}" = xno],
      [ax_python_prefix="\${prefix}"],
      [ax_python_prefix=`${PYTHON_CONFIG} --prefix 2>/dev/null`])

    AC_SUBST(
      [PYTHON_PREFIX],
      [$ax_python_prefix])

    dnl Check for Python exec-prefix
    AS_IF(
      [test "x${ac_cv_with_pyprefix}" = x || test "x${ac_cv_with_pyprefix}" = xno],
      [ax_python_exec_prefix="\${exec_prefix}"],
      [ax_python_exec_prefix=`${PYTHON_CONFIG} --exec-prefix 2>/dev/null`])

    AC_SUBST(
      [PYTHON_EXEC_PREFIX],
      [$ax_python_exec_prefix])

    dnl Check for Python library directory
    ax_python_pythondir_suffix=`${PYTHON} -c "import sys; import distutils.sysconfig; sys.stdout.write(distutils.sysconfig.get_python_lib(0, 0, prefix=''))" 2>/dev/null`;

    AS_IF(
      [test "x${ac_cv_with_pythondir}" = x || test "x${ac_cv_with_pythondir}" = xno],
      [AS_IF(
        [test "x${ac_cv_with_pyprefix}" = x || test "x${ac_cv_with_pyprefix}" = xno],
        [ax_python_pythondir="${ax_python_prefix}/${ax_python_pythondir_suffix}"],
        [ax_python_pythondir=`${PYTHON} -c "import sys; import distutils.sysconfig; sys.stdout.write(distutils.sysconfig.get_python_lib()) " 2>/dev/null`])],
      [ax_python_pythondir=$ac_cv_with_pythondir])

    AC_SUBST(
      [pythondir],
      [$ax_python_pythondir])

    dnl Check for Python platform specific library directory
    ax_python_pyexecdir_suffix=`${PYTHON} -c "import sys; import distutils.sysconfig; sys.stdout.write(distutils.sysconfig.get_python_lib(1, 0, prefix=''))" 2>/dev/null`;
    ax_python_library_dir=`${PYTHON} -c "import sys; import distutils.sysconfig; sys.stdout.write(distutils.sysconfig.get_python_lib(True)) " 2>/dev/null`;

    AS_IF(
      [test "x${ac_cv_with_pyprefix}" = x || test "x${ac_cv_with_pyprefix}" = xno],
      [ax_python_pyexecdir="${ax_python_exec_prefix}/${ax_python_pyexecdir_suffix}"],
      [ax_python_pyexecdir=$ax_python_library_dir])

    AC_SUBST(
      [pyexecdir],
      [$ax_python_pyexecdir])

    AC_SUBST(
      [PYTHON_LIBRARY_DIR],
      [$ax_python_pyexecdir_suffix])

    AC_SUBST(
      [PYTHON_PACKAGE_DIR],
      [$ax_python_library_dir])
    ])

    AC_DEFINE(HAVE_PYTHON,1,[Define if python was found])
  ])

AC_DEFUN([AX_PYTHON_CHECK_ENABLE], [
  AC_ARG_WITH([python],
              AS_HELP_STRING([--with-python],
                            [compile with python support (default is enabled)]),
              [], [ac_cv_enable_python=yes])
  AS_IF(
    [test "x${ac_cv_enable_python}" != xno],
    [AX_PYTHON_CHECK])

  AM_CONDITIONAL(HAVE_PYTHON, [test "x${ac_cv_enable_python}" != xno])]
)

AC_DEFUN([SB_PYTHON], [AX_PYTHON_CHECK_ENABLE])
