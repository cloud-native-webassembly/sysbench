# Build System Overview

**Title**: Build System Overview
**Purpose**: Explain how sysbench is built, which build system is authoritative, and the state of
the in-progress Autotools → Meson migration.
**Status**: Draft — migration framework round; Autotools remains authoritative
**Last updated**: 2026-08-02
**Related**: [dependencies.md](./dependencies.md) · [migration-status.md](./migration-status.md) ·
[requirements spec](../../.specify/specs/001-modernize-build-system/requirements.md)

## Which build system is authoritative

**GNU Autotools is still authoritative.** The Meson files in this tree are scaffolding from an
in-progress migration (Feature 019) and are **not yet verified to build**. Do not rely on them for
releases, packaging, or CI until [migration-status.md](./migration-status.md) records gate G-2 as
passed.

This is required by FR-018: Autotools may not be removed until all seven downstream consumers are
migrated and verified.

## Building with Autotools (current)

```shell
sh autogen.sh
./configure [options]
make -j"$(nproc)"
```

### Known blockers

The tree **does not build as shipped**. Two independent defects, both measured 2026-08-02:

1. **`autogen.sh` fails.** `configure.ac:31` passes `-Werror` to automake, and
   `src/wasm/Makefile.am:41` emits `warning: '%'-style pattern rules are a GNU make extension`.
   `-Werror` escalates it to fatal and `autoreconf` exits 1.
2. **`./configure` fails** with `conditional "HAVE_WAMR" was never defined`. The four per-runtime
   `AM_CONDITIONAL`s live inside `AX_CHECK_WAMR`/`WASMEDGE`/`WASMER`/`WASMTIME`, which `SB_WASM`
   (`m4/sb_wasm.m4:227-234`) invokes only when `--with-wasm` is passed. automake requires
   `AM_CONDITIONAL` to be unconditional, so the **default** configure path is broken.

Both are fixed by the migration rather than patched in place, since the Autotools layer is slated
for deletion.

## Building with Meson (scaffolding, not yet verified)

```shell
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

WASM-enabled configuration:

```shell
meson setup builddir-wasm -Dwasm=enabled -Dwamr=enabled -Dwasmedge=enabled
meson compile -C builddir-wasm
```

These commands are **specified, not verified** — `meson` and `ninja` are absent from the current
image. See [migration-status.md](./migration-status.md) for what unblocks verification.

## Why migrate

The justification rests on **readability** and **correctness**, not raw speed.

| Dimension | Measured |
|---|---|
| Total Autotools lines | 5,074 |
| — project-authored (the porting surface) | **1,905** |
| — vendored third-party `m4` (deleted, not ported) | **3,169** (62%, 13 files) |
| Target after migration | ≤762 project-authored lines (SC-004) |

**The build is not slow.** Measured medians on a 192-core x86_64 machine, warm cache: full clean
build **6 s**, no-op rebuild **55 ms**, single-C-file rebuild **178 ms** (exactly one compile).
Success criteria SC-001 and SC-002 were consequently renegotiated from aggressive speedup ratios
into non-regression criteria — the 178 ms figure is dominated by a single compiler-bound `gcc`
invocation, not by build-system overhead.

**The real defect is silent staleness.** `third_party/luajit/Makefile.am:25` declares its rule as
`$(builddir)/lib/libluajit-5.1.a:` with **no prerequisites**. Measured: touching
`third_party/luajit/luajit/src/lj_api.c` produced **zero** compiles and left the archive's mtime
unchanged — a LuaJIT source edit does not reach the binary. That is a correctness bug, and it is
the strongest argument for the migration.

Two further correctness defects the migration closes:

- **Silently-ignored options.** `--with-wamr` without `--with-wasm` is discarded without warning.
  `scripts/build.sh:2` omits the master gate, so that script produces a WASM-less binary. The
  Meson option model makes this state unexpressible.
- **Dead WASM guest rules.** `src/wasm/Makefile.am:41-45` uses space indentation where Make
  requires tabs, so the seven `.wasm` guest modules are **not built by WASI-SDK today**.

## How dependencies are found

Dependency discovery lives in project-authored Meson code, not in vendored macros. See
[dependencies.md](./dependencies.md) for each dependency's acquisition path and discovery order.
