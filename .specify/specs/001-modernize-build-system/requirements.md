# Requirements Specification: Modernize Build System

**Requirement Branch**: `001-modernize-build-system`  
**Created**: 2026-08-01  
**Status**: Draft  
**Input**: User description: "对当前项目的构建系统进行升级。当前项目仍使用较早期的 Autotools 构建体系，效率低且可读性差。需要设计一套现代化的构建方案并实施改造。"

## Related Feature *(mandatory)*

**Feature ID**: 019  
**Feature Name**: Modern Build System

Bound to a newly created Feature ([019](../../memory/features/019.md)) rather than to an
existing one: this work spans the whole project — database drivers, scripting backends, bundled
dependencies, packaging, and CI — whereas the two candidate features are both WASM-scoped.
Feature 009 (WASM Build Configuration) is already `Implemented` and covers only
`m4/sb_wasm.m4`; binding here would have regressed its status. Feature 016 (Pinned SDK
Provisioning, `Draft`) overlaps Story 4 but is far narrower. Both overlaps are recorded in
Feature 019 for reconciliation when this work completes.

## Context & Baseline

The project builds through GNU Autotools. Measured surface area (source inspection,
2026-08-01):

| Dimension | Measured value |
|---|---|
| Total autotools lines | **5,074** |
| — project-authored (the porting surface) | **1,905** (`configure.ac` 496, `autogen.sh` 3, 18 `Makefile.am` 614, 8 project `m4` macros 792) |
| — vendored third-party `m4` (deleted outright, not ported) | **3,169** (13 files, incl. `lib-link.m4` 710, `host-cpu-c-abi.m4` 530, `pkg.m4` 353) |
| User-facing options | **22** (16 `--with-*`, 6 `--enable-*`/`--disable-*`) |
| Build conditionals | **12** `AM_CONDITIONAL`, consumed by 15 `if` blocks |
| Recursive make depth | **4 levels** (`.` → `src` → `tests` → `cpu`/`fileio`/…) |
| Downstream consumers of `./configure` | **7 files** + README |

> **Baseline correction (2026-08-01, during `/speckit.plan`)**: figures re-measured directly and
> revised from an earlier estimate of 5,003 total / 1,334 project-authored. The porting surface
> is larger than first stated (1,905), while the share that is merely *deleted* rather than
> ported is also larger (3,169 = 62%). Per Constitution Principle II, the measurement wins over
> the earlier estimate.

Concrete correctness, inefficiency, and readability evidence:

- **The tree does not build as shipped** (measured 2026-08-02). Two independent blockers:
  1. `sh autogen.sh` **fails**. `configure.ac:31` passes `-Werror` to automake, and
     `src/wasm/Makefile.am:41` emits `warning: '%'-style pattern rules are a GNU make extension`,
     which `-Werror` escalates to fatal. `autoreconf` exits 1.
  2. After bypassing that, `./configure` **fails**: `conditional "HAVE_WAMR" was never defined`.
     The four per-runtime `AM_CONDITIONAL`s live *inside* `AX_CHECK_WAMR`/`WASMEDGE`/`WASMER`/
     `WASMTIME`, which `SB_WASM` (`m4/sb_wasm.m4:227-234`) invokes only when `--with-wasm` is
     given. automake requires `AM_CONDITIONAL` to be unconditional, so the **default** configure
     path is broken.
- **Bundled dependencies are never rebuilt once built — a stale-artefact bug.**
  `third_party/luajit/Makefile.am:25` declares the rule as `$(builddir)/lib/libluajit-5.1.a:`
  with **no prerequisites**. Measured: touching `third_party/luajit/luajit/src/lj_api.c` produced
  **zero** compiles and left the archive's mtime unchanged — a LuaJIT source edit does not reach
  the binary. Only when the archive is absent does a full rebuild run, and only then does the
  unconditional `make clean` fire. This is an **under-build (silent staleness) correctness
  defect**, not the over-build slowness originally assumed.
