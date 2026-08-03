---
id: "measured-baseline-inverts-spec-premise-autotools-under-build"
scope: "knowledge"
source: "/speckit.tasks"
feature: "001-modernize-build-system"
tags: ["decision", "build", "baseline", "defect", "measurement", "correction"]
title: "MEASURED baseline inverts spec premise: autotools under-builds (stale), and the tree does not build as-shipped"
created: "2026-08-01T17:23:44Z"
summary: "Measured on an isolated copy of the tree, 2026-08-02, 192-core x86_64 Linux, gcc. These supersede the spec's qualitative claims."
---

Measured on an isolated copy of the tree, 2026-08-02, 192-core x86_64 Linux, gcc. These supersede the spec's qualitative claims.

FINDING A -- THE TREE DOES NOT BUILD AS SHIPPED. Two independent blockers:
1. 'sh autogen.sh' FAILS. automake runs with -Werror (AM_INIT_AUTOMAKE([-Wno-portability -Wall -Werror foreign]), configure.ac:31) and src/wasm/Makefile.am:41 emits "warning: '%'-style pattern rules are a GNU make extension", which -Werror escalates to a fatal error. autoreconf exits 1.
2. After bypassing that, './configure' FAILS with: conditional "HAVE_WAMR" was never defined. Root cause: the four per-runtime AM_CONDITIONALs live INSIDE AX_CHECK_WAMR/WASMEDGE/WASMER/WASMTIME, which SB_WASM (m4/sb_wasm.m4:227-234) invokes only when --with-wasm is passed. Without --with-wasm they are never defined, and automake requires AM_CONDITIONAL to be unconditional. So the DEFAULT configure path is broken.
VERIFIED FIX for baseline purposes: drop -Werror from AM_INIT_AUTOMAKE, and hoist the 4 AM_CONDITIONALs into SB_WASM with ac_cv_enable_<rt>=no defaults. After both, configure exits 0 and 'make -j192' exits 0 producing a working ./src/sysbench (reports 'sysbench 1.1.0-598928f').

FINDING B -- THE SPEC'S EFFICIENCY PREMISE IS WRONG IN DIRECTION. requirements.md claims 'Bundled dependencies are fully rebuilt every time'. FALSE. third_party/luajit/Makefile.am:25 declares the rule as '$(builddir)/lib/libluajit-5.1.a:' with NO PREREQUISITES. Consequences measured:
- .a present => LuaJIT is NEVER rebuilt. Touching third_party/luajit/luajit/src/lj_api.c produced ZERO compiles and left the .a mtime unchanged. This is a STALE-ARTIFACT bug (under-build), not an always-rebuild bug (over-build).
- .a absent => full rebuild, and only then does the unconditional 'make clean' fire (5s).
So the real defect is silent staleness: a LuaJIT source edit does not reach the binary. That is a CORRECTNESS problem, strictly worse than the slowness the spec described.

FINDING C -- MEASURED TIMINGS (median of 3, warm cache, --without-mysql, no wasm):
- Full clean build: 6s
- No-op rebuild: 55ms (54/55/56)
- Single-C-file rebuild (touch src/sb_timer.c): 178ms, exactly 1 compile + link
The build is NOT slow in absolute terms. Recursive-make serialization is invisible at this scale on 192 cores.

CONSEQUENCE FOR SUCCESS CRITERIA -- SC-001 and SC-002 are likely UNACHIEVABLE as written:
- SC-001 'no-op rebuild < 10% of baseline' => < 5.5ms. Ninja's own no-op floor for a project this size is typically 10-50ms. Probably impossible.
- SC-002 'single-file rebuild < 25% of baseline' => < 44ms, but the single gcc invocation dominates the 178ms and is compiler-bound, not build-system-bound. Probably impossible.
- SC-003 'full build <= baseline (6s)' => feasible.
- SC-004 '<= 762 project-authored lines' => feasible and remains the strongest win.
RECOMMENDATION: renegotiate SC-001/SC-002 with the user against measured data, and re-anchor the migration's justification on readability (1,905 -> <=762 lines) and CORRECTNESS (stale artifacts, silently-ignored options, dead WASM recipes) rather than raw speed.

ENVIRONMENT PROBE (this machine): meson ABSENT, ninja ABSENT, libtool wrapper ABSENT but libtoolize PRESENT (autoreconf works), valgrind ABSENT, debuild/rpmbuild/snapcraft ABSENT, docker present, mysql_config + pg_config + python3 present, all 4 WASM SDKs and WASI_SDK ABSENT with all *_HOME unset. Implication: WASM-dependent and packaging-dependent tasks cannot be executed locally and must be marked deferral-eligible.
