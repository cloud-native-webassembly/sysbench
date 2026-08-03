# Contract: Option Capability Mapping

**Requirement**: `001-modernize-build-system` → Feature 019 Modern Build System  
**Date**: 2026-08-01  
**Satisfies**: FR-002 (capability parity), SC-005 (100% of capabilities retained or drop recorded)

This document is the **authoritative capability-parity record**. Every user-facing Autotools
option is mapped to its replacement or to an explicit drop rationale. FR-002 permits option
*names* to change; it prohibits capabilities disappearing silently.

## Option type legend

| Type | Meson declaration | Semantics |
|---|---|---|
| `feature` | `type: 'feature'` | Tri-state `enabled`/`disabled`/`auto` — distinguishes "I require it" from "use it if available" |
| `boolean` | `type: 'boolean'` | Two-state flag |
| `combo` | `type: 'combo'` | Fixed choice set |
| `string` | `type: 'string'` | Free-form path or value |
| `builtin` | (Meson built-in) | Provided by Meson; no project option needed |

## Component options (12 — one per current `AM_CONDITIONAL`)

| # | Autotools option | Default | → Meson option | Type | Default | Notes |
|---|---|---|---|---|---|---|
| 1 | `--with-mysql` | yes | `mysql` | `feature` | `auto` | Was default-on; `auto` preserves "build it when available" while allowing explicit `enabled` to fail loudly per C-4 |
| 2 | `--with-pgsql` | no | `pgsql` | `feature` | `disabled` | |
| 3 | `--with-python` | no | `python` | `feature` | `disabled` | |
| 4 | `--enable-aio` | yes | `aio` | `feature` | `auto` | Linux-only; resolves `skipped` on macOS with reason |
| 5 | `--enable-largefile` | yes | — | `builtin` | — | Meson handles large-file support natively; no project option. **Capability retained, option removed.** |
| 6 | `--with-wasm` | no | `wasm` | `feature` | `disabled` | Master gate; see INV-1 / C-5 |
| 7 | `--with-wamr` | no | `wamr` | `feature` | `disabled` | Child of `wasm` |
| 8 | `--with-wasmedge` | no | `wasmedge` | `feature` | `disabled` | Child of `wasm` |
| 9 | `--with-wasmer` | no | `wasmer` | `feature` | `disabled` | Child of `wasm` |
| 10 | `--with-wasmtime` | no | `wasmtime` | `feature` | `disabled` | Child of `wasm` |
| 11 | `--with-system-luajit` | no (bundled) | `luajit` | `combo` | `bundled` | Choices: `bundled`, `system` |
| 12 | `--with-system-ck` | no (bundled) | `concurrency-kit` | `combo` | `bundled` | Choices: `bundled`, `system` |

**Master-gate correction (FR-013, C-5)**: the current help text for options 7–10 claims
"(default is enabled)" while the code defaults them off, and requesting one **without**
`--with-wasm` is silently ignored (`m4/sb_wasm.m4:227-234`). The replacement MUST make requesting
a child either imply `wasm` or fail with a clear message. Silently ignoring is prohibited.

## Dependency-path options (4)

| # | Autotools option | → Meson option | Type | Notes |
|---|---|---|---|---|
| 13 | `--with-mysql-includes` | `mysql-includes` | `string` | Empty = auto-discover via `mysql_config` |
| 14 | `--with-mysql-libs` | `mysql-libs` | `string` | Empty = auto-discover |
| 15 | `--with-pgsql-includes` | `pgsql-includes` | `string` | Empty = auto-discover via `pg_config` |
| 16 | `--with-pgsql-libs` | `pgsql-libs` | `string` | Empty = auto-discover |

WASM SDK locations continue to use `<NAME>_HOME` environment variables plus pkg-config, per C-6.
No new options are introduced for them; discovery order is header-and-library verified (VD-1),
replacing the current CLI-presence inference.

## Build-type and diagnostic options (6)

