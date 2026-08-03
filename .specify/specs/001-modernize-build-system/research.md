# Phase 0 Research: Build System Selection

**Requirement**: `001-modernize-build-system` → Feature 019 Modern Build System  
**Date**: 2026-08-01  
**Status**: Complete — all Technical Context unknowns resolved

## Purpose

The requirements spec deliberately named no replacement build system (user decision recorded in
`requirements.md` § Clarifications). This document supplies the evidence for that choice and for
the four subsidiary mechanism decisions the port depends on.

## Corrected Baseline

Re-measured directly during this phase; the spec's earlier estimate has been corrected per
Constitution Principle II (code over documentation).

| Metric | Earlier estimate | **Measured** | Method |
|---|---|---|---|
| Total autotools lines | 5,003 | **5,074** | `wc -l` over `configure.ac`, `autogen.sh`, 18 `Makefile.am`, 21 `m4/*.m4` |
| Project-authored (porting surface) | ~1,334 | **1,905** | 496 + 3 + 614 + 792 |
| Vendored `m4` (deleted, not ported) | ~3,072 | **3,169** | 13 files |

**Why this matters**: 62% of the footprint is vendored macro code that is *deleted outright*
because both candidate systems have native equivalents — `acx_pthread.m4` (277),
`ax_tls.m4` (72), `ax_gcc_func_attribute.m4` (239), `host-cpu-c-abi.m4` (530),
`lib-link.m4` (710), `lib-prefix.m4` (335), `pkg.m4` (353). The migration is smaller than the
5,074 headline implies, but the hand-ported part is larger than first stated.

## Decision 1: Meson (over CMake)

**Decision**: Meson ≥ 1.3 with the Ninja backend.

**Rationale — the decisive factor is bundled-dependency incrementality.**

The project's stated pain is that bundled dependencies rebuild every time:
`third_party/luajit/Makefile.am:27` runs `$(MAKE) -C $(srcdir)/luajit clean` unconditionally,
then tar-copies the tree, on every single build. Both candidates were evaluated on whether they
can fix this:

| Mechanism | Incremental? | Verdict |
|---|---|---|
| CMake `ExternalProject_Add` + `BUILD_ALWAYS ON` | No — rebuilds every time | Reproduces the current bug exactly |
| CMake `ExternalProject_Add` + `BUILD_ALWAYS OFF` | No — stamp-based, misses source edits | Produces stale artefacts (silent wrong binary) |
| CMake `FetchContent` | N/A | Configure-time `add_subdirectory()`; requires rewriting LuaJIT's build in CMake |
| CMake `add_custom_command(OUTPUT … DEPENDS …)` | Yes | Correct, but needs a hand-maintained LuaJIT source list |
| Meson `external_project` | **Yes** | Emits a real depfile by walking the dependency source tree |
| Meson `custom_target` + `depend_files` | **Yes** | Stable API; correct incrementality |

CMake documents the dilemma explicitly: `BUILD_ALWAYS` exists *because* with local source paths
CMake "cannot detect manual alterations". There is no third state. Meson's `external_project`
implementation writes a depfile enumerating every file under the dependency's source tree, so
Ninja re-invokes the sub-make only when something actually changed — no hand-maintained list.

**The factor the spec expected to be decisive was not.** The spec weighted distro packaging
heaviest. Investigation shows it does not discriminate:

| Consumer | CMake | Meson |
|---|---|---|
| Debian | `cmake.pm` (auto-detected) | `meson.pm` (auto-detected; passes `--buildtype=plain --prefix=/usr --libdir=lib/$multiarch --wrap-mode=nodownload`) |
| Fedora/RPM | `%cmake` / `%cmake_build` / `%cmake_install` | `%meson` / `%meson_build` / `%meson_install` / `%meson_test` |
| snapcraft | `cmake` plugin (tools auto-supplied) | `meson` plugin (needs meson+ninja in `build-packages` — 2 extra lines) |

Both are first-class everywhere. `debian/rules` currently overrides `dh_auto_configure` to run
`autoreconf -vif`; either choice lets that override be **deleted** and autodetection take over.
The only asymmetry is ~2 lines of snapcraft YAML, which does not outweigh a structural
incrementality advantage.