- **Recursive make caps parallelism.** Automake processes `SUBDIRS` sequentially per directory
  across 4 levels, so `-j` cannot saturate the machine — though this is not observable as
  wall-clock pain at the current project size.
- **The developer build script forces a full rebuild and disables WASM.** `scripts/build.sh:3`
  runs `make clean` before `make -j32`, and `scripts/build.sh:2` omits `--with-wasm`, producing a
  binary with no WASM support at all.
- **Readability is dominated by vendored macro code.** 62% of the footprint is third-party `m4`
  the project neither wrote nor maintains, yet it must be read to answer "how is this library
  found?".
- **A configuration defect is hard to see.** The WASM master gate (`m4/sb_wasm.m4:227-234`)
  silently ignores `--with-wamr` unless `--with-wasm` is also passed. The bug survives because
  control flow is spread across `configure.ac` and five `m4` macros.
- **A latent substitution bug of the same class**: `m4/sb_wasm.m4:202` substitutes `WAMR_HOME`
  inside the Wasmtime branch instead of `WASMTIME_HOME`.

**Measured timing baseline** (median of 3, warm cache, 192-core x86_64, `--without-mysql`, after
patching the two blockers in an isolated scratch copy):

| Scenario | Autotools |
|---|---|
| Full clean build | **6 s** |
| No-op rebuild | **55 ms** |
| Single-C-file rebuild (`src/sb_timer.c`) | **178 ms** (1 compile + 1 link) |

The build is **not slow in absolute terms**. This re-anchors the migration's justification on
readability and correctness rather than raw speed — see § Clarifications (Session 2026-08-02).

This specification covers **designing and adopting a modern build system**. It is deliberately
technology-agnostic: naming a specific replacement is a planning decision, not a requirement.

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Build the project with a single, readable configuration (Priority: P1)

A contributor clones the repository and builds a working `sysbench` binary using the new build
system, with the WASM backends enabled, without consulting Autotools documentation or reading
`m4` macros. The configuration that selects optional components is expressed in one place they
can read top-to-bottom.

**Why this priority**: This is the irreducible core. If the new system cannot produce a
binary with the same optional components and equivalent behaviour, nothing else matters. It
also delivers the readability half of the user's stated problem on its own.

**Independent Test**: On a clean checkout, run the new system's configure-and-build steps and
confirm the resulting binary runs `sysbench --version`, `sysbench cpu run`, and a WASM
benchmark on an enabled runtime. Fully testable without touching CI, packaging, or the
dependency strategy.

**Acceptance Scenarios**:

1. **Given** a clean checkout on a machine with the required toolchain, **When** the
   contributor runs the documented configure and build commands, **Then** a working `sysbench`
   binary is produced and no Autotools tool (`autoreconf`, `automake`, `libtool`) is invoked.
2. **Given** the new build system, **When** the contributor requests the option that enables
   WASM support together with a specific runtime, **Then** that runtime is compiled in, and
   the build reports which optional components were enabled and which were skipped.
3. **Given** the new build system, **When** a contributor requests an optional component whose
   SDK or library is not installed, **Then** the build fails with a message naming the missing
   dependency and the option that requested it, or disables that component with an explicit
   warning — never silently ignoring the request.
4. **Given** the new build system, **When** a contributor asks how a dependency is located,
   **Then** the answer is discoverable in project-authored build files without reading
   vendored macro code.

---

### User Story 2 - Get fast incremental rebuilds during development (Priority: P1)

A developer iterating on a WASM backend edits one C file and rebuilds. Only the affected
translation unit and the final link are redone. Bundled dependencies that did not change are
not rebuilt, and the build saturates available cores.

**Why this priority**: Co-equal P1 with Story 1 because "效率低" (low efficiency) is half of
the stated problem, and it is the cost every contributor pays on every edit. It is
independently valuable: even if packaging and CI still used the old path, faster local
iteration would justify the work.

