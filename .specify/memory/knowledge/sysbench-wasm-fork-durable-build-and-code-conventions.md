---
id: "sysbench-wasm-fork-durable-build-and-code-conventions"
scope: "knowledge"
source: "/speckit.instructions"
tags: ["convention", "decision", "build", "wasm", "pitfall"]
title: "sysbench WASM fork: durable build and code conventions"
created: "2026-08-01T13:28:48Z"
summary: "Durable conventions for this fork, verified against source (Constitution Principle II: code is authoritative)."
---

Durable conventions for this fork, verified against source (Constitution Principle II: code is authoritative).

BUILD
- '--with-wasm' is a MASTER GATE. Per-runtime flags (--with-wamr/--with-wasmedge/--with-wasmer/--with-wasmtime) are silently ignored unless --with-wasm is also passed (m4/sb_wasm.m4:227-234).
- scripts/build.sh OMITS --with-wasm, so it produces a binary with no WASM support. Do not trust it as the build path until fixed (Feature 009).
- Working WASM build: sh autogen.sh && ./configure --without-mysql --with-wasm --with-wamr --with-wasmedge && make -j
- All WASM SDKs plus WASI-SDK are EXTERNAL and UNPINNED; nothing is vendored in third_party/ (only cram, luajit, concurrency_kit). Detection locates a CLI binary and derives include/lib by convention.
- scripts/env.rc expects LLVM_HOME, GCC_HOME, WABT_HOME from the outer shell; unset values degrade silently rather than erroring.
- Tests are cram-based: 46 .t files under tests/t/, single automake TESTS entry tests/test_run.sh; run via 'make check'.

CODE
- Adding a WASM engine means a new src/sb_<engine>.c implementing the sb_wasm_runtime vtable (sb_wasm.h:81-90) plus m4 detection and an AM_CONDITIONAL block. Never edit the generic event loop in sb_wasm.c.
- Host/guest call convention is exactly one int64_t in and one out; buffers travel as a packed 32-bit address + 32-bit size ('carrier'), translated via the sandbox addr_app_to_native pointer.
- Measurement integrity: use sb_timer_*, sb_histogram_*, sb_counter_*. No ad-hoc clock_gettime and no printf of latencies in hot paths (Constitution Principle VI).
- Diagnostics go through log_text(), not printf/fprintf. src/sb_wamr.c is the model (log_text + goto error returning NULL/FAILURE, never abort); src/sb_wasmedge.c violates this in places.
- No prebuilt .wasm may be committed; guest modules always build from source in src/wasm/.

TOOLING
- .specify/scripts/python/regen-command-copies.py sources a repo-root templates/commands/ directory that does NOT exist in this project. Per-tool command copies (.qoder/commands/, .github/prompts/, .claude/commands/) are therefore read-only here and cannot be regenerated.
- 7 compatibility symlinks point at .specify/instructions.md (AGENTS.md, CLAUDE.md, QWEN.md, QODER.md, .github/copilot-instructions.md, .qoder/project_rules.md, .claude/project_rules.md). Edit content freely; never delete-and-recreate the links.
