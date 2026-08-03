# Verification Log — 001-modernize-build-system

# -- Baseline (recorded once, BEFORE any /speckit.implement work changes the tree) --

baseline_commit=598928fcf6135b892e2844797a39093b1b929e84
baseline_date=2026-08-02
baseline_branch=001-modernize-build-system

# Machine identity for all timing measurements
baseline_machine_cores=192
baseline_machine_arch=Linux x86_64
baseline_machine_compiler=cc (Ubuntu 15.2.0-16ubuntu1) 15.2.0

# Autotools line-count baseline (re-verified 2026-08-02 with fresh command output)
baseline_configure_ac_lines=496
baseline_autogen_sh_lines=3
baseline_makefile_am_lines=614
baseline_makefile_am_files=18
baseline_project_m4_lines=792
baseline_project_authored_total=1905
baseline_vendored_m4_lines=3169
baseline_vendored_m4_files=13
baseline_autotools_grand_total=5074
baseline_sc004_target_lines=762

# Timing baseline (median of 3, warm cache, --without-mysql, isolated scratch copy
# with the two build blockers patched — see notes)
baseline_full_clean_build=6s
baseline_noop_rebuild=55ms
baseline_single_file_rebuild=178ms
baseline_single_file_compiles=1

# Baseline build health — the tree does NOT build as shipped
baseline_autogen_status=FAIL (automake -Werror escalates src/wasm/Makefile.am:41 GNU-pattern-rule warning)
baseline_configure_status=FAIL (conditional "HAVE_WAMR" was never defined)
baseline_luajit_incrementality=BROKEN — rule has no prerequisites; touching lj_api.c produced 0 compiles, archive mtime unchanged

# Downstream consumer baseline
baseline_autotools_consumer_files=9

# -- /speckit.implement results --

implementation_date=2026-08-02
post_change_commit=see git log for this branch

post_change_project_authored_total=1060
post_change_project_authored_code_only=693
post_change_meson_files=10
post_change_helper_scripts=2
post_change_vendored_macro_files_required=0

# -- Success Criteria evaluation --

SC-001_status=deferred
SC-001_value=baseline 55ms recorded; Meson figure not measurable
SC-001_note=No-op rebuild non-regression criterion. Requires meson+ninja to measure.
SC-001_deferred_reason=meson and ninja absent from image; unblocked by base-image rebuild (T033)

SC-002_status=deferred
SC-002_value=baseline 178ms / 1 compile recorded
SC-002_note=Single-file rebuild non-regression criterion. Requires meson+ninja.
SC-002_deferred_reason=meson and ninja absent from image; unblocked by base-image rebuild (T033)

SC-003_status=deferred
SC-003_value=baseline 6s recorded
SC-003_note=Full clean build must not exceed baseline. Requires meson+ninja.
SC-003_deferred_reason=meson and ninja absent from image; unblocked by base-image rebuild (T033)

SC-004_status=partial
SC-004_value=1060 total lines (55.6% of 1905) / 693 code-only lines (36.4%); 0 vendored macro files required
SC-004_note=SPLIT RESULT. Vendored-macro independence: PASS — 0 of 13 vendored m4 files (3169 lines) are needed for dependency discovery; all of it is deleted, not ported. Line budget: MISSED — 1060 authored lines vs the 762 target (40% of 1905). Code-only is 693 lines (36.4%), which WOULD meet the budget, but the criterion counts total lines and was not renegotiated, so this is recorded as a shortfall rather than met. The 367 non-code lines are comments citing the specific defects each construct fixes (file:line references for the stale-artefact bug, the dead space-indented WASM recipes, the silently-ignored master gate). Trimming them to reach the number would remove the evidence that justifies the migration and would violate Constitution Principle II's preference for verifiable claims. Gate G-3 permits proceeding with a recorded shortfall.
SC-004_shortfall_options=(a) accept as-is and amend SC-004 to measure code-only lines; (b) move the defect commentary into docs/build/ and cite it from the meson files, reducing total lines while preserving evidence; (c) accept the 55.6% reduction as sufficient. Requires a user decision.

SC-005_status=pass
SC-005_value=22/22 accounted for = 15 project options + 6 Meson built-ins + 1 recorded drop
SC-005_note=Verified by tests/build/test_option_parity.py (exit 0). Counts are derived from the parsed contract table, not hard-coded. Found and fixed an arithmetic error in contracts/option-mapping.md, which double-counted option 5 (--enable-largefile) as both a project option and a built-in.

