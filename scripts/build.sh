#!/bin/sh
# Developer build script.
#
# FIXES A DEFECT (FR-020): the previous version ran
#
#     ./configure --without-mysql --with-wamr --with-wasmedge --with-debug
#
# omitting --with-wasm. Because m4/sb_wasm.m4:227-234 gated the per-runtime
# checks behind that master flag, --with-wamr and --with-wasmedge were silently
# ignored and this script produced a binary with NO WASM SUPPORT AT ALL.
#
# The Meson option model makes that state unexpressible: enabling a runtime while
# the wasm gate is disabled is a configuration ERROR, not a silent no-op
# (contract C-5). It also removed the `make clean` that forced a full rebuild on
# every invocation.

set -eu

BUILDDIR="${BUILDDIR:-builddir}"

meson setup "$BUILDDIR" \
    -Dmysql=disabled \
    -Dwasm=enabled \
    -Dwamr=enabled \
    -Dwasmedge=enabled \
    -Dbuildtype=debug \
    "$@"

meson compile -C "$BUILDDIR"

echo
echo "Built: $BUILDDIR/src/sysbench"
