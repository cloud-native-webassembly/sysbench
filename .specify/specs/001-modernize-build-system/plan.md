# Implementation Plan: Modernize Build System

**Branch**: `001-modernize-build-system` | **Date**: 2026-08-01 | **Spec**: [requirements.md](./requirements.md)
**Requirement → Feature**: `001-modernize-build-system` → Feature 019 Modern Build System
**Input**: Specification from `.specify/specs/001-modernize-build-system/requirements.md`

## Summary

Replace GNU Autotools with **Meson + Ninja**, then delete the Autotools layer entirely.

The measured baseline is 5,074 Autotools lines, of which **3,169 (62%) is vendored third-party
`m4` that gets deleted rather than ported** — Meson and CMake both have native equivalents for
`acx_pthread`, `ax_tls`, `ax_gcc_func_attribute`, `lib-link`, `host-cpu-c-abi`, and `pkg.m4`.
The real porting surface is **1,905 project-authored lines**.

Meson is selected over CMake on the one factor that actually discriminates: **correct
incremental rebuilds of bundled dependencies**. That is the project's stated pain
(`third_party/luajit/Makefile.am:27` runs `make clean` on every build). Meson's
`external_project` module emits a real depfile by walking the dependency's source tree, so the
sub-make runs only when a source actually changed. CMake's `ExternalProject_Add` offers no
equivalent: `BUILD_ALWAYS ON` reproduces the current always-rebuild bug, and `OFF` produces
stale artefacts. Packaging support — the factor the spec weighted heaviest — turned out **not**
to discriminate: Debian, RPM, and Fedora all ship first-class macros for both, so that
constraint is satisfied either way.

## Clarifications

### Session 2026-08-01

- Q: Which build system replaces Autotools? → A: **Meson** (≥1.3), Ninja backend. Decisive factor
  is bundled-dependency incrementality; packaging support was found non-discriminating. CMake is
  recorded as the documented fallback if `external_project`'s experimental status proves
  unacceptable.
- Q: Do bundled LuaJIT / Concurrency Kit stay in-tree? → A: **Stay in-tree**, wrapped by
  `custom_target` + `depend_files` (stable API, correct incrementality). `external_project` is
  treated as a later optimization, not a prerequisite. This resolves the spec's remaining open
  decision without changing what ships.

## Technical Context

**Language/Version**: C — `configure.ac:45-47` checks `ac_cv_prog_cc_c99` but never sets
`-std=`; the Meson port pins `c_std=c99` explicitly, removing the ambiguity.
**Primary Dependencies**: Bundled static LuaJIT 2.1.0-beta3 (`libluajit-5.1.a`) and Concurrency
Kit (`libck.a`), both selectable as system via pkg-config; optional MySQL, PostgreSQL, Python3
(embedding), and four WASM runtime SDKs (WAMR, WasmEdge, Wasmer, Wasmtime) linked dynamically.
**Storage**: N/A — build-system change only.
**Testing**: Existing cram harness (46 `.t` files) via `tests/test_run.sh`, re-exposed as
`meson test`. No change to test content.
**Target Platform**: Linux x86_64 + aarch64, macOS (per FR-019). Sun Studio, PowerPC,
FreeBSD, and x86 CPUID probing are dropped.
**Project Type**: Single native C application (`bin_PROGRAMS = sysbench`) plus 5 static
convenience libs, 2 optional driver libs, 6 generated C headers, and 7 cross-compiled `.wasm`
guest modules.
**Performance Goals**: No-op rebuild <10% of Autotools baseline (SC-001); single-file rebuild
<25% (SC-002); full build ≤ baseline (SC-003). **All relative to a baseline that must be
captured before any migration work lands.**
**Constraints**: Preserve all 22 option capabilities and the 12-conditional matrix; keep both
code-generation paths byte-identical; out-of-tree builds must not write to the source tree;
identical install layout except recorded corrections.
**Scale/Scope**: Retire 5,074 lines / 40 files (`configure.ac`, `autogen.sh`, 18 `Makefile.am`,
21 `m4/*.m4`); migrate 7 downstream consumers + README.

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**Core Principles Compliance** (rendered from `.specify/memory/constitution.md`):

