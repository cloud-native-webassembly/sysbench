# Build Dependencies

**Title**: Build Dependencies
**Purpose**: Record how every build dependency is obtained and how the build discovers it, so no
contributor needs to read build-system internals to answer "where does this come from?"
**Status**: Draft — migration framework round
**Last updated**: 2026-08-02
**Related**: [overview.md](./overview.md) · [migration-status.md](./migration-status.md)

## How discovery works

Every dependency is located by project-authored Meson code — `meson.build`, `src/meson.build`,
`src/drivers/meson.build`, and `third_party/meson.build`. No vendored macro code is involved. The
Autotools build needed 13 vendored `m4` files (3,169 lines) for the same job; all of them are
deleted rather than ported.

Discovery order, applied uniformly:

1. **Explicit path option** (e.g. `-Dmysql-includes=`, `-Dwasi-sdk-home=`)
2. **`<NAME>_HOME` environment variable** (WASM SDKs, WASI-SDK)
3. **pkg-config**
4. **Vendor helper** (`mysql_config`, `pg_config`)

Discovery verifies the **header and the library**, never merely the presence of a CLI binary
(contract C-6, VD-1). This is a deliberate change: `m4/sb_wasm.m4` inferred usability from finding
`iwasm`/`wasmedge`/`wasmer`/`wasmtime` on `PATH`, which wrongly accepts a runtime shipped without
development files.

## Bundled dependencies (in-tree)

Both are vendored under `third_party/`, built from source, and linked **statically**. Each can
instead be taken from the system.

| Dependency | Version | Source | Option | Archive |
|---|---|---|---|---|
| LuaJIT | 2.1.0-beta3 | `third_party/luajit/luajit/` | `-Dluajit=bundled\|system` | `libluajit-5.1.a` |
| Concurrency Kit | as vendored | `third_party/ck/` | `-Dconcurrency-kit=bundled\|system` | `libck.a` |

Neither supports VPATH builds, so `scripts/build-bundled.sh` copies the tree into the build
directory before building. Two behaviours differ deliberately from the Autotools recipes:

- **The source tree is never written to.** The old rule ran
  `$(MAKE) -C $(srcdir)/luajit clean`, mutating vendored sources (BD-1, contract C-12).
- **No unconditional clean.** Meson's `depend_files` decides when to rebuild. The old rule declared
  its target with *no prerequisites*, so the archive was never rebuilt once present — measured:
  touching `third_party/luajit/luajit/src/lj_api.c` produced zero compiles and left the archive
  mtime unchanged. A source edit did not reach the binary.

Choosing `system` requires the library to be discoverable via pkg-config (`luajit`, `ck`).

## External SDK dependencies (must pre-exist)

None of these is vendored, and **none is version-pinned**. Pinning is tracked separately as
Feature 016 (Pinned SDK Provisioning).

| Dependency | Needed for | Discovery | Link mode |
|---|---|---|---|
| WASI-SDK | Building the 7 `src/wasm/` guest modules | `-Dwasi-sdk-home=` → `$WASI_SDK_HOME` | n/a (cross-compiler) |
| WAMR | `-Dwamr=enabled` | `wasm_export.h` + `libiwasm` | dynamic |
| WasmEdge | `-Dwasmedge=enabled` | `wasmedge/wasmedge.h` + `libwasmedge` | dynamic |
| Wasmer | `-Dwasmer=enabled` | `wasmer.h` + `libwasmer` | dynamic |
| Wasmtime | `-Dwasmtime=enabled` | `wasmtime.h` + `libwasmtime` | dynamic |
| MySQL client | `-Dmysql=enabled` | pkg-config `mysqlclient` → `mysql_config` | dynamic |
| PostgreSQL client | `-Dpgsql=enabled` | pkg-config `libpq` → `pg_config` | dynamic |
| Python 3 | `-Dpython=enabled` | pkg-config `python3-embed` | dynamic |

### Requesting something unavailable

The tri-state `feature` options make intent explicit:

- `-Dwamr=enabled` — **required**. If the SDK is incomplete, configuration **fails** naming both
  the component and what was missing.
- `-Dwamr=auto` — use it if available, skip with a recorded reason otherwise.
- `-Dwamr=disabled` — do not use it.

An explicit request is **never silently ignored** (contract C-4, FR-013). Under Autotools,
`--with-wamr` without `--with-wasm` was discarded without warning — the defect that made
`scripts/build.sh` produce a WASM-less binary.

## Toolchain

| Tool | Requirement |
|---|---|
| C compiler | C99; GCC or Clang |
| Meson | ≥ 1.3 |
| Ninja | any recent version |
| pkg-config | for dependency discovery |
| Python 3 | for `scripts/embed-file.py` (embedded header generation) |

## Current environment gap

Verified absent from the current container image as of 2026-08-02: `meson`, `ninja`, WASI-SDK, all
four WASM runtime SDKs, `valgrind`, `debuild`, `rpmbuild`, `snapcraft`. The base image must be
rebuilt before the Meson build can be exercised — see
[migration-status.md](./migration-status.md).