| # | Autotools option | → Replacement | Type | Notes |
|---|---|---|---|---|
| 17 | `--with-debug` | `-Dbuildtype=debug` | `builtin` | Maps to Meson's build type; `-DDEBUG` added via project args. **Capability retained via built-in.** |
| 18 | `--enable-coverage` | `-Db_coverage=true` | `builtin` | Replaces hand-rolled `GCOV_CFLAGS`/`GCOV_LDFLAGS` |
| 19 | `--enable-asan` | `-Db_sanitize=address` | `builtin` | Replaces `ASAN_CFLAGS` + `AX_CHECK_COMPILE_FLAG` probing |
| 20 | `--enable-msan` | `-Db_sanitize=memory` | `builtin` | Replaces `MSAN_CFLAGS` |
| 21 | `--disable-fail` | `-Dwerror=false` | `builtin` | Meson's `werror` defaults false, matching current default |
| 22 | `--with-gcc-arch` | — | — | **DROPPED** — see below |

**Note on 17**: the Autotools build couples `--with-debug` to `-O0 -g -DDEBUG` and treats debug,
coverage, and optimized as mutually exclusive branches (`configure.ac:326-338`). Meson separates
build type from sanitizer/coverage, so the combinations become independently expressible — a
capability gain, not a loss.

## Recorded drops (1)

| Option | Rationale |
|---|---|
| `--with-gcc-arch` | Provided by vendored `m4/ax_gcc_archflag.m4` (264 lines), which performs x86 CPUID probing to guess `-march`/`-mtune`. **Dropped** because: (a) FR-019 removes x86 CPUID probing from the supported platform set; (b) it produces non-portable binaries unsuitable for the distro packaging paths, which already pass `--without-gcc-arch` (`debian/rules:9`, `rpm/sysbench.spec:53-55`); (c) the capability remains fully accessible via the standard `-Dc_args=-march=native`. **No capability is lost** — only the bespoke auto-guessing wrapper. |

## Parity summary

| Category | Count | Disposition |
|---|---|---|
| Mapped to a project option | 15 | Options 1–4, 6–16 |
| Mapped to a Meson built-in | 6 | Options 5, 17–21 |
| Dropped with rationale | 1 | Option 22 (capability preserved via `-Dc_args`) |
| **Total** | **22** | **SC-005 satisfied: 22/22 accounted for; 0 capabilities lost** |

> **Arithmetic corrected 2026-08-02 during implementation.** This table previously read
> "16 project options / 5 built-ins", double-counting option 5 (`--enable-largefile`), which its
> own row already marks `builtin`. The declared option count in `meson_options.txt` is **15**,
> which now matches. No capability disposition changed — only the tally.

## Configuration matrix (satisfies SC-006)

The following configurations MUST each build successfully. This is the executable form of the
12-conditional matrix.

| # | Configuration | Purpose |
|---|---|---|
| M-1 | All components disabled | Minimal build; proves nothing is secretly mandatory |
| M-2 | `-Dmysql=enabled` | Database driver in isolation |
| M-3 | `-Dpgsql=enabled` | Second driver in isolation |
| M-4 | `-Dmysql=enabled -Dpgsql=enabled` | Both drivers together |
| M-5 | `-Dwasm=enabled -Dwamr=enabled` | Working WASM backend |
| M-6 | `-Dwasm=enabled -Dwasmedge=enabled` | Second working WASM backend |
| M-7 | `-Dwasm=enabled -Dwasmer=enabled` | Stub backend still compiles |
| M-8 | `-Dwasm=enabled -Dwasmtime=enabled` | Stub backend still compiles |
| M-9 | `-Dwasm=enabled` + all four runtimes | Full WASM matrix |
| M-10 | `-Dpython=enabled` | Scripting backend |
| M-11 | `-Dluajit=system -Dconcurrency-kit=system` | System dependency path |
| M-12 | Everything enabled | Maximal build |

### Negative cases (satisfies SC-010, FR-013)

Each MUST fail with a non-zero exit and a diagnostic naming the component and the unmet
dependency — or, for N-1, behave per the documented C-5 choice.

| # | Case | Required behaviour |
|---|---|---|
| N-1 | `-Dwamr=enabled` with `wasm=disabled` | Implies `wasm`, or fails naming the gate. **Never silently ignored** — this is the current `scripts/build.sh:2` defect |
| N-2 | `-Dwamr=enabled` with no WAMR SDK present | Fails naming `wamr` and the missing SDK |
| N-3 | `-Dmysql=enabled` with no MySQL client libs | Fails naming `mysql` and the missing dependency |
| N-4 | `-Dwasm=enabled` with no WASI-SDK | Fails naming the guest-module toolchain requirement |
| N-5 | `-Db_sanitize=address -Db_coverage=true` | Rejected as mutually exclusive |
| N-6 | Build directory equal to source root | Rejected per C-1 |
