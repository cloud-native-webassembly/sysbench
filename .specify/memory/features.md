# Feature Index

This index tracks all functional and non-functional features managed within the project. It serves as the central directory for specifications, plans, and implementation status.

Scope: this fork extends sysbench from a database/OS benchmark tool into a harness for
benchmarking external systems, currently focused on WASM runtimes. Features below cover that
extension plus project-wide engineering work (e.g. the build system); upstream sysbench
capabilities (OLTP, fileio, cpu, memory, threads, mutex, Lua and Python scripting) are
inherited and not re-registered here.

<!--
  ACTION REQUIRED for any command that mutates this table (`/speckit.feature`,
  `/speckit.plan`, `/speckit.implement`):

  The `Total Features` value below MUST be auto-derived from the number of data
  rows in the table (count rows that begin with `| ` and a feature ID — exclude
  the header and separator rows). Do NOT maintain it by hand and do NOT bump it
  separately when adding a feature row; recompute on every write.

  Reference shell expression a human can paste to verify:
      awk -F'|' '/^\| [0-9]{3} \|/ {n++} END{print n}' .specify/memory/features.md
-->

**Total Features**: 19 _(auto-derived; recompute on every edit — see comment above)_

Status values follow the canonical state machine in
`.specify/templates/feature-details-template.md` § "Canonical Status State Machine".
Statuses below were derived from source inspection during registry bootstrap, not from
completed `/speckit.plan` → `/speckit.implement` cycles.

## Functional Features

| ID | Name | Description | Status | Feature Details | Last Updated |
|---|---|---|---|---|---|
| 001 | WASM Test Type Dispatch | Registers `--type=wasm` as a first-class sysbench test type and selects a runtime backend via `--wasm-runtime`. | Implemented | [001](.specify/memory/features/001.md) | 2026-08-01 |
| 002 | WASM Runtime Abstraction Layer | Defines the shared vtable, sandbox, and module contracts that every WASM engine backend implements. | Implemented | [002](.specify/memory/features/002.md) | 2026-08-01 |
| 003 | WAMR Runtime Backend | Integrates the WebAssembly Micro Runtime SDK as a sysbench WASM backend, including memory address translation. | Implemented | [003](.specify/memory/features/003.md) | 2026-08-01 |
| 004 | WasmEdge Runtime Backend | Integrates the WasmEdge SDK as a sysbench WASM backend covering load, validate, instantiate, and invoke. | Implemented | [004](.specify/memory/features/004.md) | 2026-08-01 |
| 005 | Wasmer Runtime Backend | Integrate the Wasmer SDK as a sysbench WASM backend; currently a registered stub with no engine calls. | Draft | [005](.specify/memory/features/005.md) | 2026-08-01 |
| 006 | Wasmtime Runtime Backend | Integrate the Wasmtime SDK as a sysbench WASM backend; currently a registered stub with no engine calls. | Draft | [006](.specify/memory/features/006.md) | 2026-08-01 |
| 007 | Host-Sandbox Data Exchange | Packed address/size encoding plus app-to-native translation for passing buffers between sysbench and a WASM module. | Implemented | [007](.specify/memory/features/007.md) | 2026-08-01 |
| 008 | WASM Benchmark Workload Suite | A set of C-sourced WASM modules exporting `event`, compiled by WASI-SDK, used as the workloads under test. | Implemented | [008](.specify/memory/features/008.md) | 2026-08-01 |
| 009 | WASM Build Configuration | Autoconf/automake wiring that detects each WASM SDK and conditionally compiles the corresponding backend. | Implemented | [009](.specify/memory/features/009.md) | 2026-08-01 |
| 019 | Modern Build System | Replace GNU Autotools with a modern build system delivering incremental parallel builds and readable, maintainable configuration. | Planned | [019](.specify/memory/features/019.md) | 2026-08-01 |

## Non-functional Features

| ID | Name | Description | Status | Feature Details | Last Updated |
|---|---|---|---|---|---|
| 010 | Sandbox Resource Configuration | Environment-variable-driven heap, stack, thread, and buffer sizing for WASM sandboxes (DFCfg). | Implemented | [010](.specify/memory/features/010.md) | 2026-08-01 |
| 011 | Runtime Debug Harness Scripts | Per-runtime shell wrappers and shared environment file for running and debugging WASM benchmarks (DFM). | Implemented | [011](.specify/memory/features/011.md) | 2026-08-01 |
| 012 | WASM Resource Lifecycle Teardown | Release engine, module, instance, and heap resources on shutdown so long runs do not leak (DFR). | Draft | [012](.specify/memory/features/012.md) | 2026-08-01 |
| 013 | Cross-Runtime Result Comparability | Record execution mode, SDK version, and configuration with every result so runtime comparisons are valid (DFP). | Draft | [013](.specify/memory/features/013.md) | 2026-08-01 |
| 014 | WASM Layer Automated Tests | Cram/unit coverage for WASM dispatch, backend lifecycle, and data exchange (DFT). | Draft | [014](.specify/memory/features/014.md) | 2026-08-01 |
| 015 | WASM Continuous Integration | CI jobs that configure, build, and exercise the WASM backends on every change (DFD). | Draft | [015](.specify/memory/features/015.md) | 2026-08-01 |
| 016 | Pinned SDK Provisioning | Reproducible, version-pinned acquisition of the four WASM SDKs and the WASI guest toolchain (DFCfg). | Draft | [016](.specify/memory/features/016.md) | 2026-08-01 |
| 017 | WASM Layer Documentation | Architecture, backend contract, workload authoring, and usage docs for the WASM extension (DFDoc). | Draft | [017](.specify/memory/features/017.md) | 2026-08-01 |
| 018 | WASM Failure Diagnostics | Consistent logger-routed errors and graceful degradation instead of NULL-pointer crashes (DFR). | Draft | [018](.specify/memory/features/018.md) | 2026-08-01 |

## Runtime Support Matrix

Current backend maturity, per Constitution Principle II (code is authoritative for actual
state). Reported from source inspection on 2026-08-01.

| Runtime | Configure flag | Build wiring | Engine calls | Sandbox lifecycle | Address translation | Harness scripts | Feature |
|---|---|---|---|---|---|---|---|
| WAMR | `--with-wamr` | yes | yes | yes | yes | yes | 003 |
| WasmEdge | `--with-wasmedge` | yes | yes | yes | no | yes | 004 |
| Wasmer | `--with-wasmer` | yes | no | no | no | no | 005 |
| Wasmtime | `--with-wasmtime` | yes | no | no | no | no | 006 |

All four require the master `--with-wasm` gate in addition to their own flag
(`m4/sb_wasm.m4`); see Feature 009 for the known defect in `scripts/build.sh`.
