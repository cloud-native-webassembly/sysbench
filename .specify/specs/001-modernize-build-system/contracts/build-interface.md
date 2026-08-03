# Contract: Build System Interface

**Requirement**: `001-modernize-build-system` → Feature 019 Modern Build System  
**Date**: 2026-08-01  
**Scope**: The user-facing interface of the new build system — commands, options, diagnostics,
exit codes, and artefacts.

This feature exposes no network API. The contract surface is the **build system's interface**,
which is what the 7 downstream consumers and every contributor actually depend on.

## Verification status of examples

Meson is **not installed in the authoring environment**, so the `meson` command lines below are
specified, not execution-verified. Each is pinned by a contract that `/speckit.implement` must
verify against the real tool (contract C-16).

**One contract is fully execution-verified**: C-7 (embedded-header byte-compatibility). The
replacement script was run against all 6 real inputs and diffed against the current `sed` output
— see the Verification Record at the end.

---

## Configuration Contracts

### C-1 — Configure entry point
The build MUST configure via a single command taking a build directory distinct from the source
root:

```shell
meson setup builddir [options]
```

- MUST NOT require any preparatory code-generation step (no `autogen.sh`/`autoreconf` equivalent).
- MUST succeed on a clean checkout with no Autotools tooling installed (FR-001, SC-012).
- MUST fail with a non-zero exit and a clear message if `builddir` equals the source root
  (FR-009).

### C-2 — Option surface
All 22 current capabilities MUST be reachable as build options. Full mapping in
[`option-mapping.md`](./option-mapping.md) (FR-002, SC-005).

- Optional components MUST use a tri-state feature type (`enabled` / `disabled` / `auto`) so
  "try it" and "I require it" are distinct, expressible intents.
- Option names MAY differ from Autotools names; capabilities MUST NOT be dropped without a
  recorded rationale.

### C-3 — Configuration summary
Configuration MUST print a summary listing, for every optional component, its resolution
(`compiled-in` / `skipped`) and — when skipped — the reason (FR-012, INV-4).

The summary MUST also report: build type, C standard, sanitizer/coverage state, LuaJIT and
Concurrency Kit source (bundled/system), and the resolved version metadata.

### C-4 — Unsatisfiable request fails loudly
When a component is explicitly `enabled` but its dependency is unsatisfiable, configuration MUST
fail with a non-zero exit and a message naming **both** the component and the unmet dependency
(FR-013, SC-010).

Silently ignoring an explicit request is **prohibited**. This is the contract that eliminates the
`m4/sb_wasm.m4:227-234` defect class.

### C-5 — Parent gating is explicit
A child component (the four WASM runtimes, parent `wasm`) MUST NOT be silently ignored when its
parent is disabled (INV-1). Exactly one of:

- **(a)** requesting a child implies the parent; or
- **(b)** requesting a child with the parent disabled fails per C-4.

The chosen behaviour MUST be documented. Implementation SHOULD prefer (a): it makes the
current `scripts/build.sh:2` bug — which silently yields a WASM-less binary — impossible to
express.

### C-6 — Dependency discovery verifies headers and libraries
Discovery of an external SDK MUST verify the header **and** the library (VD-1). Presence of a
runtime CLI on `PATH` MUST NOT be treated as sufficient evidence, since a runtime CLI can ship
without development files.

Discovery order per SDK: explicit path option → `<NAME>_HOME` environment variable →
pkg-config → system default.

---

## Code Generation Contracts

### C-7 — Embedded headers are byte-identical *(execution-verified)*
Generated `.lua.h` / `.py.h` files MUST be byte-identical to the current `sed`-rule output
(FR-004).

Output format:

```c
unsigned char <var>[] =
  "<escaped line>\n"
  ...
;
size_t <var>_len = sizeof(<var>) - 1;
```

Normative rules:
- `<var>` is the input **filename** (not path) with every `.` replaced by `_`.
- Escaping order: `\` → `\\`, then `"` → `\"`.
- Each source line is wrapped as `  "` + escaped-line + `\n"`.
- **When the input does not end with a newline, the closing `;` appears on the same line as the
  final string literal.** This reproduces `sed` behaviour and is load-bearing:
  `src/python/sysbench.py` has no trailing newline (verified: last byte `0x29`).
- Inputs: 5 Lua files under `src/lua/internal/`, 1 Python file under `src/python/`.

### C-8 — WASM guest modules
The 7 guest modules MUST be compiled from `src/wasm/*.c` with the WASI-SDK toolchain
(FR-004):

- Compiler: `$WASI_SDK_HOME/bin/clang`
- Flags: `--target=wasm32-wasi --sysroot=$WASI_SDK_HOME/share/wasi-sysroot -fPIC
  -z stack-size=8192 -Wl,--initial-memory=65536 -Wl,--no-entry -Wl,--export=event`
- `pass_data` additionally requires `-Wl,--export=create_buffer`
- Outputs MUST NOT be installed to `bindir` (BT-1, FR-020)
- Guest builds MUST be gated on the `wasm` component being compiled-in

**Note**: the current rules at `src/wasm/Makefile.am:41-45` are space-indented rather than
tab-indented and are therefore dead — the guests are not built by WASI-SDK today. This contract
specifies the *intended* behaviour, not the current behaviour.

