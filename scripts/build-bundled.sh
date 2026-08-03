#!/bin/sh
# Build a bundled dependency out-of-tree and place its archive at a known path.
#
# Replaces the tar-copy recipes in third_party/luajit/Makefile.am:26-35 and
# third_party/concurrency_kit/Makefile.am:22-34.
#
# Two behaviours differ deliberately from the Autotools originals:
#   1. The source tree is NEVER written to. The old LuaJIT rule ran
#      `$(MAKE) -C $(srcdir)/luajit clean`, mutating vendored sources
#      (BD-1, contract C-12, INV-3).
#   2. No unconditional `clean`. Meson's depend_files decides when to rebuild,
#      so cleaning here would defeat incrementality (BD-2, contract C-11).
#
# Usage: build-bundled.sh <luajit|ck> <source-dir> <work-dir> <output-archive>

set -eu

WHICH="$1"
SRC="$2"
WORK="$3"
OUT="$4"

if [ ! -d "$SRC" ]; then
    echo "build-bundled.sh: source directory not found: $SRC" >&2
    exit 1
fi

# Validate before copying — an unknown name must not trigger a tree copy.
case "$WHICH" in
luajit|ck) ;;
*)
    echo "build-bundled.sh: unknown dependency '$WHICH' (expected luajit|ck)" >&2
    exit 1
    ;;
esac

# Neither dependency supports VPATH builds, so copy into the build tree first.
# The copy is what keeps the source tree pristine.
rm -rf "$WORK"
mkdir -p "$WORK"
cp -R "$SRC/." "$WORK/"
chmod -R u+w "$WORK"

PREFIX="$WORK/_install"
mkdir -p "$PREFIX"

case "$WHICH" in
luajit)
    # LuaJIT ships a plain Makefile with no configure step.
    ${MAKE:-make} -C "$WORK" \
        PREFIX="$PREFIX" \
        INSTALL_INC="$PREFIX/include" \
        install
    ARCHIVE="$PREFIX/lib/libluajit-5.1.a"
    ;;
ck)
    # Concurrency Kit ships its own configure script.
    ( cd "$WORK" && \
      CC="${CC:-cc}" \
      CFLAGS="${CFLAGS:-}" \
      LDFLAGS="${LDFLAGS:-}" \
      ./configure --prefix="$PREFIX" ${CK_CONFIGURE_FLAGS:-} )
    ${MAKE:-make} -C "$WORK"
    ${MAKE:-make} -C "$WORK" install
    ARCHIVE="$PREFIX/lib/libck.a"
    ;;
esac

if [ ! -f "$ARCHIVE" ]; then
    echo "build-bundled.sh: expected archive not produced: $ARCHIVE" >&2
    exit 1
fi

mkdir -p "$(dirname "$OUT")"
cp -f "$ARCHIVE" "$OUT"
