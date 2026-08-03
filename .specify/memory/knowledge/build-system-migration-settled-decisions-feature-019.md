---
id: "build-system-migration-settled-decisions-feature-019"
scope: "knowledge"
source: "/speckit.clarify"
feature: "001-modernize-build-system"
tags: ["decision", "build", "autotools", "migration", "platform", "preference"]
title: "Build-system migration: settled decisions (Feature 019)"
created: "2026-08-01T16:29:38Z"
summary: "Durable decisions from the 2026-08-01 clarification of spec 001-modernize-build-system. These are user-confirmed and must not be contradicted without explicit re-confirmation."
---

Durable decisions from the 2026-08-01 clarification of spec 001-modernize-build-system. These are user-confirmed and must not be contradicted without explicit re-confirmation.

1. FEATURE BINDING: created Feature 019 'Modern Build System' (Draft) rather than binding to existing features. Rationale: build modernization is whole-project (MySQL/PostgreSQL drivers, LuaJIT, Concurrency Kit, Python, packaging, CI), while Feature 009 'WASM Build Configuration' and Feature 016 'Pinned SDK Provisioning' are both WASM-scoped. Additionally 009 is already Implemented, so binding would have regressed its status, which the registry's never-regress rule forbids. Overlaps with 009 and 016 are recorded in 019 for reconciliation at completion.

2. END STATE = FULL AUTOTOOLS RETIREMENT. Not coexistence. After all 7 consumers migrate and verify, delete configure.ac, autogen.sh, all 18 Makefile.am, and all 21 m4/ files. Autotools is retained for NO purpose (not dev, not packaging, not release). Encoded as FR-018 and SC-012. This is what makes SC-004 (cut project-authored build config to <=40% of 1,334 lines) achievable, since the ~3,072 vendored m4 lines actually disappear.

3. REPLACEMENT TECHNOLOGY DEFERRED TO /speckit.plan. The user explicitly chose not to constrain the choice (CMake, Meson, or other). The spec stays technology-agnostic; the plan must justify its selection against FR-002 (capability parity for all 22 options) and FR-010 (readability/size reduction), and must produce a capability-mapping table with one row per current option before committing.

4. PLATFORM SCOPE = Linux (x86_64 + aarch64) and macOS ONLY. Explicitly DROPPED: Sun Studio compiler support (-xO2/sparc paths), PowerPC cache-line overrides, x86 CPUID-based arch probing (ax_gcc_archflag/ax_gcc_x86_cpuid), and FreeBSD-specific LuaJIT -rdynamic handling. RETAINED: the AArch64 LSE atomics probe in m4/sb_concurrency_kit.m4, since aarch64 stays supported. Justification: CI exercises only ubuntu-20.04/22.04, and no WASM runtime SDK targets Sun Studio. Encoded as FR-019.

STILL OPEN for planning: whether bundled LuaJIT / Concurrency Kit remain in-tree, become git submodules, or move to a package manager (spec Story 4, FR-007).

HARD PREREQUISITE: SC-001..SC-004 are ratios against an Autotools baseline that does not yet exist. The plan MUST capture timing and line-count baselines on the current tree BEFORE any migration work lands, or those criteria are unverifiable (Constitution Principle VI: 'improved' only from comparable before/after evidence).
