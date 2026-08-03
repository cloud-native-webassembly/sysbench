---

description: "Task list template for feature implementation"
---

# Tasks: Modernize Build System

**Requirement ID**: 001
**Requirement Key**: 001-modernize-build-system
**Related Feature**: 019 Modern Build System (from .specify/memory/features.md)
**Input**: Design documents from `.specify/specs/001-modernize-build-system/`
**Prerequisites**: plan.md (required), requirements.md (required for user stories), research.md, data-model.md, contracts/

**Tests Mode**: ON (Constitution has no principle named NON-NEGOTIABLE, but § Development Workflow mandates "Core C utilities follow a test-first discipline" and Principle VI *Measurement Integrity* requires before/after evidence for any performance claim. Structural/contract verification tasks are therefore emitted per story.)

**⚠️ SCOPE DIRECTIVE (user, 2026-08-02)**: This round delivers the **build-system framework only**.
The image lacks required dependencies — `meson`, `ninja`, all four WASM SDKs, WASI-SDK, `valgrind`,
`debuild`/`rpmbuild`/`snapcraft` are all **verified absent**. Every task that requires *executing*
a build, test, or package is pre-marked `[~]` **deferred** with its blocking dependency named.
Authoring tasks are fully actionable now. A follow-up round runs the deferred set after the base
image is rebuilt.

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

## Definition of Done (DoD)

- DoD-1: Every `meson.build` / `meson_options.txt` file listed in plan.md § Source Code exists and is syntactically well-formed
- DoD-2: All 22 option capabilities from `contracts/option-mapping.md` are declared in `meson_options.txt`, with the 5 built-in-backed ones documented as such
- DoD-3: `scripts/embed-file.py` reproduces byte-identical output for all 6 embedded headers (contract C-7) — verifiable now, no Meson required
- DoD-4: Autotools files remain untouched and authoritative (FR-018); nothing is deleted this round
- DoD-5: Every deferred task carries a named blocking dependency in `verification.md` under `deferred_tasks=`
- DoD-6: `docs/build/` documents the new system's commands and the Autotools-is-still-authoritative status (FR-017)
- DoD-7: Feature 019 detail records this round's framework-only scope and the measured baseline findings

**DoD Status**: pending

## Completion Gate

- GATE-1: `scripts/embed-file.py` output byte-identical for all 6 inputs — check: `for f in <6 files>; do diff <(sed-rule "$f") <(python3 scripts/embed-file.py "$f" -); done`
- GATE-2: All 10 `meson.build`/`meson_options.txt` files exist — check: `ls meson.build meson_options.txt src/meson.build src/lua/meson.build src/python/meson.build src/wasm/meson.build src/drivers/meson.build src/tests/meson.build tests/meson.build third_party/meson.build`
- GATE-3: No `[ ]` or `[>]` task rows remain — check: `grep -cE '^- \[[ >]\]' tasks.md` returns 0
- GATE-4: verification.md lists every SC-NNN with a status — check: grep SC ids against requirements.md
- GATE-5: Autotools intact — check: `test -f configure.ac && test -d m4 && ls **/Makefile.am | wc -l` returns 18
- GATE-6: Option parity — check: every option name in `contracts/option-mapping.md` appears in `meson_options.txt` or is annotated `builtin`

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- **[blockedBy: T001,T002]**: explicit dependency tag — task MUST NOT start until every listed task is `[X]`
- Include exact file paths in descriptions

### Task State Sigil (REQUIRED)

- `- [ ]` — **Open**
- `- [>]` — **Claimed / in progress**
- `- [X]` — **Closed**
- `- [~]` — **Deferred** — reasons recorded in `verification.md` under `deferred_tasks=`

## Environment Prerequisites (probed 2026-08-02)

| Requirement | Status | Gates which tasks |
|---|---|---|
| `gcc` / `clang` / `pkg-config` / `python3` | ✅ present | authoring + embed verification |
| `autoconf` / `automake` / `libtoolize` | ✅ present | baseline capture (scratch copy only) |
| `mysql_config` / `pg_config` | ✅ present | would enable DB option probing |
| `docker` | ✅ present | image rebuild (follow-up round) |
| **`meson` / `ninja`** | ❌ **ABSENT** | **all configure/compile/test tasks** |
| **WASI-SDK, WAMR, WasmEdge, Wasmer, Wasmtime SDKs** (all `*_HOME` unset) | ❌ **ABSENT** | **WASM guest + runtime tasks** |
| `valgrind` | ❌ absent | leak verification (out of scope) |
| `debuild` / `rpmbuild` / `snapcraft` | ❌ absent | packaging verification |

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Establish the framework skeleton and record the baseline that Principle VI requires.

