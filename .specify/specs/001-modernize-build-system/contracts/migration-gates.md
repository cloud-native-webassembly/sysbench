# Contract: Migration Gates

**Requirement**: `001-modernize-build-system` → Feature 019 Modern Build System  
**Date**: 2026-08-01  
**Satisfies**: FR-014, FR-018, SC-008, SC-012

Autotools removal is irreversible in practice — seven independent consumers depend on
`./configure`, and a broken packaging path is discovered only at release time. These gates define
the ordered, verifiable sequence that governs the cutover. Each gate MUST pass before the next
begins.

## G-1 — Baseline captured *(NON-NEGOTIABLE, blocks everything)*

**Precondition for every subsequent gate.** No `meson.build` file may be authored before G-1
passes.

Exit criteria:

- [ ] Full clean build wall-clock recorded (3 runs, median, cache state noted)
- [ ] No-op rebuild wall-clock recorded (3 runs, median)
- [ ] Single-C-file-change rebuild wall-clock recorded (3 runs, median)
- [ ] Machine identity recorded: CPU model, core count, OS, compiler version
- [ ] Line-count baseline confirmed: **1,905** project-authored, **3,169** vendored across 13 files
- [ ] Installed file list captured via staged install (baseline for SC-011)
- [ ] Test-run count and pass/fail recorded from the Autotools build (baseline for SC-007)

**Rationale**: SC-001–SC-004, SC-007, and SC-011 are all defined relative to this baseline.
Per Constitution Principle VI, "improved" may only be claimed from comparable before/after
evidence. Once Autotools is deleted the baseline is unrecoverable.

## G-2 — Meson build reaches parity (development use only)

The new build produces a working binary; Autotools remains authoritative.

Exit criteria:

- [ ] `meson setup` + `meson compile` produce a `sysbench` binary
- [ ] `sysbench --version`, `sysbench cpu run`, and a WASM benchmark on WAMR all succeed
- [ ] All 12 matrix configurations M-1…M-12 build (`option-mapping.md`)
- [ ] All 6 negative cases N-1…N-6 behave as specified
- [ ] `meson test` runs the cram suite with the same test count and pass set as G-1 (SC-007)
- [ ] Generated headers byte-identical to Autotools output — all 6 files (C-7)
- [ ] `config.h` defines the same macro set as the Autotools header (C-9)
- [ ] Out-of-tree purity verified: `git status --porcelain` clean after a full build (C-12)
- [ ] Incrementality verified: no-op rebuild does zero work; single-file change recompiles one
      file; bundled deps do not rebuild when unchanged (C-11)
- [ ] Documentation states Autotools is still authoritative (FR-018)

**Gate discipline**: no consumer migrates until G-2 passes. A consumer switched to a
non-parity build would fail in a way easily misattributed to the consumer rather than the build.

## G-3 — Performance criteria met

Exit criteria (all measured on the G-1 machine, same method):

- [ ] SC-001: no-op rebuild < 10% of baseline
- [ ] SC-002: single-file rebuild < 25% of baseline
- [ ] SC-003: full build ≤ baseline
- [ ] SC-004: project-authored build config ≤ 40% of 1,905 lines (**≤ 762 lines**), with zero
      vendored macro files required for dependency discovery
- [ ] Whole-project parallelism confirmed — build saturates available cores rather than
      serialising per directory (FR-008)

If a criterion is missed, the shortfall MUST be recorded with analysis before proceeding.
Proceeding with a recorded, accepted shortfall is permitted; proceeding silently is not.

## G-4 — Consumers migrated

Each of the 7 consumers reaches `migration_state = verified` (Entity 7, BC-1). Each row is
independently verifiable, so consumers may migrate one at a time.

| Consumer | File | Verification |
|---|---|---|
| [ ] CI | `.github/workflows/ci.yml` | Workflow green on ubuntu-22.04 + ubuntu-20.04 |
| [ ] Legacy CI | `.travis.yml` | Migrated, or **removed** if Travis is no longer used — decide explicitly |
| [ ] Container | `Dockerfile` | Image builds; `sysbench --version` runs inside it |
| [ ] Debian | `debian/rules` | Package builds; `autoreconf` override deleted |
| [ ] RPM | `rpm/sysbench.spec` | Package builds with `%meson` macros |
| [ ] snap | `snap/snapcraft.yaml.in` | Snap builds; meson+ninja added to `build-packages` |
| [ ] Dev script | `scripts/build.sh` | Builds **with WASM actually enabled** — fixes the FR-020 master-gate defect |
| [ ] Docs | `README.md` | Documented commands match reality (FR-017) |

**`.travis.yml` requires an explicit decision.** The repository contains both a Travis config and
GitHub Actions workflows. Migrating a dead CI file wastes effort; deleting a live one loses
coverage. Determine which before touching it.

## G-5 — Install parity verified

Exit criteria:

- [ ] Staged install under Meson produces a file list identical to the G-1 baseline, except the
      recorded corrections (SC-011, C-15)
- [ ] The only intended difference is that 7 `.wasm` guests are **no longer** installed to
      `bindir` (BT-1, FR-020)
- [ ] Nonstandard test-file install reproduced: `t/` and `include/` globs to
      `datadir/sysbench/tests/` (C-15)
- [ ] Lua scripts install to `pkgdatadir` unchanged

## G-6 — Autotools deleted

**Only after G-1…G-5 all pass.**

Exit criteria:

- [ ] `configure.ac` deleted
- [ ] `autogen.sh` deleted
- [ ] All 18 `Makefile.am` deleted
- [ ] All 21 `m4/*.m4` deleted (directory removed)
- [ ] Repository-wide grep for `autogen.sh`, `autoreconf`, `./configure`, `dh_auto_configure`,
      `%configure`, `plugin: autotools` returns zero live references (SC-008)
- [ ] Clean-checkout build succeeds in an environment where `autoconf`, `automake`, and
      `libtool` are **absent** (SC-012)
- [ ] `EXTRA_DIST`-era distribution concerns resolved — the release tarball mechanism replaced
      by `meson dist`
- [ ] Feature 019 status advanced per the canonical state machine

## Rollback

| Gate reached | Rollback action |
|---|---|
| G-1…G-5 | Delete the Meson files. Autotools is still authoritative and untouched; no consumer is affected. |
| After G-6 | `git revert` the deletion commit. **G-6 MUST therefore be a single, isolated commit touching only deletions** — no functional change may be mixed into it. |

## Gate dependency graph

```
G-1 (baseline) ──→ G-2 (parity) ──→ G-3 (performance)
                                          │
                                          ↓
                        G-4 (consumers, parallelizable per row)
                                          │
                                          ↓
                                   G-5 (install parity)
                                          │
                                          ↓
                                   G-6 (deletion, single commit)
```

G-1 is a hard serialization point. G-4's rows are mutually independent and may proceed in
parallel once G-3 passes.
