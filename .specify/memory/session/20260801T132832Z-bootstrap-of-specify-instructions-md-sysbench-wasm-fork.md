---
id: "20260801T132832Z-bootstrap-of-specify-instructions-md-sysbench-wasm-fork"
scope: "session"
source: "/speckit.instructions"
tags: ["instructions", "bootstrap", "wasm", "convention"]
title: "Bootstrap of .specify/instructions.md (sysbench WASM fork)"
created: "2026-08-01T13:28:32Z"
summary: "Full reconcile with empty $ARGUMENTS. No pre-existing .specify/instructions.md and no backups => clean bootstrap, recovery path (Action 3) skipped."
---

Full reconcile with empty $ARGUMENTS. No pre-existing .specify/instructions.md and no backups => clean bootstrap, recovery path (Action 3) skipped.

CONVERGED (placeholders filled): Project Overview (fork purpose + per-runtime maturity table); Documentation Map (CONTRIBUTING.md and docs/ verified ABSENT and marked so; added Feature Details and m4/sb_wasm.m4 rows; recorded verified feature count 18); Tech Stack (autotools/C/LuaJIT 2.1.0-beta3/cram, real key dirs).
ADDED (non-template, project-specific): 'Architecture: How a Benchmark Runs' (two-level dispatch: sb_test_t/sb_operations_t then sb_wasm_runtime vtable; single-int64 carrier call convention), 'Developer Workflows' (verified configure/make/make check invocations), 'Project-Specific Conventions & Pitfalls'.
TOLERATED (untouched): Fact/Logic Checks, Task Complexity Rubric, Dogfooding Practice, Tooling Scope, AI Tool Compatibility, Symlink Model, all 3 registry ranges (markers intact).

Self-corrections during the run: retracted an unverified C99 claim (configure.ac only has AC_PROG_CC), pinned LuaJIT to 2.1.0-beta3 from luajit.h, and fixed a wrong regen instruction (regen-command-copies.py sources repo-root templates/commands/ which does not exist here, so regeneration is NOT runnable in this project).

Glossary seeded with 14 auto/proposed domain terms. All 7 compatibility symlinks verified write-through (257 lines each).
