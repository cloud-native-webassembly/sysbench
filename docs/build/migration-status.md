# Build Migration Status

**Title**: Build Migration Status
**Purpose**: Track progress of the Autotools → Meson migration against its six gates, and record
what remains blocked and why.
**Status**: Active — framework round complete, execution gates blocked on the base image
**Last updated**: 2026-08-02
**Related**: [overview.md](./overview.md) · [dependencies.md](./dependencies.md) ·
[migration gates contract](../../.specify/specs/001-modernize-build-system/contracts/migration-gates.md)

## Round scope

This round delivered the **build-system framework only**, by explicit direction: the container
image lacks the tooling needed to exercise the new build, so it must be rebuilt first. Compilation
and test verification are deferred to a follow-up round.

**Autotools remains authoritative.** Nothing has been deleted.

## Gate status

| Gate | Requirement | Status |
|---|---|---|
| **G-1** | Baseline captured | ✅ **Complete** — line counts, timings, machine identity recorded in `verification.md` |
| **G-2** | Meson build reaches parity | ⏸ **Blocked** — framework authored; `meson` and `ninja` absent |
| **G-3** | Performance criteria met | ⏸ **Blocked** — requires G-2 |
| **G-4** | Consumers migrated | ✅ **Source complete** / ⏸ execution unverified — all 7 migrated in source |
| **G-5** | Install parity verified | ⏸ **Blocked** — requires both systems to build |
| **G-6** | Autotools deleted | ⏸ **Out of scope this round** (FR-018) |

## What was delivered

**Build framework** — 10 files, 1,060 lines (693 code):

| File | Replaces |
|---|---|
| `meson.build` | `configure.ac` (496 lines) + 21 `m4/` macros (3,961 lines) |
| `meson_options.txt` | The 22-option `--with-*`/`--enable-*` surface |
| `src/meson.build` | `src/Makefile.am` |
| `src/tests/meson.build` | `src/tests/Makefile.am` + 5 per-benchmark files |
| `src/drivers/meson.build` | `src/drivers/Makefile.am` + mysql/pgsql files |
| `src/lua/meson.build` | `src/lua/Makefile.am` + `internal/Makefile.am` |
| `src/python/meson.build` | `src/python/Makefile.am` |
| `src/wasm/meson.build` | `src/wasm/Makefile.am` |
| `tests/meson.build` | `tests/Makefile.am` |
| `third_party/meson.build` | Both `third_party/*/Makefile.am` wrappers |

**Helper scripts**: `scripts/embed-file.py` (replaces the `sed` embedding rules, verified
byte-identical), `scripts/build-bundled.sh` (out-of-tree bundled dependency builds).

**Contract tests** — 4 under `tests/build/`:

| Test | Asserts | Result |
|---|---|---|
| `test_option_parity.py` | All 22 capabilities accounted for | ✅ PASS (15 project + 6 builtin + 1 dropped) |
| `test_dependency_docs.py` | Every SDK documented, every `*_HOME` wired | ✅ PASS (8/8) |
| `test_no_vendored_macros.py` | No `m4/` dependency; size budget | ⚠️ PARTIAL — see below |
| `test_consumer_refs.py` | Zero live Autotools refs | ❌ FAIL by design — becomes the G-6 check |
| `check_incremental.sh` | Incrementality properties | ⏸ SKIP (exit 77) — needs `meson` |

**Consumers migrated** (all 7): CI workflow (plus a new configuration-matrix job), Dockerfile
(rebased to ubuntu:24.04 with Meson — the base-image rebuild vehicle), `debian/rules` (autoreconf
override deleted, relies on debhelper autodetection), `rpm/sysbench.spec` (`%meson*` macros),
`snap/snapcraft.yaml.in` (`meson` plugin), `scripts/build.sh` (master-gate defect fixed),
`README.md`. `.travis.yml` was **deleted** — last touched 2021-02-06, superseded by GitHub Actions
in 2023, and Travis ended free OSS support in 2021.

## Known shortfall: SC-004 line budget

SC-004 has two halves. One passes decisively; the other misses.

| Half | Target | Actual | Result |
|---|---|---|---|
| Vendored-macro independence | 0 files needed | **0** (was 13 files / 3,169 lines) | ✅ PASS |
| Line budget | ≤762 total lines | **1,060** total / 693 code-only | ❌ MISSED |

Code-only is 693 lines (36.4% of the 1,905 baseline), which *would* meet the 40% budget. The
criterion counts total lines, and the 367 non-code lines are comments citing the specific defect
each construct fixes — `file:line` references for the stale-artefact bug, the dead space-indented
WASM recipes, the silently-ignored master gate. Trimming them to reach the number would delete the
evidence that justifies the migration.

Recorded as a shortfall rather than met. Three resolutions are open for decision:
(a) amend SC-004 to measure code-only lines; (b) move the defect commentary into this docs tree and
cite it from the build files; (c) accept the 55.6% reduction as sufficient.

## What unblocks the rest

Rebuild the base image with: `meson` (≥1.3), `ninja`, WASI-SDK, and the four WASM runtime SDKs
(WAMR, WasmEdge, Wasmer, Wasmtime). The migrated `Dockerfile` already installs Meson and Ninja.

Then run the deferred tasks in order: **T022** (setup+compile) → **T023** (summary output) →
**T025** (incrementality) → **T028** (parallelism) → **T030** (packaging) → **T045**
(12-config matrix + 6 negative cases) → **T046** (install parity) → **T047** (delete Autotools, in
a single isolated commit so rollback is a clean revert).
