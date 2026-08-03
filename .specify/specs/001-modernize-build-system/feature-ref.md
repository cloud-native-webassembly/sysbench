# Feature Reference: Modernize Build System

**Requirement**: `001-modernize-build-system` → Feature 019 Modern Build System  
**Date**: 2026-08-01

## Binding

| Field | Value |
|---|---|
| **Feature ID** | 019 |
| **Feature Name** | Modern Build System |
| **Classification** | Functional |
| **Detail file** | [`.specify/memory/features/019.md`](../../memory/features/019.md) |
| **Index row** | [`.specify/memory/features.md`](../../memory/features.md) § Functional Features |
| **Status transition** | `Draft` → `Planned` (owned by `/speckit.plan`) |
| **Requirement key** | `001-modernize-build-system` |

Feature 019 was created during the 2026-08-01 `/speckit.clarify` session rather than binding to
an existing feature. Rationale: this work spans the whole project — database drivers, scripting
backends, bundled dependencies, packaging, and CI — while the two topically closest features are
both WASM-scoped. Feature 009 is additionally already `Implemented`, so binding there would have
regressed its status, which the registry's never-regress rule forbids.

## How this plan maps to Feature 019

| Feature 019 key change | Plan artefact |
|---|---|
| 1. Select a modern build system | [`research.md`](./research.md) § Decision 1 — Meson, with CMake as documented fallback |
| 2. Reproduce all 22 options + 12-conditional matrix | [`contracts/option-mapping.md`](./contracts/option-mapping.md) — 22/22 mapped, 12 matrix configs, 6 negative cases |
| 3. Incremental, parallel, out-of-tree builds | `contracts/build-interface.md` C-11, C-12; gate G-3 |
| 4. Keep both code-generation paths working | C-7 (execution-verified byte-identical), C-8 (WASI-SDK guests) |
| 5. Migrate 7 consumers, then delete Autotools | [`contracts/migration-gates.md`](./contracts/migration-gates.md) G-4, G-6 |
| 6. Narrow platform set | Technical Context § Target Platform; FR-019 |
| 7. Fix three carried-forward defects | C-5 (master gate), C-8 (guest install + dead recipes), BT-1 |

## Feature-registry review (mandatory for the plan phase)

### New features introduced by this plan

**None.** The plan stays within Feature 019's scope as clarified. No new capability is introduced
that would warrant its own feature row.

### Features affected

| Feature | Status | Effect | Action |
|---|---|---|---|
| **009 — WASM Build Configuration** | Implemented | Owns `m4/sb_wasm.m4`, which gate G-6 **deletes**. Its capability (per-runtime SDK detection and conditional compilation) is preserved and re-expressed in `meson_options.txt` + `contracts/option-mapping.md`. The FR-020 defects (master gate, `WAMR_HOME`/`WASMTIME_HOME` substitution) are fixed here. | Reconcile when Feature 019 reaches `Implemented`: annotate 009 as superseded in its build-wiring aspects. **Do not regress 009's status** — the capability continues to exist. |
| **016 — Pinned SDK Provisioning** | Draft | Overlaps requirements Story 4 (dependency acquisition). This plan resolves the *bundled* half (LuaJIT/CK stay in-tree, per Decision 2) but does **not** pin external WASM SDK versions, which remains 016's distinct scope. | Keep 016 as an independent Draft. Its scope narrows to external SDK version pinning; note the narrowing when Feature 019 completes. |

### Classification consistency

Feature 019 is registered as **functional**, alongside 001–009. Build capability is a
user-facing project capability — a contributor invokes it directly and its option surface is
part of the project's interface — rather than a quality attribute of some other feature. This
is consistent with Feature 009 (`WASM Build Configuration`) also being classified functional.

### Key changes / notes to record in Feature 019 detail

1. **Build system selected: Meson ≥1.3 + Ninja.** Decided on bundled-dependency incrementality,
   the project's actual pain. Packaging support — expected to be decisive — was found
   non-discriminating (Debian, Fedora, and snapcraft all support both candidates natively).
2. **Baseline corrected.** Direct re-measurement gives 5,074 total / 1,905 project-authored /
   3,169 vendored, versus the spec's earlier 5,003 / 1,334 / 3,072. `requirements.md` updated per
   Constitution Principle II. SC-004's target is now ≤762 lines.
3. **Latent defect found: WASM guests are not built today.**
   `src/wasm/Makefile.am:41-45` uses space indentation where Make requires tabs, so those recipes
   are dead. The port implements intended behaviour, not current behaviour.
4. **C-7 execution-verified during planning.** A candidate `embed-file.py` produced byte-identical
   output for all 6 embedded headers — after a first attempt failed on `src/python/sysbench.py`,
   which lacks a trailing newline and therefore places `;` on the final literal's line. Rule now
   normative.
5. **Bundled dependencies stay in-tree** via `custom_target` + `depend_files` (stable API),
   deliberately avoiding dependence on Meson's experimental `external_project`.
6. **Baseline capture is a blocking gate (G-1).** SC-001–SC-004 are ratios against measurements
   that do not yet exist and become unrecoverable once Autotools is deleted.

## Downstream

- `/speckit.tasks` — decompose into tasks. Gate G-1 MUST become the first task; no `meson.build`
  authoring may precede it. G-4's seven consumer rows are mutually independent and
  parallelizable.
- `/speckit.implement` — owns the `Planned` → `Implemented` transition for Feature 019, gated on
  G-1…G-6 all passing.
