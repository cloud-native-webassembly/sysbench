# Data Model: Modernize Build System

**Requirement**: `001-modernize-build-system` → Feature 019 Modern Build System  
**Date**: 2026-08-01  
**Source**: `requirements.md` § Key Entities, resolved against measured build-system state

This feature has no runtime data model — it changes how the project is built, not what it
computes. The entities below are **build-time configuration entities**: the things the build
system reasons about, validates, and reports. They define the state a `meson setup` invocation
produces and the rules that govern it.

## Entity 1: Build Configuration

The complete set of choices for one configured build directory.

| Field | Type | Constraint |
|---|---|---|
| `build_dir` | path | Must differ from source root (FR-009) |
| `build_type` | enum | `debug` \| `release` \| `debugoptimized`; default `release` |
| `c_std` | fixed | `c99` — pinned, resolving the current `configure.ac:45-47` ambiguity |
| `sanitizer` | enum | `none` \| `address` \| `memory`; mutually exclusive |
| `coverage` | bool | Mutually exclusive with `sanitizer` |
| `werror` | bool | Default false (mirrors current `--disable-fail` default) |
| `components` | set of Optional Component | See Entity 2 |
| `platform` | derived | Linux x86_64/aarch64 or macOS (FR-019) |
| `version_metadata` | string | Package version + git SHA; empty SHA when git absent (FR-005) |

**Invariant**: a Build Configuration must be fully reportable — every field's effective value is
printed at configure time (FR-012, contract C-3).

## Entity 2: Optional Component

A feature compiled in only when both *requested* and *satisfiable*. This is the entity whose
mishandling causes the defect FR-013 exists to eliminate.

| Field | Type | Notes |
|---|---|---|
| `name` | string | e.g. `mysql`, `wamr`, `python` |
| `request_state` | enum | `enabled` \| `disabled` \| `auto` |
| `satisfiable` | bool | Result of dependency probing |
| `resolution` | enum | `compiled-in` \| `skipped` |
| `reason` | string | Required when `resolution = skipped` |
| `parent` | ref \| null | Non-null for the four WASM runtimes (parent = `wasm`) |

### State machine

```
                  ┌──────────────────────────────────────────┐
request=disabled → │ skipped (reason: "not requested")        │
                  └──────────────────────────────────────────┘
request=auto ────→ probe ──satisfiable──→ compiled-in
                         └─unsatisfiable─→ skipped (reason: "<dep> not found")
request=enabled ─→ probe ──satisfiable──→ compiled-in
                         └─unsatisfiable─→ ERROR — configuration fails (C-4)
```

**The `auto`/`enabled` distinction is the whole point.** Under Autotools, requesting a component
whose dependency was missing either aborted configure (`AC_MSG_ERROR`) or — worse, in the
master-gate case — was silently ignored. The tri-state makes "try it" and "I require it"
different, expressible intents.

### Components (12, matching the 12 `AM_CONDITIONAL`s)

| Component | Parent | Dependency probed |
|---|---|---|
| `mysql` | — | `mysql_config` / libmysqlclient |
| `pgsql` | — | `pg_config` / libpq |
| `python` | — | python3 embed via pkg-config |
| `aio` | — | `libaio.h` + `io_queue_init` (Linux only) |
| `largefile` | — | Compiler/system support |
| `wasm` | — | Master gate for the four runtimes |
| `wamr` | `wasm` | `wasm_export.h` + `-liwasm` |
| `wasmedge` | `wasm` | `wasmedge/wasmedge.h` + `-lwasmedge` |
| `wasmer` | `wasm` | `wasmer.h` + `-lwasmer` |
| `wasmtime` | `wasm` | `wasmtime.h` + `-lwasmtime` |
| `system-luajit` | — | pkg-config `luajit` (else bundled) |
| `system-ck` | — | pkg-config `ck` (else bundled) |

## Entity 3: Bundled Dependency

A dependency vendored in-tree, buildable from source, statically linked.

| Field | Type | Value |
|---|---|---|
| `name` | string | `luajit` \| `concurrency_kit` |
| `source_dir` | path | `third_party/<name>/` (read-only during build) |
| `artifact` | path | `libluajit-5.1.a` \| `libck.a` |
| `link_mode` | fixed | static |
| `source_selectable` | bool | true — bundled or system |
| `version` | string | LuaJIT 2.1.0-beta3; CK as vendored |
| `foreign_build` | enum | `plain-make` (LuaJIT) \| `own-configure` (CK) |

**Validation rules**:
- **BD-1**: The build MUST NOT write into `source_dir` (FR-009). The current
  `third_party/luajit/Makefile.am:27` violates this by running `make clean` in `$(srcdir)`.
- **BD-2**: The artifact MUST be rebuilt if and only if a file under `source_dir` changed
  (FR-006, FR-007) — enforced by `depend_files`.

## Entity 4: External SDK Dependency

A dependency that must pre-exist on the machine; discovered at configure time.

| Field | Type | Notes |
|---|---|---|
| `name` | string | `wamr`, `wasmedge`, `wasmer`, `wasmtime`, `wasi-sdk`, `mysql`, `pgsql`, `python3` |
| `discovery` | enum | `pkg-config` \| `env-var` \| `explicit-path` |
| `home_var` | string \| null | e.g. `WAMR_HOME`, `WASI_SDK_HOME` |
| `link_mode` | enum | dynamic (WASM runtimes, DB clients) |
| `version` | string \| null | Recorded when the SDK exposes it |