| # | Principle | Compliance | Evidence |
|---|-----------|------------|----------|
| I | Documentation-First | ✅ Pass | `quickstart.md` + `contracts/build-interface.md` land before implementation; FR-017 requires README/build-doc updates in the same change |
| II | Code as the Single Source of Truth | ✅ Pass | Baseline re-measured directly and the spec's estimate corrected (5,003→5,074, 1,334→1,905); dead space-indented recipes at `src/wasm/Makefile.am:41-45` found by reading code, not docs |
| III | Documentation Naming & Location Conventions | ✅ Pass | New docs use lowercase kebab-case under `docs/build/`; tool-mandated names (`meson.build`, `meson_options.txt`) kept exactly as required |
| IV | Feature-Centric Development | ✅ Pass | Bound to Feature 019; status advances Draft→Planned; overlaps with Features 009/016 recorded in `feature-ref.md` |
| V | Modular Runtime-Backend Architecture | ✅ Pass | `contracts/option-mapping.md` preserves per-runtime independence; each WASM backend stays separately toggleable and omittable |
| VI | Measurement Integrity | ⚠ Partial — see Complexity Tracking | SC-001–SC-004 are ratios against a baseline not yet captured; "improved" is unclaimable until T001 records it |
| VII | Extensibility for External Systems | ✅ Pass | Build-system-only change; no `sb_*` interface, Lua hook contract, or benchmark code is touched |
| VIII | Better-Harness Orientation | ✅ Pass | Strengthens Controlled Execution (one readable config) and Change Validation (`meson test`); FR-013 eliminates the silently-ignored-option defect class |

**Gates Status**: ✅ All gates pass, with Principle VI ⚠ Partial tracked and mitigated — see
Complexity Tracking. No unjustified failures.

**Re-check after Phase 1**: 2026-08-01 — re-run against the landed artefacts
(`data-model.md`, `contracts/` ×3, `quickstart.md`, `feature-ref.md`). Table unchanged;
Principle VI remains ⚠ Partial by design until the baseline task executes, and
`contracts/build-interface.md` C-14 now makes baseline capture a blocking precondition.

## Project Structure

### Documentation (this spec)

```text
.specify/specs/001-modernize-build-system/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output — build-system selection evidence
├── data-model.md        # Phase 1 output — build-configuration entities
├── quickstart.md        # Phase 1 output — verified build commands
├── contracts/           # Phase 1 output — build interface, option mapping, migration gates
├── feature-ref.md       # Phase 1 output — Feature 019 binding
├── tasks.md             # Phase 2 output (/speckit.tasks — NOT created here)
└── verification.md      # Implementation output (/speckit.implement)
```

### Source Code (repository root)

```text
meson.build                          # NEW — root build definition, project(), config.h generation
meson_options.txt                    # NEW — all 22 user-facing options in one readable file
src/meson.build                      # NEW — sysbench target, sources, conditional backends
src/lua/meson.build                  # NEW — Lua script install + generator() for embedded headers
src/python/meson.build               # NEW — Python embed header generation
src/wasm/meson.build                 # NEW — 7 guest modules via WASI-SDK custom_target
src/drivers/meson.build              # NEW — mysql/pgsql static libs
src/tests/meson.build                # NEW — 5 builtin benchmark static libs
tests/meson.build                    # NEW — cram harness wired to `meson test`
third_party/meson.build              # NEW — LuaJIT + CK wrappers with correct incrementality
scripts/embed-file.py                # NEW — portable replacement for the BSD/GNU-fragile sed rules
docs/build/                          # NEW — build documentation (FR-017)
configure.ac                         # DELETE (after all consumers migrate)
autogen.sh                           # DELETE
m4/                                  # DELETE — all 21 macros
**/Makefile.am                       # DELETE — all 18
.github/workflows/ci.yml             # EDIT — meson setup/compile/test
Dockerfile                           # EDIT — drop autogen/configure
debian/rules                         # EDIT — remove autoreconf override, use meson buildsystem
rpm/sysbench.spec                    # EDIT — %meson macros
snap/snapcraft.yaml.in               # EDIT — meson plugin + build-packages
scripts/build.sh                     # EDIT — fixes the FR-020 master-gate defect
README.md                            # EDIT — build instructions
```

