---
id: "20260803T030023Z-build-system-framework-round-implemented-39-47-tasks-8-defer"
scope: "session"
source: "/speckit.implement"
feature: "001-modernize-build-system"
tags: ["implement", "build", "meson", "framework", "deferred", "sc004-shortfall"]
title: "Build-system framework round implemented (39/47 tasks, 8 deferred)"
created: "2026-08-03T03:00:23Z"
summary: "Closed 39 of 47 tasks; 8 pre-deferred (T022,T023,T025,T028,T030,T045,T046,T047). Zero open. All 6 completion gates PASS. Feature 019 stays Planned — the Pre-Status-Flip Gate is not satisfied because S"
---

Closed 39 of 47 tasks; 8 pre-deferred (T022,T023,T025,T028,T030,T045,T046,T047). Zero open. All 6 completion gates PASS. Feature 019 stays Planned — the Pre-Status-Flip Gate is not satisfied because SC-001..SC-003,SC-006,SC-007,SC-009,SC-011,SC-012 are deferred (meson/ninja/SDKs absent) and SC-004 is partial (line budget missed).

DELIVERED:
- 10 meson.build/meson_options.txt files (1060 total lines, 693 code-only) replacing configure.ac + 18 Makefile.am + 21 m4 macros (5074 lines)
- scripts/embed-file.py: byte-identical to sed rules for all 6 embedded headers (contract C-7, execution-verified)
- scripts/build-bundled.sh: out-of-tree bundled dep builds — executed for real, produced 970KB/70-object LuaJIT archive, source tree stayed pristine (BD-1/C-12 verified)
- 4 contract tests under tests/build/
- All 7 consumers migrated: ci.yml (+ new config-matrix job), Dockerfile (rebased ubuntu:24.04), debian/rules, rpm/sysbench.spec, snap/snapcraft.yaml.in, scripts/build.sh (master-gate defect fixed), README.md
- .travis.yml deleted by user decision (dead since 2021)
- docs/build/ created: overview.md, dependencies.md, migration-status.md

CONTRACT TEST RESULTS:
- test_option_parity.py: PASS (22/22 = 15 project + 6 builtin + 1 drop)
- test_dependency_docs.py: PASS (8/8 SDKs documented, WASI_SDK_HOME wired)
- test_no_vendored_macros.py: FAIL by design — SC-004 line budget missed (1060 vs 762; code-only 693/36.4% would meet it; vendored-macro half PASSES)
- test_consumer_refs.py: FAIL by design — 2 live refs remain (Makefile.am, autogen.sh), both deleted by T047

TWO ARTEFACT ERRORS FOUND AND FIXED:
1. contracts/option-mapping.md double-counted option 5 (--enable-largefile) as both project option and builtin; corrected 16/5 -> 15/6/1
2. .travis.yml disposition decided (delete) — was dead since 2021, superseded by GH Actions 2023

SC-004 SHORTFALL (the one material miss): 1060 authored lines vs 762 target (40% of 1905). Code-only is 693 (36.4%). The 367 non-code lines are comments citing file:line evidence for each defect fixed. Recorded as partial with 3 resolution options: (a) amend SC-004 to measure code-only, (b) move commentary to docs/build/, (c) accept 55.6% reduction.

NEXT: rebuild base image with meson+ninja+WASI-SDK+4 WASM SDKs (Dockerfile is the vehicle), then run the 8 deferred tasks in order: T022 -> T023 -> T025 -> T028 -> T030 -> T045 -> T046 -> T047 (delete Autotools in a single isolated commit).