- [X] T001 [P] Record the measured Autotools baseline into `.specify/specs/001-modernize-build-system/verification.md`: full build 6 s, no-op 55 ms, single-file 178 ms (median of 3, 192-core x86_64, `--without-mysql`), plus line counts 1,905 project-authored / 3,169 vendored across 13 files, and machine identity (satisfies gate G-1 for the framework round)
- [X] T002 [P] Create `docs/build/` with `overview.md` stating that Autotools remains authoritative this round and Meson is scaffolding-only, per Constitution Principle I metadata rules (title, purpose, status, last-updated, related links)
- [X] T003 [P] Add `.gitignore` entries for Meson build directories (`builddir*/`, `build/`) so scaffolding work never pollutes `git status` (supports contract C-12)

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: The code-generation replacement and root build definition that every story depends on.

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T004 Create `scripts/embed-file.py` — portable replacement for the `sed` suffix rules in `src/lua/internal/Makefile.am:25-36` and `src/python/Makefile.am:23-34`. MUST reproduce contract C-7 exactly: `<var>` = filename with `.`→`_`; escape `\`→`\\` then `"`→`\"`; wrap each line as `  "<line>\n"`; and **when the input lacks a trailing newline, emit the closing `;` on the same line as the final literal** (`src/python/sysbench.py` has no trailing newline — last byte `0x29`)
- [X] T005 [blockedBy: T004] Verify `scripts/embed-file.py` byte-identical against the current `sed` rules for all 6 inputs (`src/lua/internal/sysbench{,.sql,.rand,.cmdline,.histogram}.lua`, `src/python/sysbench.py`) by diffing against reference output generated with the existing rule — this is executable now and requires no Meson
- [X] T006 [P] Create root `meson.build`: `project('sysbench', 'c', version: '1.1.0', default_options: ['c_std=c99', 'warning_level=2', 'werror=false'])`, `meson_version: '>=1.3'`, and the `config.h` generation per contract C-9 (17 header checks, 15 function checks, `SIZEOF_SIZE_T`, `SIZEOF_BOOL`, `HAVE_DECL_SHM_HUGETLB`, `HAVE_DECL_O_SYNC`, TLS keyword as `TLS` with hard failure if unsupported, `HAVE_FUNC_ATTRIBUTE_FORMAT`, `HAVE_FUNC_ATTRIBUTE_UNUSED`)
- [X] T007 [P] Create `meson_options.txt` declaring all 16 project options from `contracts/option-mapping.md` — 12 component options (`mysql`, `pgsql`, `python`, `aio` as `feature`; `wasm`, `wamr`, `wasmedge`, `wasmer`, `wasmtime` as `feature`; `luajit`, `concurrency-kit` as `combo` with `bundled`/`system`) and 4 dependency-path `string` options — with a header comment mapping the 5 built-in-backed capabilities and recording the `--with-gcc-arch` drop rationale
- [X] T008 [blockedBy: T006,T007] Implement the parent-gating rule (contract C-5, INV-1) in root `meson.build`: requesting any of `wamr`/`wasmedge`/`wasmer`/`wasmtime` MUST imply `wasm` or fail naming the gate. This closes the `m4/sb_wasm.m4:227-234` defect class where `--with-wamr` alone is silently ignored
- [X] T009 [blockedBy: T006] Implement version metadata in root `meson.build` per contract C-10: package version plus short git SHA via `vcs_tag`/`run_command`, resolving to an empty SHA when git metadata is absent, without failing the build (FR-005)
- [X] T010 [blockedBy: T006] Implement the configuration summary via Meson's `summary()` per contract C-3: every optional component's resolution (`compiled-in`/`skipped`) with a reason when skipped, plus build type, C standard, sanitizer/coverage state, and LuaJIT/CK source (FR-012, INV-4)