### C-9 — Config header
A `config.h` MUST be generated defining the results of: 17 header checks, 15 function checks,
`SIZEOF_SIZE_T`, `SIZEOF_BOOL`, `HAVE_DECL_SHM_HUGETLB`, `HAVE_DECL_O_SYNC`, the working TLS
keyword as `TLS`, `HAVE_FUNC_ATTRIBUTE_FORMAT`, `HAVE_FUNC_ATTRIBUTE_UNUSED`, and the
component-enablement macros (`USE_MYSQL`, `USE_PGSQL`, `HAVE_WAMR`, `HAVE_WASMEDGE`,
`HAVE_WASMER`, `HAVE_WASMTIME`, `HAVE_PYTHON`, `SB_WITH_LUAJIT`, `SB_WITH_CK`, `SB_GIT_SHA`,
`HAVE_LARGE_PAGES`, `HUGETLB_USE_PROC_MEMINFO`, `HAVE_LIBAIO_H`, `HAVE_OLD_GETEVENTS`).

Configuration MUST fail if no TLS keyword is supported (matching current `configure.ac:238`).

### C-10 — Substituted files
`tests/include/config.sh` and `snap/snapcraft.yaml` MUST be generated from their `.in` templates
with `@PACKAGE_VERSION@`, `@SB_GIT_SHA@`, `@USE_MYSQL@`, `@USE_PGSQL@` substituted (FR-005).

`SB_GIT_SHA` MUST resolve to an empty string when git metadata is unavailable, and the build MUST
still succeed (FR-005).

---

## Build & Test Contracts

### C-11 — Incremental correctness
- A no-op rebuild MUST perform zero compilation and zero linking (FR-006, SC-001).
- A single-source-file change MUST recompile only that file and relink (FR-006, SC-002).
- A bundled dependency MUST rebuild **if and only if** a file under its source directory changed
  (FR-007, BD-2). Unconditional `clean` of a bundled dependency is **prohibited**.

### C-12 — Out-of-tree purity
A build MUST NOT create or modify any file inside the source tree (FR-009, INV-3). After a full
build, `git status --porcelain` on the source tree MUST report no new or modified files.

This is a behaviour change: `third_party/luajit/Makefile.am:27` currently runs `make clean` in
`$(srcdir)`.

### C-13 — Test entry point
The cram suite MUST be runnable through the build system's native test command:

```shell
meson test -C builddir
```

The harness environment MUST be preserved (FR-015): `SBTEST_ROOTDIR`, `SBTEST_SCRIPTDIR`,
`SBTEST_SUITEDIR`, `SBTEST_CONFIG`, `SBTEST_INCDIR`, `LUA_PATH`, plus `PATH`/`PYTHONPATH`
additions for `third_party/cram`. The `sysbench` binary MUST be discoverable by
`tests/test_run.sh`, whose search list is source-relative — the build MUST either satisfy that
search or pass the binary location explicitly.

### C-14 — Baseline capture is a blocking precondition *(NON-NEGOTIABLE)*
No migration task may land before the Autotools baseline is captured and recorded.

Required measurements on one fixed machine and core count, 3 runs each, median reported, cache
state noted:
- Full clean build wall-clock
- No-op rebuild wall-clock
- Single-file-change rebuild wall-clock
- Project-authored build-config line count (**1,905** measured 2026-08-01)
- Vendored macro file count and lines (**13 files / 3,169 lines**)

Rationale: SC-001–SC-004 are ratios against this baseline. Per Constitution Principle VI,
"improved" may only be claimed from comparable before/after evidence. Without capture, those
criteria are permanently unverifiable.

### C-15 — Install layout
`meson install` MUST place artefacts identically to the Autotools build (FR-016, SC-011), except
the recorded correction in C-8/BT-1:

| Artefact | Destination |
|---|---|
| `sysbench` binary | `bindir` |
| 11 Lua benchmark scripts + `oltp_common.lua` | `pkgdatadir` |
| Test files (`t/*.t`, `include/*`) | `datadir/sysbench/tests/{t,include}` |
| 7 `.wasm` guests | **not installed** (was wrongly `bindir`) |

The nonstandard test-file install (globbing `*.t *.sh *.lua` across `t/` and `include/`, per
`tests/Makefile.am:28-56`) MUST be reproduced.

### C-16 — Emitted examples must be verified
Every command example in `quickstart.md` and in this contract MUST be executed against the real
tool during implementation and corrected if it differs. Examples authored from intent rather than
from execution MUST NOT be treated as verified.

---

## Verification Record

**C-7 — execution-verified 2026-08-01.** A candidate `scripts/embed-file.py` was run against all
6 real inputs and its output diffed against the current `sed` rule output:

| Input | Lines | Result |
|---|---|---|
| `sysbench.lua` | 160 | byte-identical |
| `sysbench.sql.lua` | 506 | byte-identical |
| `sysbench.cmdline.lua` | 215 | byte-identical |
| `sysbench.rand.lua` | 84 | byte-identical |
| `sysbench.histogram.lua` | 68 | byte-identical |
| `sysbench.py` | — | byte-identical *(after handling the no-trailing-newline case)* |

The first implementation attempt **failed** on `sysbench.py` because that file lacks a trailing
newline, which causes `sed` to place the closing `;` on the same line as the final literal. The
rule is now normative in C-7. Without this verification the port would have silently changed a
generated header.

**Not verified**: all `meson` invocations (tool absent from the authoring environment) — pinned
by C-16.
