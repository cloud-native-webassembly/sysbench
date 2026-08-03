---
id: "meson-selected-for-build-migration-three-verified-build-fact"
scope: "knowledge"
source: "/speckit.plan"
feature: "001-modernize-build-system"
tags: ["decision", "build", "meson", "autotools", "migration", "defect"]
title: "Meson selected for build migration + three verified build facts"
created: "2026-08-01T17:11:08Z"
summary: "Planning outcome for spec 001-modernize-build-system / Feature 019. All facts below were verified directly against source or by execution."
---

Planning outcome for spec 001-modernize-build-system / Feature 019. All facts below were verified directly against source or by execution.

DECISION: Meson >=1.3 + Ninja replaces Autotools. CMake is the documented fallback.
Decisive factor = bundled-dependency incrementality (the project's actual pain: third_party/luajit/Makefile.am:27 runs 'make clean' unconditionally every build). Meson wrappers rebuild only on real source change; CMake ExternalProject_Add offers only BUILD_ALWAYS=ON (reproduces the current always-rebuild bug) or OFF (stale artefacts) -- there is no correct third state.
IMPORTANT INVERSION: the spec assumed distro packaging would decide this. It does NOT discriminate -- Debian (meson.pm, auto-detected), Fedora (%meson/%meson_build/%meson_install), and snapcraft (meson plugin) all support both. Only asymmetry is ~2 lines of snapcraft build-packages. Do not re-litigate the choice on packaging grounds.
Bundled LuaJIT + Concurrency Kit STAY IN-TREE, wrapped via custom_target + depend_files (stable API) -- deliberately NOT Meson's experimental external_project.

THREE VERIFIED FACTS THAT CHANGED THE DESIGN:

1. CORRECTED BASELINE (supersedes the spec's original figures): 5,074 total autotools lines = configure.ac 496 + autogen.sh 3 + 18 Makefile.am 614 + 21 m4 3,961. Project-authored PORTING SURFACE = 1,905 (496+3+614+792 project m4). Vendored m4 = 3,169 across 13 files (62% -- deleted outright, not ported). Earlier estimate of 5,003/1,334/3,072 was wrong; requirements.md was corrected. SC-004 target is therefore <=762 lines.

2. WASM GUEST MODULES ARE NOT BUILT TODAY. src/wasm/Makefile.am:41-45 uses SPACE indentation where Make requires TABs (verified with cat -A), so those recipes are dead code. The 7 .wasm guests are not compiled by WASI-SDK at all. Any 'preserve existing behaviour' framing for src/wasm is wrong -- the port must implement INTENDED behaviour. Also, guests are declared bin_PROGRAMS and would install into bindir; corrected per FR-020.

3. EMBEDDED-HEADER GENERATION HAS A TRAILING-NEWLINE TRAP (execution-verified). src/python/sysbench.py has NO trailing newline (last byte 0x29). The sed rule therefore emits the closing ';' on the SAME LINE as the final string literal. A naive line-joining reimplementation is byte-identical for all 5 .lua files but DIFFERS on sysbench.py. Verified fix: after joining, strip the final newline when the input does not end with one. Output contract: 'unsigned char <var>[] =' / '  "<escaped>\n"' lines / ';' / 'size_t <var>_len = sizeof(<var>) - 1;' where <var> = filename with '.'->'_', escaping '\'->'\\' then '"'->'\"'.

OTHER BUILD FACTS WORTH KEEPING:
- LuaJIT and Concurrency Kit are linked STATICALLY from explicit .a paths (m4/sb_luajit.m4:44, m4/sb_concurrency_kit.m4:44). All four WASM runtimes link DYNAMICALLY (-liwasm/-lwasmedge/-lwasmer/-lwasmtime with rpath).
- third_party/luajit/Makefile.am:27 writes into $(srcdir) -- violates out-of-tree purity; the replacement must copy-then-build without cleaning the original.
- Option accounting: 22 total = 16 project options + 5 Meson built-ins (largefile, buildtype, b_coverage, b_sanitize, werror) + 1 recorded drop (--with-gcc-arch, capability preserved via -Dc_args=-march=native; already passed as --without-gcc-arch by debian/rules and rpm spec).
- 9 files currently reference autotools entry points (7 consumers + autogen.sh + README.md).

BLOCKING PREREQUISITE: gate G-1 requires capturing the timing/install/test baseline on the Autotools tree BEFORE any meson.build is authored. Once Autotools is deleted the baseline is unrecoverable and SC-001..SC-004 become permanently unverifiable (Constitution Principle VI). G-6 (deletion) must be a single isolated commit so rollback is a clean git revert.