SC-006_status=deferred
SC-006_value=12 matrix configs + 6 negative cases
SC-006_note=Requires executing meson setup per configuration.
SC-006_deferred_reason=meson absent; four WASM SDKs absent (T045)

SC-007_status=deferred
SC-007_value=baseline test count not capturable
SC-007_note=Autotools `make check` unreachable because the tree does not configure as shipped.
SC-007_deferred_reason=baseline build broken AND meson absent; unblocked by base-image rebuild

SC-008_status=partial
SC-008_value=all 7 consumers migrated in source; 2 live autotools references remain (Makefile.am:35, autogen.sh:3)
SC-008_note=Consumer migration COMPLETE in source: ci.yml, Dockerfile, debian/rules, rpm/sysbench.spec, snap/snapcraft.yaml.in, scripts/build.sh, README.md all migrated; .travis.yml deleted by user decision (dead since 2021, superseded by GitHub Actions 2023). tests/build/test_consumer_refs.py reports exactly 2 remaining references, both inside the Autotools files that task T047 deletes. Execution verification of the packaging paths is deferred (debuild/rpmbuild/snapcraft absent).

SC-009_status=deferred
SC-009_value=n/a
SC-009_note=Requires an uninvolved contributor to follow docs on a prepared machine.
SC-009_deferred_reason=needs a working build environment and a second person

SC-010_status=partial
SC-010_value=parent-gating and unsatisfiable-request errors authored; 6 negative cases not executed
SC-010_note=meson.build raises a configuration error when a WASM runtime is enabled while the wasm master gate is disabled (closing the m4/sb_wasm.m4:227-234 silent-ignore defect), and src/meson.build + src/drivers/meson.build raise errors naming the component and the unmet dependency for explicitly-enabled-but-unsatisfiable components. Execution of the 6 negative cases requires meson.
SC-010_deferred_reason=meson absent; unblocked by base-image rebuild (T045)

SC-011_status=deferred
SC-011_value=n/a
SC-011_note=Install-parity diff requires both systems to build.
SC-011_deferred_reason=meson absent; Autotools baseline install not reachable as shipped

SC-012_status=deferred
SC-012_value=configure.ac + autogen.sh + 18 Makefile.am + 21 m4 files all still present
SC-012_note=Autotools deletion is explicitly OUT OF SCOPE this round (FR-018, gate G-6).
SC-012_deferred_reason=blocked on gates G-2..G-5; deliberate per 2026-08-02 scope directive

# -- Deferred tasks (mirrors `[~]` rows in tasks.md) --

deferred_tasks=T022,T023,T025,T028,T030,T045,T046,T047
deferred_reason_summary=Framework-only round per user directive 2026-08-02: meson/ninja, all four WASM runtime SDKs, WASI-SDK, valgrind, and debuild/rpmbuild/snapcraft are absent from the image, so every build/test/package EXECUTION task is deferred until the base image is rebuilt. T047 (Autotools deletion) is additionally blocked on gates G-2..G-5.

# -- Free-form notes --

notes=Scope directive (user, 2026-08-02, verbatim in requirements.md § Clarifications): deliver the build-system framework only; rebuild the base image before any compile testing. Two build blockers found during task generation mean the tree does not build as shipped: (1) autogen.sh fails because configure.ac:31 passes -Werror to automake and src/wasm/Makefile.am:41 emits a GNU-pattern-rule warning; (2) configure then fails with `conditional "HAVE_WAMR" was never defined` because the four per-runtime AM_CONDITIONALs sit inside macros SB_WASM invokes only under --with-wasm. The timing baseline was therefore captured in an isolated scratch copy with both blockers patched, then discarded — the Autotools tree itself is untouched. A third finding inverted the spec's premise: third_party/luajit/Makefile.am:25 declares its rule with no prerequisites, so LuaJIT is never rebuilt once the archive exists (measured: 0 compiles, unchanged mtime) — a silent-staleness correctness defect, not the always-rebuild slowness originally assumed. SC-001/SC-002 were consequently renegotiated with the user from sub-10%/sub-25% ratios into non-regression criteria.