**Independent Test**: Measure a no-op rebuild and a single-file-change rebuild against the
Autotools baseline on the same machine. Requires no changes to CI or packaging.

**Acceptance Scenarios**:

1. **Given** a fully built tree, **When** the developer rebuilds without changing anything,
   **Then** the build performs no compilation or linking and completes in a small fraction of
   the baseline no-op time.
2. **Given** a fully built tree, **When** the developer modifies exactly one C source file,
   **Then** only that file is recompiled and the binary relinked; no bundled dependency is
   rebuilt.
3. **Given** a fully built tree, **When** the developer changes nothing about the bundled
   dependencies, **Then** those dependencies are not re-cleaned, re-copied, or re-compiled.
4. **Given** a machine with multiple cores, **When** a full build runs, **Then** it uses the
   available cores concurrently across the whole project rather than serialising per
   directory.
5. **Given** a build directory separate from the source tree, **When** a build runs, **Then**
   it succeeds and writes no build artefacts into the source tree.

---

### User Story 3 - Preserve every packaging and automation consumer (Priority: P2)

A release engineer builds the Debian package, the RPM, the snap, and the container image; CI
runs its jobs. All continue to work after the migration.

**Why this priority**: P2 because it protects existing distribution rather than delivering new
capability — but it is a hard gate before the old system can be removed, since seven files
plus the README currently invoke `./configure` or `autoreconf`.

**Independent Test**: Build each packaging target and run the CI workflow against the new
system, comparing produced artefacts and installed file lists to the baseline.

**Acceptance Scenarios**:

1. **Given** the migrated tree, **When** each packaging path (Debian, RPM, snap, container)
   is built, **Then** each succeeds and installs the same set of files to the same locations
   as the Autotools build.
2. **Given** the migrated tree, **When** the automated test suite is invoked through the new
   system's test entry point, **Then** all tests that passed under Autotools still pass, with
   the same cram-based harness environment.
3. **Given** the migrated tree, **When** a consumer's documented build commands are followed
   from the README, **Then** they match the commands the new system actually requires.
4. **Given** the migration is complete, **When** a contributor searches the tree for build
   entry points, **Then** no consumer still invokes a removed Autotools entry point.

---

### User Story 4 - Simplify how bundled and external dependencies are obtained (Priority: P3)

A contributor on a fresh machine obtains the dependencies needed for the build — bundled
LuaJIT and Concurrency Kit, plus any external SDKs for optional components — through a
documented, repeatable path rather than assembling them by hand.

**Why this priority**: P3 because Stories 1–3 can land while dependency acquisition still
works as it does today. It addresses the deeper maintainability problem (5,074 lines exist
largely to find libraries) and is the enabler for reproducible builds, but it is separable and
carries the most design freedom.

**Independent Test**: On a machine without the dependencies pre-installed, follow the
documented path and reach a successful build; verify the same versions are obtained on a
second machine.

**Acceptance Scenarios**:

1. **Given** a machine with only a compiler toolchain, **When** the contributor follows the
   documented dependency path, **Then** the build's required dependencies are obtained and the
   build succeeds.
2. **Given** two different machines following the same documented path, **When** both build,
   **Then** both obtain the same dependency versions.
3. **Given** the migrated build, **When** a contributor inspects which version of a bundled or
   external dependency the build used, **Then** that version is recorded and reportable.

---

### Edge Cases

- **Partial component selection**: a build requesting only one WASM runtime must succeed with
  the other three absent — the current 12-conditional matrix means many valid subsets exist.
- **Master-gate semantics**: requesting a sub-component without its parent gate must not be
  silently ignored (the present `--with-wamr`-without-`--with-wasm` defect); the new system
  must either imply the parent or reject the combination with a clear message.
- **Bundled vs system dependency**: both LuaJIT and Concurrency Kit can come from the system
  or the bundled copy; both paths must remain selectable.