**Secondary factors**: Meson's typed DSL (real bools/lists/dicts) is measurably terser than
CMake's stringly-typed language for the same work — relevant to FR-010/SC-004. `generator()` is
purpose-built for the 6 embedded-header rules. Config-header generation is a genuine tie.

**Where CMake would have won**: WASI-SDK ships maintained CMake toolchain files
(`wasi-sdk-p1.cmake` et al.) and no Meson cross file. This is real but modest — see Decision 3.

**Rejected alternatives**:
- **CMake** — retained as the documented fallback. Adopt it if `external_project`'s experimental
  status becomes unacceptable; Decision 2 already avoids depending on it.
- **Bazel** — hermetic-build machinery far exceeds the need; no distro packaging story; would
  add more concept count than Autotools removed.
- **Plain Ninja / hand-written Makefiles** — no configuration or feature-detection layer, so the
  22-option surface and `config.h` generation would have to be hand-rolled. Trades one bespoke
  system for another.

**Precedent**: MySQL migrated Autotools→CMake (WL#5161) citing m4 complexity and unified
`config.h`. PostgreSQL adopted Meson citing autoconf "aging" and — directly on point —
"the recursive compilation method suffered from **sluggish incremental updates**". Notably
PostgreSQL kept Autotools alongside; this project's user has mandated full retirement, so that
part of the precedent does not transfer. GNOME/GTK/GLib/GStreamer and systemd dropped Autotools
for Meson. No benchmark-harness precedent was found — recorded as absent rather than stretched.

## Decision 2: Bundled dependencies stay in-tree via `custom_target`

**Decision**: LuaJIT and Concurrency Kit remain vendored in `third_party/`, wrapped by
`custom_target` with `depend_files`. `external_project` is a later optimization, not a
prerequisite.

**Rationale**: This resolves the spec's remaining open decision (Story 4 / FR-007) while
changing nothing about what ships. `custom_target` + `depend_files` is **stable** API and
already gives the correct incrementality that fixes the stated pain — so the port does not
depend on an experimental module. Both deps are linked **statically** from explicit `.a` paths
(`m4/sb_luajit.m4:44`, `m4/sb_concurrency_kit.m4:44`), and both remain selectable as system
libraries via pkg-config, preserving `--with-system-luajit` / `--with-system-ck`.

**Critical constraint discovered**: `third_party/luajit/Makefile.am:27` runs `make clean` **in
`$(srcdir)`** — it writes to the source tree. This violates FR-009 (out-of-tree builds must not
touch the source tree). The Meson wrapper must copy-then-build without ever cleaning the
original, which also removes the need for the unconditional clean.

**Rejected**: git submodules (adds clone-time friction and a fetch dependency for a source tree
that already works); a package manager (no distro ships LuaJIT 2.1.0-beta3 consistently, and CK
packaging is sparse) — both were rejected as changing contributor workflow for no build-quality
gain.

## Decision 3: WASI-SDK guests via `custom_target`

**Decision**: Compile the 7 `.wasm` guest modules with a `custom_target` invoking
`$WASI_SDK_HOME/bin/clang` directly.

**Rationale**: The guests are standalone artefacts with **no host link dependency**, so a full
second toolchain context is unnecessary. Meson supports two compiler contexts per language
(build + host); wasm would be a third, so a cross file cannot express it — but it does not need
to. A `custom_target` per module is ~10 lines, fully incremental, and needs no cross file.

**Defect found — must fix, not preserve**: `src/wasm/Makefile.am:41-45` uses **space
indentation instead of tabs** (verified with `cat -A`). Make requires tab-indented recipes, so
those rules are dead: the guests are **not currently built by WASI-SDK at all**. Porting
"existing behaviour" would have ported a no-op. The Meson port implements what was *intended*.
Additionally, the guests are declared `bin_PROGRAMS`, which would install `.wasm` files into
`bindir` — corrected per FR-020.

## Decision 4: Embedded headers via `generator()` + a portable script

**Decision**: Replace the two `sed`-based suffix rules with `scripts/embed-file.py`, driven by a
Meson `generator()` applied to 5 Lua files and 1 Python file.

**Rationale**: The existing rules in `src/lua/internal/Makefile.am:25-36` and
`src/python/Makefile.am:23-34` are byte-identical to each other and use
`sed -e 's/$$/\\n"/g'`, whose `$` handling differs between BSD and GNU sed — a portability
hazard given macOS is a supported target (FR-019). A small script is portable, testable, and
produces byte-identical output. `generator()` declares the rule once for all 6 inputs with
correct dependency tracking. Output contract preserved exactly: `unsigned char <var>[] = …;`
plus `size_t <var>_len = sizeof(<var>) - 1;` with dots in the filename mapped to underscores.

## Decision 5: `config.h` via `configuration_data`

**Decision**: Generate `config.h` with `cc.has_header` / `cc.has_function` / `cc.sizeof` /
`cc.compiles` driving a `configuration_data` object, iterated over lists.

**Rationale**: The current checks (17 headers, 15 functions, 2 sizeof, 2 decls, TLS keyword, 2
GCC attributes, ~20 `AC_DEFINE`s) collapse into loops of roughly 30 lines, replacing ~60 lines
of `AC_CHECK_*` **and** deleting ~1,100 lines of vendored macros that exist only to perform
these checks. Meson's `cc.get_supported_arguments()` also replaces the
`AX_CHECK_COMPILE_FLAG` dance for the sanitizer flags, and `b_sanitize=address|memory` is
built in, removing the hand-rolled `ASAN_CFLAGS`/`MSAN_CFLAGS` plumbing entirely.

## Unresolved / Flagged

- **No current rigorous C-specific CMake-vs-Meson benchmark was found.** The syntax and
  learning-curve comparisons are consensus-shaped, not measured. The load-bearing evidence
  (incrementality mechanisms, packaging integration) comes from primary docs and source, and is
  solid. Recorded rather than overstated.
- **WASI-SDK's lack of a Meson cross file** was confirmed only by non-discovery. Immaterial:
  Decision 3 needs no cross file.
- **`external_project` is experimental.** Deliberately routed around by Decision 2.
- **Timing baseline not yet captured.** This is the single blocking prerequisite — SC-001–SC-003
  are unverifiable without it. Enforced by contract C-14 and gate G-1.

## References

Meson: [External Project module](https://mesonbuild.com/External-Project-module.html) ·
[custom_target](https://mesonbuild.com/Reference-manual_functions_custom_target.html) ·
[Generating sources](https://mesonbuild.com/Generating-sources.html) ·
[Configuration](https://mesonbuild.com/Configuration.html) ·
[Cross-compilation](https://mesonbuild.com/Cross-compilation.html) ·
[Porting from autotools](https://mesonbuild.com/Porting-from-autotools.html)

CMake: [ExternalProject](https://cmake.org/cmake/help/latest/module/ExternalProject.html) ·
[FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html) ·
[add_custom_command](https://cmake.org/cmake/help/latest/command/add_custom_command.html)

Packaging: [dh_auto_configure](https://manpages.debian.org/testing/debhelper/dh_auto_configure.1.en.html) ·
[debhelper buildsystems](https://sources.debian.org/src/debhelper/latest/lib/Debian/Debhelper/Buildsystem/) ·
[Fedora Meson guidelines](https://docs.fedoraproject.org/en-US/packaging-guidelines/Meson/) ·
[Fedora CMake guidelines](https://docs.fedoraproject.org/en-US/packaging-guidelines/CMake/) ·
[snapcraft meson plugin](https://ubuntu.com/docs/snapcraft/8/common/craft-parts/reference/plugins/meson_plugin/)

Precedent: [PostgreSQL wiki: Meson](https://wiki.postgresql.org/wiki/Meson) ·
[MySQL WL#5161](https://dev.mysql.com/worklog/task/?id=5161) ·
[WebAssembly/wasi-sdk](https://github.com/WebAssembly/wasi-sdk)