**Checkpoint**: Framework foundation authored — user story scaffolding can now proceed in parallel

---

## Phase 3: User Story 1 - Build with a single, readable configuration (Priority: P1) 🎯 MVP

**Goal**: A contributor can read one `meson_options.txt` plus per-directory `meson.build` files and understand the whole build, with no `m4` macros involved.

**Independent Test**: Inspect the authored files and confirm every dependency-discovery path is expressed in project-authored Meson code; confirm option count parity against `contracts/option-mapping.md`. Executable now. Actual build verification is deferred.

### Verification for User Story 1

- [X] T011 [P] [US1] Structural contract test in `tests/build/test_option_parity.py`: assert every option name in `contracts/option-mapping.md` appears in `meson_options.txt` or is annotated as a Meson built-in; derive the count with `len(...)` from the parsed table rather than hard-coding (satisfies SC-005, GATE-6)
- [X] T012 [P] [US1] Structural contract test in `tests/build/test_no_vendored_macros.py`: assert no file under `m4/` is referenced by any `meson.build`, and that project-authored Meson line count is ≤762 (40% of the measured 1,905 baseline) — floor-semantics assertion, not equality (satisfies SC-004, FR-010, FR-011)
- [X] T013 [US1] Manual review: walk `docs/build/overview.md` and confirm a reader can answer "how is each dependency found?" without opening any `m4` file (FR-011)

### Implementation for User Story 1

- [X] T014 [P] [US1] [blockedBy: T006] Create `src/meson.build`: the `sysbench` executable with the base source list from `src/Makefile.am:33-43`, plus conditional appends for `sb_python.c`, `sb_wasm.c`, `sb_wamr.c`, `sb_wasmedge.c`, `sb_wasmer.c`, `sb_wasmtime.c` guarded on their component options; link the 5 builtin static libs and 2 driver libs; carry `-DDATADIR`/`-DLIBDIR` per `src/Makefile.am:20`
- [X] T015 [P] [US1] [blockedBy: T006] Create `src/tests/meson.build` declaring the 5 builtin benchmark static libraries (`libsbcpu`, `libsbfileio`, `libsbmemory`, `libsbthreads`, `libsbmutex`) with sources from each `src/tests/*/Makefile.am:18-20`; note `libsbfileio` additionally needs `crc32.c`
- [X] T016 [P] [US1] [blockedBy: T006] Create `src/drivers/meson.build` declaring `libsbmysql` and `libsbpgsql` static libraries, each gated on its component option, with dependency discovery via pkg-config falling back to `mysql_config`/`pg_config` and honouring the 4 path options (contract C-6, VD-1: verify header **and** library, never CLI presence alone)
- [X] T017 [P] [US1] [blockedBy: T004] Create `src/lua/meson.build`: a `generator()` invoking `scripts/embed-file.py` over the 5 `src/lua/internal/*.lua` inputs, plus installation of the 11 benchmark Lua scripts and `oltp_common.lua` to `pkgdatadir` per `src/lua/Makefile.am:19-31`
- [X] T018 [P] [US1] [blockedBy: T004] Create `src/python/meson.build`: reuse the same `generator()` for `src/python/sysbench.py` → `sysbench.py.h`, gated on the `python` option, with python3 embed discovery via `dependency('python3', method: 'pkg-config')`
- [X] T019 [US1] [blockedBy: T006] Create `third_party/meson.build` wrapping bundled LuaJIT and Concurrency Kit as `custom_target` with `depend_files` over each dependency's source tree, so the archive rebuilds **if and only if** a source changed (contract C-11, BD-2). MUST NOT run `make clean` in `$(srcdir)` — copy-then-build only (BD-1, contract C-12, fixing the `third_party/luajit/Makefile.am:27` source-tree write). Also wire the `system` alternative for both via pkg-config
- [X] T020 [US1] [blockedBy: T006] Create `src/wasm/meson.build`: a `custom_target` per guest module invoking `$WASI_SDK_HOME/bin/clang` with `--target=wasm32-wasi --sysroot=$WASI_SDK_HOME/share/wasi-sysroot -fPIC -z stack-size=8192 -Wl,--initial-memory=65536 -Wl,--no-entry -Wl,--export=event`, plus `-Wl,--export=create_buffer` for `pass_data`. Outputs MUST NOT install to `bindir` (BT-1, FR-020). **Note**: the current rules at `src/wasm/Makefile.am:41-45` are space-indented where Make requires tabs and are therefore dead — implement intended behaviour, not current behaviour
- [X] T021 [US1] [blockedBy: T006] Create `tests/meson.build` registering the cram suite as a Meson test per contract C-13, preserving `SBTEST_ROOTDIR`, `SBTEST_SCRIPTDIR`, `SBTEST_SUITEDIR`, `SBTEST_CONFIG`, `SBTEST_INCDIR`, `LUA_PATH`, and the `PATH`/`PYTHONPATH` additions for `third_party/cram`; generate `tests/include/config.sh` from its `.in` with `@PACKAGE_VERSION@`, `@SB_GIT_SHA@`, `@USE_MYSQL@`, `@USE_PGSQL@` (contract C-10); reproduce the nonstandard test-file install from `tests/Makefile.am:28-56` (contract C-15)
- [~] T022 [US1] [blockedBy: T014,T015,T016,T017,T018,T019,T020,T021] Execute `meson setup builddir && meson compile -C builddir` and confirm a working `sysbench` binary <!-- deferred: meson/ninja absent from image; awaiting base-image rebuild per 2026-08-02 scope directive -->
- [~] T023 [US1] [blockedBy: T022] Verify the configuration summary output lists every component's resolution and reason (contract C-3) <!-- deferred: requires meson -->