- **Generated sources**: Lua and Python sources are embedded into C headers by hand-rolled
  `sed` rules, and 7 WASM guest modules are built by a separate cross-compiler toolchain. Both
  code-generation steps must keep working, including correct rebuild-on-change.
- **Cross-platform**: the supported set is Linux (x86_64, aarch64) and macOS. Builds must be
  verified on both families, including macOS-specific deployment-target and linker handling.
  Removing the dropped platforms (Sun Studio, PowerPC, FreeBSD, x86 CPUID probing) must not
  break the retained ones — several currently share code paths in
  `m4/sb_concurrency_kit.m4` and `m4/sb_luajit.m4`, notably the AArch64 atomics probe, which
  is retained.
- **Non-native install targets**: WASM guest modules are currently declared as `bin_PROGRAMS`
  and would be installed into the binary directory — a pre-existing defect the migration
  should not carry forward.
- **Version metadata**: the build records a short git SHA; a build from a tarball or a shallow
  clone with no git metadata must still succeed.
- **Concurrent migration**: while both systems coexist, a contributor must be able to tell
  which one is authoritative.
- **Sanitizer and coverage builds**: ASan, MSan, coverage, and debug configurations exist
  today and are used for diagnosing the WASM layer; they must remain available.

## Requirements *(mandatory)*

### Functional Requirements

**Capability parity**

- **FR-001**: The new build system MUST produce a `sysbench` binary functionally equivalent to
  the Autotools build for every supported combination of optional components.
- **FR-002**: The new build system MUST preserve every user-facing configuration capability
  currently exposed by the 22 Autotools options, covering: database drivers (MySQL,
  PostgreSQL), scripting backends (Python; bundled-vs-system LuaJIT), Concurrency Kit
  (bundled-vs-system), the WASM master gate and its four runtimes, asynchronous I/O,
  large-file support, debug builds, coverage, ASan, MSan, and warnings-as-errors. Option
  *names* MAY change; capabilities MUST NOT be dropped without an explicit recorded decision.
- **FR-003**: The new build system MUST reproduce the conditional-compilation matrix such that
  any subset of optional components builds successfully, and a component whose dependency is
  absent is excluded without breaking the build of the rest.
- **FR-004**: The new build system MUST keep both code-generation steps working: embedding Lua
  and Python sources into C headers, and cross-compiling the WASM guest modules with a separate
  toolchain — including correct rebuild when an input changes.
- **FR-005**: The new build system MUST continue to record and expose the build's version
  metadata, and MUST succeed when git metadata is unavailable.

**Efficiency**

- **FR-006**: The new build system MUST support incremental builds in which an unchanged input
  is not recompiled.
- **FR-007**: The new build system MUST NOT clean or rebuild a bundled dependency that has not
  changed.
- **FR-008**: The new build system MUST parallelise across the whole project rather than
  serialising directory by directory.
- **FR-009**: The new build system MUST support building into a directory separate from the
  source tree, leaving the source tree free of build artefacts.

**Readability & maintainability**

- **FR-010**: The new build system's project-authored configuration MUST be substantially
  smaller than the current 1,905 project-authored lines, and MUST NOT require vendoring
  third-party macro code to locate dependencies.
- **FR-011**: Dependency-discovery logic MUST be expressed in project-authored build files, so
  a contributor can determine how any dependency is found without reading vendored code.
- **FR-012**: The new build system MUST report, at configuration time, which optional
  components were enabled and which were skipped, and why.
- **FR-013**: The new build system MUST fail loudly — or disable with an explicit warning —
  when a requested optional component cannot be satisfied. Silently ignoring a requested
  option is prohibited. This closes the present master-gate defect class.

**Migration & consumers**

- **FR-014**: All existing downstream consumers MUST be migrated to the new system: CI
  workflows, the container image, Debian packaging, RPM packaging, snap packaging, and the
  developer build script.