**Structure Decision**: Converts a single native C application from 4-level recursive
Autotools into a flat Meson build with one `meson.build` per source directory (9 files) plus a
single `meson_options.txt` holding all 22 options. Two new top-level additions:
`scripts/embed-file.py` (replacing the fragile in-Makefile `sed` pipeline) and `docs/build/`.
The `third_party/` wrappers are rewritten but the vendored sources are untouched. Ninja
replaces recursive make, which is what removes the parallelism ceiling (FR-008).

### Mirror Obligations

N/A — this spec touches no mirrored or generated-copy surface. The `.specify/` command copies
under `.qoder/commands/` are auto-generated and not modified here; no source file in this
change has a dual-written twin.

## Phase 0: Research Review

Standalone [`research.md`](./research.md) — the build-system selection required external
evidence (Meson/CMake docs, Debian/Fedora/snapcraft packaging references, `external_project`
implementation source, migration precedents) and exceeds 50 lines.

**Decisions**: Meson ≥1.3 + Ninja; bundled deps stay in-tree via `custom_target` +
`depend_files`; WASI-SDK guests via `custom_target` invoking clang directly; `generator()` for
embedded headers; CMake documented as fallback.

**Three findings that changed the plan:**

1. **Packaging does not discriminate.** The spec assumed packaging support would drive the
   choice. It does not — Debian (`meson.pm`, auto-detected), Fedora (`%meson`/`%meson_build`/
   `%meson_install`), and snapcraft (`meson` plugin) all support both. This freed the decision
   to rest on incrementality, where Meson wins decisively.
2. **`src/wasm/Makefile.am:41-45` recipes are space-indented, not tab-indented** — verified with
   `cat -A`. Make requires tabs, so **the WASM guest modules are not currently built by
   WASI-SDK at all**. "Port existing behaviour" would have ported a no-op; this is a latent
   defect to fix, not preserve.
3. **The baseline was wrong.** Re-measurement gives 5,074 total / 1,905 project-authored, versus
   the spec's 5,003 / 1,334. The spec has been corrected (Principle II).

## Phase 1: Design & Contracts

Artefacts landed on disk (counts filled in after generation):

| Artefact | Content |
|---|---|
| [`data-model.md`](./data-model.md) | **7 entities**, 12 optional components, 1 resolution state machine, 5 named validation rules, 5 cross-entity invariants |
| [`contracts/build-interface.md`](./contracts/build-interface.md) | **16 contracts** (C-1…C-16) covering configure/compile/test/install |
| [`contracts/option-mapping.md`](./contracts/option-mapping.md) | **All 22 options** mapped (16 project options, 5 Meson built-ins, 1 recorded drop) + 12 matrix configs (M-1…M-12) + 6 negative cases (N-1…N-6) — satisfies FR-002/SC-005/SC-006/SC-010 |
| [`contracts/migration-gates.md`](./contracts/migration-gates.md) | **6 gates** (G-1…G-6) governing the staged cutover and Autotools deletion |
| [`quickstart.md`](./quickstart.md) | Verified command sequences for build, test, configuration matrix, baseline capture |
| [`feature-ref.md`](./feature-ref.md) | Feature 019 binding + Features 009/016 overlap reconciliation |

**Deviation from template guidance**: no REST/GraphQL contracts — this feature has no network
API. `contracts/` instead specifies the **build system's user-facing interface** (options,
commands, exit codes, diagnostics), which is the analogous contract surface and what downstream
consumers actually depend on.

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Principle VI ⚠ Partial — SC-001–SC-004 cannot be verified at plan time | The spec's efficiency criteria are deliberately expressed as ratios against the current system, which is the only honest way to state "faster than today". The baseline does not exist yet. | Absolute targets ("no-op rebuild <2s") were rejected as machine-dependent and unfalsifiable across contributors' hardware. Mitigation: `contracts/build-interface.md` C-14 makes baseline capture a **blocking precondition** — no migration task may land before it, and `quickstart.md` pins the exact measurement procedure (3 runs, median, cache state noted). |
| Two build systems coexist during migration | Seven independent consumers must each be converted and verified; a single atomic cutover would leave every packaging path broken if any one conversion failed. | Big-bang replacement rejected: it makes rollback all-or-nothing and blocks verification of consumers one at a time. Bounded by FR-018 + gate G-6, which require full Autotools deletion once all consumers pass — coexistence is a migration state with a defined exit, not an end state. |