**Checkpoint**: US1 framework fully authored and structurally verified; execution deferred

---

## Phase 4: User Story 2 - Fast incremental rebuilds (Priority: P1)

**Goal**: Rebuilds do only the work a change requires, and — critically — a bundled-dependency source edit actually reaches the binary.

**Independent Test**: After the image rebuild, touch one C file and confirm exactly one recompile; touch a LuaJIT source and confirm the archive **does** rebuild (the current build silently does not).

### Verification for User Story 2

- [X] T024 [P] [US2] Author the incrementality verification script `tests/build/check_incremental.sh` asserting: (a) no-op rebuild does zero work, (b) single-file change recompiles exactly one TU, (c) touching a bundled-dep source **does** rebuild its archive, (d) `git status --porcelain` stays clean after a full build. Script is authored now and executed in the follow-up round
- [~] T025 [US2] [blockedBy: T024,T022] Execute `tests/build/check_incremental.sh` and record results against the measured baseline (SC-001 ≤55 ms no-op, SC-002 ≤178 ms single-file, SC-003 ≤6 s full) <!-- deferred: requires meson/ninja -->

### Implementation for User Story 2

- [X] T026 [US2] [blockedBy: T019] Document in `third_party/meson.build` the `depend_files` globs that make each bundled dependency's rebuild trigger correct, and record in `docs/build/overview.md` that this fixes a **stale-artefact correctness bug** — measured: touching `third_party/luajit/luajit/src/lj_api.c` currently produces zero compiles and leaves the archive mtime unchanged
- [X] T027 [P] [US2] Confirm by inspection that no authored `meson.build` writes into the source tree (contract C-12, INV-3): no `custom_target` output path resolves under the source root, and no recipe invokes `make clean` in a source directory
- [~] T028 [US2] [blockedBy: T022] Verify whole-project parallelism — a full build saturates available cores rather than serialising per directory (FR-008) <!-- deferred: requires ninja -->

**Checkpoint**: Incrementality design authored and statically reviewed; measurement deferred

---

## Phase 5: User Story 3 - Preserve packaging and automation consumers (Priority: P2)

**Goal**: All 7 downstream consumers keep working after migration.

**Independent Test**: Each consumer's build file is migrated and reviewable independently; execution deferred per scope directive.

### Verification for User Story 3

- [X] T029 [P] [US3] Author `tests/build/test_consumer_refs.py`: assert that after migration completes, zero live references to `autogen.sh`, `autoreconf`, `./configure`, `dh_auto_configure`, `%configure`, or `plugin: autotools` remain outside `third_party/` and `.specify/` (SC-008, gate G-6). Test MUST currently be expected-to-fail (9 files match today), documenting the target state
- [~] T030 [US3] [blockedBy: T031,T032,T033,T034,T035,T036] Execute each consumer's build and verify success <!-- deferred: debuild/rpmbuild/snapcraft absent; meson absent -->