- **FR-015**: The automated test suite MUST remain runnable through the new build system's test
  entry point, preserving the existing cram-based harness environment.
- **FR-016**: Installation MUST place the same artefacts in the same locations as the
  Autotools build, except where a pre-existing defect is deliberately corrected and recorded.
- **FR-017**: Build documentation MUST be updated so the documented commands match what the
  new system requires.
- **FR-018**: The Autotools build MUST be fully retired once every consumer in FR-014 is
  migrated and verified — specifically `configure.ac`, `autogen.sh`, all 18 `Makefile.am`
  files, and all 21 `m4/` macros MUST be deleted. Autotools MUST NOT be retained for
  development, packaging, or release. While both systems coexist during migration, the
  authoritative system MUST be documented.
- **FR-019**: The new build system MUST support Linux on x86_64 and aarch64, and macOS.
  Support for Sun Studio, PowerPC cache-line overrides, x86 CPUID-based architecture probing,
  and FreeBSD-specific linker handling MUST be dropped, and the removal MUST be recorded. Any
  further narrowing of this set requires an explicit recorded decision.
- **FR-020**: Correctness defects discovered in the current build MUST be fixed or explicitly
  carried as known issues in the new system — specifically the master-gate omission in
  `scripts/build.sh`, the `WAMR_HOME`-for-`WASMTIME_HOME` substitution error, and the
  misclassification of WASM guest modules as installable native programs.

### Key Entities

- **Build configuration**: the set of choices selecting optional components, build type, and
  dependency sources; must be inspectable and reportable.
- **Optional component**: a feature compiled in only when requested and satisfiable — a
  database driver, scripting backend, or WASM runtime. Has a request state, a satisfiability
  state, and a resulting enabled/skipped state with a reason.
- **Bundled dependency**: a dependency whose source is carried in-tree (LuaJIT, Concurrency
  Kit), selectable as bundled or system, with a version identity.
- **External SDK dependency**: a dependency that must pre-exist on the machine (WASM runtime
  SDKs, WASI-SDK, database client libraries), discovered at configuration time.
- **Generated source**: a build-produced input to compilation (embedded Lua/Python headers,
  compiled WASM guest modules) with a defined dependency on its inputs.
- **Build consumer**: an automation or packaging entry point that invokes the build (CI,
  container, Debian, RPM, snap, developer script, documentation).

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: A no-op rebuild of a fully built tree completes in **no more than the Autotools
  no-op time (measured: 55 ms)** and performs zero compilation and zero linking.
- **SC-002**: A single-C-file-change rebuild completes in **no more than the Autotools time
  (measured: 178 ms)** and recompiles exactly one translation unit.
- **SC-003**: A full clean build completes in no more than the Autotools full-build time
  (measured: 6 s) on the same machine and core count, and preferably less.

- **SC-004**: Project-authored build configuration is reduced to at most 40% of the current
  1,905 lines, with zero vendored third-party macro files required for dependency discovery
  (down from 13 files / 3,169 lines).
- **SC-005**: 100% of the capabilities behind the current 22 options remain available, or are
  recorded as deliberately dropped with a rationale.
- **SC-006**: Every valid subset of optional components in the conditional matrix builds
  successfully, verified across a documented set of configurations that includes at least:
  no optional components; each WASM runtime individually; all runtimes together; and each
  database driver.
- **SC-007**: 100% of the tests passing under Autotools pass under the new system, with no
  reduction in the number of tests executed.
- **SC-008**: All 7 downstream consumers plus the README build or run successfully against the
  new system, with zero remaining references to removed Autotools entry points.
- **SC-009**: A contributor unfamiliar with the project can produce a working WASM-enabled
  build by following the documented commands only, without reading build-system internals,
  in under 15 minutes of active effort on a prepared machine.