**Validation rule VD-1**: discovery MUST verify the header *and* the library, not merely the
presence of a CLI binary. The current `m4/sb_wasm.m4` infers usability from finding `iwasm` /
`wasmedge` / `wasmer` / `wasmtime` on `PATH`, which wrongly accepts a runtime CLI shipped without
development files.

## Entity 5: Generated Source

A build-produced input to compilation.

| Field | Type | Notes |
|---|---|---|
| `kind` | enum | `embedded-header` \| `wasm-guest` \| `config-header` \| `substituted-file` |
| `inputs` | list of path | Declared for dependency tracking |
| `output` | path | Always under `build_dir` |
| `generator` | string | `embed-file.py` \| WASI-SDK clang \| `configuration_data` |

### Instances

| Kind | Count | Inputs → Output |
|---|---|---|
| `embedded-header` | 6 | 5 `.lua` + 1 `.py` → `<name>.lua.h` / `.py.h` |
| `wasm-guest` | 7 | `src/wasm/*.c` → `.wasm` modules |
| `config-header` | 1 | Feature probes → `config.h` |
| `substituted-file` | 2 | `tests/include/config.sh.in`, `snap/snapcraft.yaml.in` |

**Embedded-header output contract** (byte-identical to current behaviour):

```c
unsigned char <var>[] =
  "<escaped line>\n"
  ...
;
size_t <var>_len = sizeof(<var>) - 1;
```

where `<var>` is the input filename with `.` → `_`. Escaping: `\` → `\\`, then `"` → `\"`.

**Substitution variables**: `@PACKAGE_VERSION@`, `@SB_GIT_SHA@`, `@USE_MYSQL@`, `@USE_PGSQL@`
(config.sh); `@PACKAGE_VERSION@` (snapcraft.yaml).

## Entity 6: Build Target

A compiled artefact.

| Field | Type | Notes |
|---|---|---|
| `name` | string | Target identifier |
| `kind` | enum | `executable` \| `static_library` \| `custom` |
| `install_dir` | path \| null | null = not installed |

### Instances (16)

| Target | Kind | Install | Source of truth |
|---|---|---|---|
| `sysbench` | executable | `bindir` | `src/Makefile.am:22` |
| `libsbcpu`, `libsbfileio`, `libsbmemory`, `libsbthreads`, `libsbmutex` | static | none | `src/tests/*/Makefile.am:18` |
| `libsbmysql`, `libsbpgsql` | static | none | `src/drivers/*/Makefile.am:18` |
| 7 `.wasm` guests | custom | **none** — corrected | `src/wasm/Makefile.am:24-31` wrongly used `bin_PROGRAMS` |

**Validation rule BT-1**: WASM guest modules MUST NOT install to `bindir` (FR-020). They are
benchmark inputs, not host executables.

## Entity 7: Build Consumer

An automation or packaging entry point that invokes the build.

| Field | Type | Notes |
|---|---|---|
| `name` | string | Consumer identifier |
| `file` | path | Where the invocation lives |
| `migration_state` | enum | `autotools` \| `migrated` \| `verified` |

### Instances (7 + docs)

| Consumer | File | Current invocation |
|---|---|---|
| CI | `.github/workflows/ci.yml:15,17,41,43` | `./autogen.sh` + `./configure` |
| Legacy CI | `.travis.yml:219,224,229` | `./autogen.sh && ./configure` |
| Container | `Dockerfile:16,17,19` | `autogen.sh`, `configure`, `make install` |
| Debian | `debian/rules:8,9` | `autoreconf -vif`, `dh_auto_configure` |
| RPM | `rpm/sysbench.spec:52,53-55` | `autoreconf -vif`, `%configure` |
| snap | `snap/snapcraft.yaml.in:31` | `plugin: autotools` |
| Dev script | `scripts/build.sh:1,2` | `autogen.sh`, `configure` (**omits `--with-wasm`**) |
| Docs | `README.md:238,240,242` | Documented commands |

**Validation rule BC-1**: `migration_state` must reach `verified` for all 7 before Autotools
files may be deleted (FR-018, gate G-6).

## Cross-Entity Invariants

- **INV-1 (parent gating)**: a child component cannot be `compiled-in` while its parent is
  `skipped`. Requesting a child MUST either imply the parent or fail with a clear message —
  never be silently ignored (FR-013). This is the exact defect at `m4/sb_wasm.m4:227-234`.
- **INV-2 (capability parity)**: every one of the 22 current options maps to exactly one Meson
  option or a recorded drop rationale (FR-002, `contracts/option-mapping.md`).
- **INV-3 (out-of-tree purity)**: no entity writes into the source tree during a build (FR-009).
- **INV-4 (reportability)**: every Optional Component's `resolution` and, when skipped, `reason`
  appears in configure output (FR-012).
- **INV-5 (baseline precedence)**: performance claims about a Build Configuration are only valid
  against a captured baseline (Constitution Principle VI; contract C-14).
