---
id: "20260801T154900Z-spec-001-modernize-build-system-drafted-first-spec-in-repo"
scope: "session"
source: "/speckit.requirements"
feature: "001-modernize-build-system"
tags: ["requirements", "build", "autotools", "migration", "baseline"]
title: "Spec 001 modernize-build-system drafted (first spec in repo)"
created: "2026-08-01T15:49:00Z"
summary: "Created branch 001-modernize-build-system and .specify/specs/001-modernize-build-system/{requirements.md,checklists/requirements.md}. First spec in the repo, so it also establishes spec conventions (a"
---

Created branch 001-modernize-build-system and .specify/specs/001-modernize-build-system/{requirements.md,checklists/requirements.md}. First spec in the repo, so it also establishes spec conventions (added an Assumptions section, which the template does not define).

MEASURED BASELINE captured in the spec's Context table (source inspection, verified):
- 5,003 total autotools lines: configure.ac 497, 18 Makefile.am 631, m4 3,872 (of which ~3,072 is vendored third-party: lib-link.m4 710, host-cpu-c-abi.m4 530, pkg.m4 353...). Project-authored ~1,334.
- 22 user-facing options (16 --with-*, 6 --enable-*/--disable-*).
- 12 AM_CONDITIONALs consumed by 15 if-blocks in Makefile.am.
- 4-level recursive make: . -> src -> tests -> cpu/fileio/memory/threads/mutex.
- 7 downstream consumers of ./configure (independently re-verified by grep): .github/workflows/ci.yml, .travis.yml, Dockerfile, debian/rules, rpm/sysbench.spec, snap/snapcraft.yaml.in, scripts/build.sh (+ README.md docs).
- Inefficiency evidence: third_party/luajit/Makefile.am:27 runs make clean every build; both LuaJIT and CK are tar-copied to tmp/ because neither supports VPATH; scripts/build.sh:3 forces make clean.

SPEC SHAPE: 4 user stories (two co-equal P1 = readable single-config build, fast incremental rebuild; P2 = preserve all packaging/automation consumers; P3 = simplify dependency acquisition). 20 FRs in 4 groups (capability parity / efficiency / readability / migration). 11 SCs each with a named collection method. 0 NEEDS CLARIFICATION markers. Related Feature intentionally left 'Need clarification' for /speckit.clarify.

DELIBERATELY TECHNOLOGY-AGNOSTIC: user said 'modern build scheme' without naming one, so no replacement (CMake/Meson/Bazel) is named anywhere; the choice is deferred to /speckit.plan via FR-002 and Assumptions.

CRITICAL FOLLOW-UP: SC-001..SC-004 are ratios against a baseline that does not yet exist (timings) — the plan MUST capture the timing baseline on the Autotools tree BEFORE migration work lands, or those criteria are unverifiable (Constitution Principle VI).

OPEN DECISIONS for planning: (1) which concrete build system; (2) whether bundled LuaJIT/CK stay in-tree, become submodules, or move to a package manager; (3) which exotic platforms (Sun Studio, PowerPC, FreeBSD) remain supported.

FEATURE-REGISTRY OVERLAP: maps to no existing feature; Features 009 (WASM Build Configuration) and 016 (Pinned SDK Provisioning) overlap materially — /speckit.clarify must decide bind-vs-create and record the overlap.

USER DECISION: declined recording 4 proposed glossary terms (Autotools, master gate, bundled dependency, build consumer) — revisit after clarify/plan.