- **SC-010**: Requesting an unsatisfiable or incompletely-gated optional component produces a
  message naming the component and the unmet dependency in 100% of such cases — measured
  against a list of negative cases that includes the current master-gate defect scenario.
- **SC-011**: Installed file lists from the new system and the Autotools system are identical,
  except for deliberately recorded corrections.
- **SC-012**: After migration completes, zero Autotools input files remain in the repository —
  `configure.ac`, `autogen.sh`, 18 `Makefile.am`, and 21 `m4/` files are all deleted — and the
  build succeeds on a clean checkout with no Autotools tooling installed.

> **Criteria revised 2026-08-02** (see § Clarifications). SC-001 and SC-002 originally demanded
> <10% and <25% of baseline — i.e. <5.5 ms and <44 ms. Both were unachievable and not
> build-system-bound: Ninja's no-op floor for a project this size is ~10–50 ms, and the 178 ms
> single-file figure is dominated by one compiler-bound `gcc` invocation. They are restated as
> non-regression criteria. The migration's value rests on SC-004 (readability) and the
> correctness criteria, not on raw speed.

### Measurement Sources & Collection Methods

- **SC-001 / SC-002 / SC-003 Source**: Wall-clock timings captured on one fixed machine and
  core count. Baseline measured on the Autotools tree before migration and recorded in the
  plan; post-migration timings measured the same way. Each scenario run at least three times,
  reporting the median, with the file-system cache state noted. Per Constitution Principle VI,
  before/after comparisons are only valid from comparable measurements.
- **SC-004 Source**: Line counts of project-authored build files and a count of vendored macro
  files, collected by script and compared against the recorded baseline (1,905 project-authored
  lines; 13 vendored macro files, 3,169 lines).
- **SC-005 Source**: A capability-mapping table in the plan, one row per current option,
  mapping it to its replacement or to a recorded drop rationale. Reviewed at plan approval.
- **SC-006 Source**: A configuration matrix exercised by an automated script, one build per
  documented configuration, reporting pass/fail per row.
- **SC-007 Source**: Test-run counts and pass/fail results from the cram harness, compared
  before and after migration.
- **SC-008 Source**: Per-consumer build/run verification recorded in the verification log, plus
  a repository-wide grep for removed entry points (`autogen.sh`, `autoreconf`, `./configure`,
  `dh_auto_configure`, `%configure`, `plugin: autotools`) confirming zero live references.
- **SC-009 Source**: A timed walkthrough by a contributor not involved in the migration,
  following only the updated documentation; effort time and any point where they had to read
  build internals are recorded.
- **SC-010 Source**: A negative-case list executed as part of the configuration matrix,
  asserting a diagnostic naming the component and unmet dependency for each case.
- **SC-011 Source**: Install into a staging directory under both systems, with the resulting
  file lists diffed.
- **SC-012 Source**: A repository file-existence check for the named Autotools inputs, plus a
  clean-checkout build in an environment with `autoconf`/`automake`/`libtool` absent.

## Assumptions

- **Scope is the build system, not the code it builds.** No change to benchmark behaviour,
  measured results, or the C sources is in scope, beyond what is mechanically required to
  compile under the new system and the correctness fixes in FR-020.
- **Replacement technology is a planning decision.** The user asked for "a modern build
  scheme" without naming one; this spec therefore states capability and efficiency
  requirements only. Selecting the concrete system, and whether bundled dependencies move to
  a package manager or submodules, belongs to `/speckit.plan`.
- **Migration may be staged, but coexistence is temporary.** Both build systems may coexist
  during migration because seven independent consumers must each be converted and verified;
  FR-018 requires full Autotools deletion once they are. Coexistence is a migration state, not
  an end state.
- **The measurement baseline must be captured before migration.** SC-001–SC-003 and SC-004 are
  relative to the current tree, so the baseline is recorded as the first implementation step.
