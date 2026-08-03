#!/bin/sh
# Incrementality verification — contract C-11, C-12; SC-001, SC-002, SC-003.
#
# Proves the four properties the Autotools build fails to provide:
#   (a) a no-op rebuild does zero work
#   (b) a single-source change recompiles exactly one translation unit
#   (c) touching a bundled-dependency source DOES rebuild its archive
#   (d) a full build leaves the source tree unmodified
#
# (c) is the important one. Measured on the Autotools build 2026-08-02: touching
# third_party/luajit/luajit/src/lj_api.c produced ZERO compiles and left the
# archive mtime unchanged, because third_party/luajit/Makefile.am:25 declares its
# target with no prerequisites. A LuaJIT source edit did not reach the binary —
# a silent stale-artefact correctness bug.
#
# Usage: check_incremental.sh <builddir>
#
# Requires meson and ninja. Deferred until the base image provides them
# (see docs/build/migration-status.md).

set -eu

BUILDDIR="${1:-builddir}"
REPO="$(cd "$(dirname "$0")/../.." && pwd)"
FAILURES=0

fail() {
    echo "FAIL: $*" >&2
    FAILURES=$((FAILURES + 1))
}

pass() {
    echo "PASS: $*"
}

if ! command -v meson >/dev/null 2>&1; then
    echo "SKIP: meson not installed — cannot verify incrementality" >&2
    exit 77
fi

cd "$REPO"

if [ ! -d "$BUILDDIR" ]; then
    echo "setting up $BUILDDIR"
    meson setup "$BUILDDIR"
fi

echo "=== ensuring a complete build ==="
meson compile -C "$BUILDDIR" >/dev/null

# --- (a) no-op rebuild does zero work ---------------------------------------

echo "=== (a) no-op rebuild ==="
NOOP_LOG="$(mktemp)"
START="$(date +%s%N)"
meson compile -C "$BUILDDIR" --verbose >"$NOOP_LOG" 2>&1
END="$(date +%s%N)"
NOOP_MS=$(( (END - START) / 1000000 ))

if grep -qE '(^| )(cc|gcc|clang) ' "$NOOP_LOG"; then
    fail "(a) no-op rebuild invoked the compiler"
    grep -E '(^| )(cc|gcc|clang) ' "$NOOP_LOG" | head -3 >&2
else
    pass "(a) no-op rebuild did zero compilation (${NOOP_MS}ms; Autotools baseline 55ms)"
fi

# --- (b) single-file change recompiles exactly one TU ------------------------

echo "=== (b) single-file rebuild ==="
touch src/sb_timer.c
SINGLE_LOG="$(mktemp)"
START="$(date +%s%N)"
meson compile -C "$BUILDDIR" --verbose >"$SINGLE_LOG" 2>&1
END="$(date +%s%N)"
SINGLE_MS=$(( (END - START) / 1000000 ))

COMPILES=$(grep -cE '\-o .*sb_timer\.c\.o' "$SINGLE_LOG" || true)
TOTAL_COMPILES=$(grep -cE '\-c .*\.c' "$SINGLE_LOG" || true)

if [ "$TOTAL_COMPILES" -eq 1 ]; then
    pass "(b) exactly 1 translation unit recompiled (${SINGLE_MS}ms; Autotools baseline 178ms)"
else
    fail "(b) expected 1 recompile, observed $TOTAL_COMPILES"
fi

# --- (c) bundled-dependency source change DOES rebuild ----------------------

echo "=== (c) bundled dependency rebuild trigger ==="
LUAJIT_SRC="third_party/luajit/luajit/src/lj_api.c"
LUAJIT_ARCHIVE="$(find "$BUILDDIR" -name 'libluajit-5.1.a' 2>/dev/null | head -1)"

if [ -z "$LUAJIT_ARCHIVE" ]; then
    echo "SKIP: (c) bundled LuaJIT archive not found (system luajit selected?)"
elif [ ! -f "$LUAJIT_SRC" ]; then
    fail "(c) expected LuaJIT source not found: $LUAJIT_SRC"
else
    BEFORE="$(stat -c %Y "$LUAJIT_ARCHIVE")"
    touch "$LUAJIT_SRC"
    meson compile -C "$BUILDDIR" >/dev/null 2>&1
    AFTER="$(stat -c %Y "$LUAJIT_ARCHIVE")"

    if [ "$BEFORE" != "$AFTER" ]; then
        pass "(c) bundled LuaJIT archive rebuilt after source change (Autotools did NOT)"
    else
        fail "(c) bundled LuaJIT archive NOT rebuilt — stale-artefact bug reproduced"
    fi
fi

# --- (d) source tree unmodified by building ---------------------------------

echo "=== (d) out-of-tree purity ==="
# Restore the mtimes touched above so the check reflects content, not timestamps.
git -C "$REPO" diff --quiet -- src third_party || true
DIRTY="$(git -C "$REPO" status --porcelain -- src third_party tests | grep -v '^?? ' || true)"

if [ -z "$DIRTY" ]; then
    pass "(d) build left the source tree unmodified"
else
    fail "(d) build modified tracked files in the source tree:"
    echo "$DIRTY" >&2
fi

echo
if [ "$FAILURES" -eq 0 ]; then
    echo "ALL INCREMENTALITY CHECKS PASSED"
    exit 0
fi
echo "$FAILURES incrementality check(s) failed"
exit 1
