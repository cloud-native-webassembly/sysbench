<!--
SYNC IMPACT REPORT
==================
Version change: (none) → 1.0.0.1 (initial ratification — MAJOR baseline)
Bump type: MAJOR (first ratified constitution; establishes the baseline
  against which all future amendments are measured).

Modified principles: N/A (initial creation)

Added sections:
  - Core Principles (I–VIII)
  - WASM Runtime Harness Standards
  - Development Workflow
  - Governance

Removed sections: none.

Templates requiring updates:
  - .specify/templates/plan-template.md → ✅ no change required; the
    "Constitution Check" block instructs /speckit.plan to enumerate
    principles dynamically from this file (comment block at lines 35-48).
  - .specify/templates/requirements-template.md → ✅ no hard principle
    references found; /speckit.requirements resolves Feature binding at
    runtime, no edit required.
  - .specify/templates/tasks-template.md → ✅ only references the
    constitution for the Tests Mode toggle; no hard principle numbers.
  - README.md → ✅ no constitution references present.
  - docs/quickstart.md → file does not exist; no update required.

Follow-up TODOs:
  - TODO(RATIFICATION_DATE): the user did not supply an explicit
    ratification date; defaulted to today (2026-08-01). Amend if the
    project stakeholders agree on a different effective date.
  - TODO(QUICKSTART): create docs/quickstart.md once the build pipeline
    for the WASM runtime drivers stabilises, so the Documentation-First
    principle has an onboarding artefact to point at.
-->

# sysbench-WASM Constitution

This constitution governs the customised fork of the open-source
`sysbench` benchmark framework maintained in this repository. The fork
extends sysbench from a database / OS micro-benchmarker into a
general-purpose harness for benchmarking external systems, with the
current focus on **WebAssembly runtimes** (Wasmtime, Wasmer, WAMR,
WasmEdge, and others as they are added). Where this constitution is
silent, upstream sysbench conventions apply; where it conflicts with
upstream convention for the forked code paths, this constitution
prevails inside this repository.

## Core Principles

### I. Documentation-First
Documentation is a first-class deliverable and MUST take priority over
test coverage:

- The project MUST maintain sufficiently detailed and accurate
  documentation so it can serve as reliable context for AI agents /
  large language models, supplying project knowledge and background.
- Documentation MUST NOT record implementation details; those belong in
  the code itself.
- Documents MUST stay focused and reasonably sized; when a document
  grows too complex it MUST be split into smaller, cohesive documents.
- Documents MUST cross-reference one another so that basic navigation
  can be accomplished purely through internal links.
- Markdown documents MUST maintain basic metadata (title,
  purpose/summary, status, last-updated date, related links).

Rationale: high-quality documentation is the primary knowledge context
that lets collaborators — human or agent — understand a C/Lua project
whose behaviour is split across runtime drivers, Lua scripts, and
build-time configuration.

### II. Code as the Single Source of Truth
Source code is the authoritative source of truth for the project's
actual state; documentation describes intended/target behaviour that
may not yet be realised.

- When establishing or citing facts about how the system currently
  behaves, code MUST take precedence over documentation, unless a
  document is explicitly designated as authoritative for that fact.
- When code and documentation disagree, treat the divergence as a
  signal to update the documentation (or flag the code as
  not-yet-implementing the intended goal), not to trust the document
  as current reality.

Rationale: sysbench's observable behaviour (timers, histograms, thread
scheduling, WASM foreign-function bindings) is defined in C; drift
between docs and code has historically caused misreported benchmark
numbers, so code wins.

### III. Documentation Naming & Location Conventions
- ALL-CAPS Markdown filenames are RESERVED for conventional,
  ecosystem-recognized root-level artefacts (e.g. `README.md`,
  `LICENSE`, `CHANGELOG.md`, `CONTRIBUTING.md`); ordinary content
  documents use lowercase `kebab-case.md` and MUST NOT squat these
  names.