- **Platform scope is decided**: Linux (x86_64, aarch64) and macOS, per FR-019. Sun Studio,
  PowerPC, FreeBSD, and x86 CPUID probing are dropped — justified because CI exercises only
  ubuntu-20.04/22.04 and no WASM runtime SDK targets Sun Studio. The AArch64 atomics probe is
  retained since aarch64 remains supported.
- **Input modality is typed developer interaction** — command-line invocation and file editing.
  No voice, import, or API-driven consumption channel is involved.
- **No new identifiers collide.** Candidate names for new build options and variables were
  checked against the tree; no collisions were found. Concrete option naming is deferred to
  planning, where FR-002 requires a capability-mapping table.

## Clarifications

### Session 2026-08-01

- Q: Which Feature should this requirement bind to, given that no existing feature covers whole-project build modernization? → A: Create a new Feature 019 "Modern Build System"; Features 009 (WASM Build Configuration, Implemented) and 016 (Pinned SDK Provisioning, Draft) are both WASM-scoped and binding to 009 would regress an Implemented status.
- Q: Is the goal to fully retire Autotools, or to add a modern system alongside it? → A: Full replacement — migrate all 7 consumers, verify, then delete `configure.ac`, the 18 `Makefile.am` files, `autogen.sh`, and the 21 `m4/` macros. Autotools is not retained for any purpose.
- Q: Do you have a constraint on which build system replaces Autotools, or should the plan choose and justify it? → A: Defer to `/speckit.plan`; the spec stays technology-agnostic and the plan must justify the choice against the capability-parity and readability requirements.
- Q: Which platforms must the new build system support, given the current tree accommodates Sun Studio, PowerPC, FreeBSD and CPU probes that CI never exercises? → A: Linux (x86_64 + aarch64) and macOS only. Sun Studio, PowerPC cache-line overrides, x86 CPUID probes, and FreeBSD-specific handling are dropped.
### Session 2026-08-02

- 用户修订指示（verbatim）: "先完成基本框架的改造。当前镜像中缺少许多必要依赖，需重建构建环境。因此，本轮改造仅需完成新构建系统的框架搭建，无需进行实际测试。待后续完成底层镜像改造后，再进行编译测试。"
  → **Scope directive**: this round delivers the new build system's **framework/scaffolding only**.
  The container image lacks required dependencies (verified absent: `meson`, `ninja`, all four WASM
  runtime SDKs, WASI-SDK, `valgrind`, `debuild`/`rpmbuild`/`snapcraft`), so the build environment
  must be rebuilt first. Compilation and test verification are **deferred** to a follow-up round
  after the base image is reworked. All build/test/verify tasks are therefore recorded as
  deliberate deferrals (`[~]`), not omissions.
- Q: SC-001 and SC-002 demand a no-op rebuild under 5.5 ms and a single-file rebuild under 44 ms
  relative to the Autotools baseline. Measurement suggests both are unachievable. How should they
  be handled? → A: **Renegotiate against measured data.** Measured baseline (median of 3, warm
  cache, 192-core x86_64, `--without-mysql`): full clean build **6 s**, no-op rebuild **55 ms**,
  single-C-file rebuild **178 ms** (exactly 1 compile + link). Ninja's no-op floor for a project
  this size is ~10–50 ms, and the 178 ms is dominated by a single compiler-bound `gcc` invocation
  — neither is build-system-bound. SC-001/SC-002 are restated as non-regression criteria, and the
  migration's justification re-anchors on **readability** (1,905 → ≤762 project-authored lines)
  and **correctness** (stale artefacts, silently-ignored options, dead WASM recipes).
- Q: The tree does not build as shipped — `autogen.sh` fails, and `./configure` then fails with
  `conditional "HAVE_WAMR" was never defined`. How should the baseline be obtained? → A: Covered
  by the scope directive above. Both blockers are recorded as findings in § Context & Baseline. A
  provisional baseline was captured in an isolated scratch copy; the Autotools tree itself is not
  repaired, since it is slated for deletion.