### Implementation for User Story 3

- [X] T031 [P] [US3] Migrate `.github/workflows/ci.yml` to `meson setup` / `meson compile` / `meson test`, replacing `./autogen.sh` + `./configure` at lines 15,17,41,43; keep the ubuntu-22.04 and ubuntu-20.04 matrix and add meson/ninja to the installed packages
- [X] T032 [P] [US3] Decide and record the disposition of `.travis.yml` (lines 219,224,229) — migrate or delete. The repo has both Travis and GitHub Actions; migrating a dead config wastes effort, deleting a live one loses coverage. Record the decision in `docs/build/overview.md`
- [X] T033 [P] [US3] Migrate `Dockerfile` (lines 16,17,19) to install meson+ninja and use `meson setup`/`meson compile`/`meson install`, dropping `autogen.sh` and `./configure`. This is also the vehicle for the base-image rebuild the scope directive requires
- [X] T034 [P] [US3] Migrate `debian/rules`: delete the `autoreconf -vif` override at line 8 and the explicit `dh_auto_configure` args at line 9, relying on debhelper's `meson.pm` autodetection; pass component options via `dh_auto_configure -- -Dmysql=enabled -Dpgsql=enabled`
- [X] T035 [P] [US3] Migrate `rpm/sysbench.spec`: replace `autoreconf -vif` (line 52) and `%configure` (lines 53-55) with `%meson`, `%meson_build`, `%meson_install`, `%meson_test`; add `BuildRequires: meson ninja-build`
- [X] T036 [P] [US3] Migrate `snap/snapcraft.yaml.in` (line 31) from `plugin: autotools` to `plugin: meson`, adding `meson` and `ninja` to `build-packages` since the snapcraft meson plugin does not auto-supply them
- [X] T037 [US3] [blockedBy: T031] Fix `scripts/build.sh` — replace the autotools invocation and **eliminate the master-gate defect**: the current line 2 omits `--with-wasm`, silently producing a WASM-less binary. The Meson equivalent must make that state unexpressible (FR-020)
- [X] T038 [US3] [blockedBy: T033] Update `README.md` build instructions (lines 238,240,242) to the Meson commands, and add a note that Autotools remains authoritative until gate G-6 (FR-017, FR-018)

**Checkpoint**: All 7 consumers migrated in source; execution verification deferred

---

## Phase 6: User Story 4 - Simplify dependency acquisition (Priority: P3)

**Goal**: A contributor on a fresh machine obtains build dependencies through a documented, repeatable path.

**Independent Test**: Follow `docs/build/dependencies.md` on a machine without the dependencies and reach a successful configure. Deferred per scope directive.

### Verification for User Story 4

- [X] T039 [P] [US4] Author `tests/build/test_dependency_docs.py`: assert `docs/build/dependencies.md` names every external SDK from `data-model.md` Entity 4 and that each named `*_HOME` variable is referenced by at least one authored `meson.build`; derive the SDK list from the data model rather than hard-coding

### Implementation for User Story 4

- [X] T040 [P] [US4] Create `docs/build/dependencies.md` documenting how each dependency is obtained: bundled LuaJIT/Concurrency Kit (in-tree, per research.md Decision 2), the four WASM runtime SDKs and WASI-SDK (external, via `*_HOME`), and the DB client libraries. Record that **none is version-pinned today** and that pinning remains Feature 016's scope
- [X] T041 [US4] [blockedBy: T010] Extend the `summary()` output to report each resolved dependency's version where the SDK exposes one, so a build records what it was built against (Story 4 acceptance scenario 3)

**Checkpoint**: Dependency acquisition documented; reproducibility verification deferred

---

## Phase 7: Polish & Cross-Cutting Concerns