- A document's meaning derives from its FULL PATH, not just its
  filename: place docs so that `<area>/<topic>.md` reads as "the
  <topic> of <area>" (e.g. `docs/wasm/wasmtime-driver.md`,
  `docs/design/extensibility.md`), reusing generic filenames scoped
  by directory rather than inventing globally-unique names.
- Tool/framework-mandated filenames are NON-NEGOTIABLE and MUST match
  the exact required pattern and location (e.g. GitHub Copilot
  prompts MUST be `.github/prompts/<name>.prompt.md`); such names
  MUST NOT be renamed to fit project conventions.

Rationale: the repo already mixes auto-generated tooling directories
(`.github/`, `.specify/`, `.qoder/`) with hand-written sources; path-
scoped naming prevents collisions and keeps `grep`-based discovery
reliable.

### IV. Feature-Centric Development
Feature is the long-term core framework of the project:

- The Feature list MUST remain the single source of truth for what
  the project does.
- Every phase of spec → plan → tasks → implement MUST review Feature
  additions / merges / splits / deletions.
- Feature changes MUST be traceable to corresponding spec/plan
  evidence and recorded in the Feature detail.

Rationale: the fork adds features that cut across the C core, the Lua
layer, and per-runtime drivers (e.g. "Wasmtime backend", "WAMR AOT
mode support"); tracking them as Features — rather than as scattered
tickets — keeps evolution coherent.

### V. Modular Runtime-Backend Architecture
Every WASM runtime integration MUST be implemented as an independent
driver module that conforms to the shared `sb_wasm.h` interface:

- Each driver (`sb_wasmtime.c`, `sb_wasmer.c`, `sb_wamr.c`,
  `sb_wasmedge.c`, …) MUST compile cleanly in isolation and MUST NOT
  reference symbols from another runtime's driver.
- Drivers MUST be conditionally compiled via `configure.ac` /
  `--with-<runtime>` flags, so a build can omit a runtime whose SDK
  is unavailable without breaking the tree.
- Shared helpers (address conversion, memory marshalling, call-frame
  packing) MUST live in `sb_wasm.c` / `sb_wasm.h`, never duplicated
  inside a driver.

Rationale: isolating runtimes keeps benchmark numbers attributable to
the runtime under test and lets contributors add a new runtime without
touching existing ones.

### VI. Measurement Integrity
Benchmark results are the product; anything that silently distorts them
is a correctness bug:

- Timing instrumentation MUST use `sb_timer_*` primitives; ad-hoc
  `clock_gettime` / `gettimeofday` calls in driver code are FORBIDDEN
  unless the core timer cannot reach the call site, and the deviation
  is documented in the driver's `.md`.
- Histograms and counters MUST be updated through `sb_histogram_*` /
  `sb_counter_*`; raw `printf` of latencies inside hot paths is
  FORBIDDEN.
- Any change that alters reported throughput or latency by more than
  1% on a stable reference workload MUST be accompanied by a before/
  after measurement in the commit description or the linked spec.

Rationale: sysbench's value — and the fork's reason for existing — is
trustworthy measurement; silent drift is the hardest defect to find.

### VII. Extensibility for External Systems
The framework MUST remain general-purpose even while current work
focuses on WASM runtimes:

- New external-system integrations MUST be added as drivers behind an
  existing or newly-defined `sb_<subsystem>.h` interface; they MUST
  NOT fork the `sysbench` main loop.
- Lua-facing APIs MUST preserve the hook contract documented in
  upstream sysbench so that user-supplied `.lua` scripts written for
  the vanilla tool keep working.
- When a feature is WASM-specific, its public surface MUST be named
  with a `wasm_` / `wasm-` prefix so it does not pollute the generic
  benchmark namespace.

Rationale: protecting the generic core is what makes this a framework
and not a one-off WASM benchmark suite.

### VIII. Better-Harness Orientation
The project treats itself as a harness for AI agent work — an
environment in which an agent can understand the task, execute on
supported and repeatable paths, validate its changes, deliver safely,
and carry lessons forward. Improvement work MUST be oriented toward
making that harness better:

- Locate and motivate improvements against the five Agent Work Loop
  dimensions (Task Understanding, Controlled Execution, Change
  Validation, Reliable Delivery, Learning Capture); the canonical
  goal model is `.specify/shared/guidelines/better-harness.md` —
  reference it, do not restate it.
- Evidence discipline governs improvement claims: a configured asset
  proves at most that a mechanism exists (configured ≠ used);
  unobserved evidence MUST NOT be treated as a defect or a conclusion;
  "improved" MUST only be claimed from comparable before/after
  evidence.
- This principle adds orientation, not machinery: it MUST NOT justify
  new scoring systems, maturity reports, or tracking/recording
  engines.

Rationale: as agents take on more of the implementation work on this
fork, the quality of the surrounding harness (docs, build scripts,
test fixtures, driver scaffolding) determines whether their output is
trustworthy or merely fast.

## WASM Runtime Harness Standards

Operational standards that concretise Principles V–VII for the WASM
drivers currently in the tree:

- **Build reproducibility.** Each runtime's SDK version MUST be pinned
  in `scripts/<runtime>/env.rc` (or equivalent) and referenced by
  `scripts/build.sh`; ad-hoc `PATH` / `LD_LIBRARY_PATH` tweaks in
  developer shells MUST NOT be required to reproduce a release build.
- **Address-conversion contract.** The `wasm addr convert` helper
  introduced on the `wasm` branch is a shared service; drivers MUST
  use it rather than rolling per-driver pointer marshalling.
- **Bundled workloads.** Reference WASM modules used for benchmarking
  MUST live under `src/wasm/` with a `Makefile.am` rule that rebuilds
  them from source; pre-built `.wasm` binaries checked in without
  source are FORBIDDEN.
- **Failure modes.** A driver MUST degrade gracefully when its runtime
  library is missing at runtime (clear error message, non-zero exit),
  never segfault.

## Development Workflow

- **Branching.** Feature work lands on topic branches (`wasm`,
  `wasmtime-<topic>`, …) and is merged via pull request; `master`
  reflects the last agreed stable state of the fork.
- **Review gates.** Every PR that touches a driver MUST be reviewed
  against Principles V, VI, and VII; every PR that touches
  documentation MUST be reviewed against Principles I and III.
- **Testing.** Core C utilities follow a test-first discipline where
  the `src/tests/` harness can reach them; driver-level benchmarks
  are validated by running the bundled workloads end-to-end in CI
  where the runtime SDK is available, and skipped with an explicit
  `XFAIL` reason where it is not.
- **Commit hygiene.** Commit messages MUST cite the Feature ID they
  advance (when one exists) and MUST NOT bundle unrelated changes.

## Governance

- This constitution supersedes informal conventions, chat decisions,
  and README notes wherever they conflict.
- Amendments require:
  1. A proposed diff to `.specify/memory/constitution.md` reviewed in
     a PR (or an explicit `/speckit.constitution` invocation recorded
     in project memory).
  2. A version bump according to the scheme below.
  3. Propagation to every template that cites the changed principle
     (see Sync Impact Report at the top of this file).
- Versioning scheme: `MAJOR.MINOR.PATCH.DAILY`
  - MAJOR — rewrite-level restructuring or removal of a core
    principle.
  - MINOR — adding, removing, or materially re-scoping a principle.
  - PATCH — wording clarifications, typo fixes, non-semantic edits.
  - DAILY — counter that increments on every update regardless of
    magnitude; resets to `1` when MAJOR/MINOR/PATCH bumps.
- Compliance review: any contributor MAY call `/speckit.plan` or
  `/speckit.review` to verify an in-flight change against the current
  constitution; the result is advisory but SHOULD be addressed before
  merge.

**Version**: 1.0.0.1 | **Ratified**: 2026-08-01 | **Last Amended**: 2026-08-01
