---
id: "20260801T173644Z-tasks-md-generated-for-build-migration-framework-only-round"
scope: "session"
source: "/speckit.tasks"
feature: "001-modernize-build-system"
tags: ["tasks", "build", "meson", "deferred", "scope"]
title: "tasks.md generated for build migration (framework-only round)"
created: "2026-08-01T17:36:44Z"
summary: "Created .specify/specs/001-modernize-build-system/tasks.md: 47 tasks (39 actionable + 8 pre-deferred) in 7 phases. Tests Mode ON. Feature 019 stays Planned (tasks phase does not advance status)."
---

Created .specify/specs/001-modernize-build-system/tasks.md: 47 tasks (39 actionable + 8 pre-deferred) in 7 phases. Tests Mode ON. Feature 019 stays Planned (tasks phase does not advance status).

SCOPE (user directive, verbatim in requirements.md Clarifications Session 2026-08-02): framework authoring only this round; no build/test execution until the base image is rebuilt with the missing dependencies.

PHASE MAP:
- P1 Setup T001-T003: record measured baseline, docs/build/ skeleton, gitignore for build dirs
- P2 Foundational T004-T010: scripts/embed-file.py + byte-identical verification (executable NOW), root meson.build with config.h, meson_options.txt (16 project options), parent-gating fix, version metadata, summary()
- P3 US1 T011-T023: 9 meson.build files + 2 structural contract tests; T022/T023 deferred
- P4 US2 T024-T028: incrementality check script authored; T025/T028 deferred
- P5 US3 T029-T038: 6 consumer migrations all [P] (ci.yml, .travis.yml disposition, Dockerfile, debian/rules, rpm spec, snapcraft), plus scripts/build.sh master-gate fix and README; T030 deferred
- P6 US4 T039-T041: dependency docs + version reporting
- P7 Polish T042-T047: migration-status doc, feature record, verification.md; T045/T046/T047 deferred

THE 8 DEFERRED TASKS and blockers: T022,T023 (meson absent), T025,T028 (meson/ninja absent), T030 (debuild/rpmbuild/snapcraft absent), T045 (meson + all 4 WASM SDKs), T046 (meson), T047 (blocked on all gates; Autotools deletion explicitly OUT OF SCOPE this round per FR-018).

HIGHEST-VALUE EARLY TASK: T005 -- verifies contract C-7 (embed-file.py byte-identical for all 6 headers) using only python3, which IS present. This closes the round's one genuinely risky correctness requirement without needing meson.

SPEC RESTRUCTURED FIRST (per spec-restructure-first rule) before writing tasks: appended Clarifications Session 2026-08-02 with the verbatim user directive; replaced the false 'bundled deps rebuild every time' evidence with the measured stale-artifact finding; added the two build-blocker findings; added the measured timing table; restated SC-001/SC-002 as non-regression criteria and relocated the revision note below SC-012 to keep the SC sequence contiguous.

VALIDATION PASSED: DoD section has zero checkbox-syntax lines; T001-T047 sequential, no gaps/dups, strictly increasing; story labels absent in phases 1/2/7 and present in 3-6; all 26 blockedBy refs resolve to real task IDs; all referenced file paths verified to exist; gate counts confirmed (18 Makefile.am, 21 m4 files).

NEXT: /speckit.analyze (cross-artifact consistency) or /speckit.implement (execute the 39 actionable tasks).