- [X] T042 [P] Create `docs/build/migration-status.md` tracking gate G-1…G-6 progress from `contracts/migration-gates.md`, with this round's framework scope and the deferred set explicitly marked
- [X] T043 [P] Record in `.specify/memory/features/019.md` the framework-only round scope, the two build-blocker findings, the inverted incrementality finding, and the revised SC-001/SC-002
- [X] T044 [blockedBy: T005,T011,T012] Write `verification.md` with a status row for every SC-001…SC-012 (`pass` / `deferred`) and a `deferred_tasks=` list naming each deferred task's blocking dependency
- [~] T045 [blockedBy: T022,T025,T030] Run the full 12-configuration matrix M-1…M-12 and the 6 negative cases N-1…N-6 from `contracts/option-mapping.md` <!-- deferred: requires meson + all four WASM SDKs -->
- [~] T046 [blockedBy: T045] Verify install parity against the baseline file list — only expected difference is 7 `.wasm` guests no longer in `bindir` (SC-011, gate G-5) <!-- deferred: requires meson -->
- [~] T047 [blockedBy: T046] Gate G-6: delete `configure.ac`, `autogen.sh`, 18 `Makefile.am`, 21 `m4/*.m4` in a single isolated commit containing only deletions <!-- deferred: blocked on all prior gates; explicitly OUT OF SCOPE for the framework round per FR-018 -->

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies — can start immediately
- **Foundational (Phase 2)**: Depends on Setup — BLOCKS all user stories
- **User Stories (Phase 3–6)**: All depend on Foundational completion; then parallelizable
- **Polish (Phase 7)**: Depends on desired user stories being complete

### User Story Dependencies

- **US1 (P1)**: Foundational only — no dependency on other stories
- **US2 (P1)**: Depends on T019 (`third_party/meson.build`) from US1; verification is independent
- **US3 (P2)**: Independent of US1/US2 authoring — consumer files can migrate in parallel
- **US4 (P3)**: Depends on T010 (`summary()`) from Foundational

### Parallel Opportunities

- Phase 1: T001, T002, T003 all `[P]`
- Phase 2: T006, T007 `[P]` (different files); T004 must precede T005
- US1: T014–T018 all `[P]` (separate `meson.build` files); T019, T020, T021 touch distinct files
- US1 verification: T011, T012 `[P]`
- US3: T031–T036 all `[P]` — six independent consumer files, the single largest parallel batch
- Phase 7: T042, T043 `[P]`

---

## Parallel Example: User Story 3

```bash
# Six consumer migrations, all independent files:
Task: "Migrate .github/workflows/ci.yml to meson setup/compile/test"
Task: "Decide disposition of .travis.yml"
Task: "Migrate Dockerfile to meson + install meson/ninja"
Task: "Migrate debian/rules to debhelper meson.pm autodetection"
Task: "Migrate rpm/sysbench.spec to %meson macros"
Task: "Migrate snap/snapcraft.yaml.in to plugin: meson"
```

---

## Implementation Strategy

### This round: framework only

1. Phase 1 (Setup) — baseline recorded, docs skeleton
2. Phase 2 (Foundational) — `embed-file.py` **verified byte-identical now**, root `meson.build`, `meson_options.txt`
3. Phase 3–6 — author all `meson.build` files and migrate consumer files
4. Phase 7 — status docs, `verification.md` with the deferred register
5. **STOP** — do not execute builds; Autotools stays authoritative (FR-018)

### Follow-up round (after base-image rebuild)

1. Rebuild the image with meson, ninja, WASI-SDK, and the four WASM runtime SDKs (T033 is the vehicle)
2. Execute the deferred set: T022, T023, T025, T028, T030, T045, T046
3. Evaluate gates G-2…G-5 against measured results
4. Only then T047 (gate G-6 deletion) in an isolated commit

### MVP scope

**User Story 1** alone (Phases 1–3) delivers a complete, readable build framework and satisfies
SC-004 — the migration's strongest measured win — without requiring any absent tooling.

---

## Notes

- `[P]` tasks = different files, no dependencies
- `[Story]` label maps task to user story for traceability
- **Deferral discipline**: 8 tasks are pre-marked `[~]` because probing confirmed their tooling is
  absent. Each names its blocker inline. This is a deliberate handoff, not unfinished work.
- **Autotools is untouched this round.** T047 (deletion) is deferred and explicitly out of scope.
- T005 is the highest-value early task: it verifies contract C-7 with tooling that is present,
  closing the round's one genuinely risky correctness requirement.
