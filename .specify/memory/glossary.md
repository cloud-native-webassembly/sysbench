# Project Glossary (项目词汇表)

> **Note**: This file is initialized by `/speckit.instructions` and lives beside `constitution.md` / `features.md`. It is the project's single, project-wide vocabulary anchor: it corrects voice/dictated input (homophones, easily-confused words) and doubles as a lightweight domain-knowledge dictionary. It is loaded as ambient context by every `/speckit.*` command via the Documentation Map. See `.specify/shared/workflow/glossary.md` for the correction / enrichment / conflict protocol.

## Authoring Rules

- **Common words are NOT recorded** — only project-specific / domain terms that carry special meaning here.
- **User edits are authoritative (以用户输入为准)** — manual entries win over automatic proposals and are preserved across regenerations; automatic proposals MUST NOT silently overwrite a `user` entry.
- **Conflicts require confirmation** — a new term that collides with an existing entry (same term/different meaning, or a homophone/near-duplicate) is written only after the user confirms the resolution.

## Column Definitions

| Column | Meaning |
|--------|---------|
| Canonical | The agreed project term (unique, case-insensitive). |
| Variants | Comma-separated homophones / easily-confused / dictation-error forms that anchor back to Canonical; `-` when none. |
| Meaning | Brief one-line domain definition. |
| Origin | `auto` (framework-proposed) or `user` (manually authored/confirmed). |
| Status | `proposed` (awaiting confirmation) or `confirmed`. |

## Glossary

| Canonical | Variants | Meaning | Origin | Status |
|-----------|----------|---------|--------|--------|
| sysbench | sys bench, sysbanch | The benchmark framework this project forks; LuaJIT-based, multi-threaded, C core. | auto | proposed |
| WASM | wasm, WebAssembly, web assembly | WebAssembly; the bytecode format whose runtimes are this fork's benchmark subject. | auto | proposed |
| WAMR | wamr, wanmr, WebAssembly Micro Runtime | WebAssembly Micro Runtime; the default and most complete backend (--wasm-runtime=wamr). | auto | proposed |
| WasmEdge | wasmedge, wasm edge | CNCF WASM runtime; second working backend, lacks address translation. | auto | proposed |
| Wasmer | wasmer, wasm er | WASM runtime; backend currently a stub with no SDK calls (Feature 005). | auto | proposed |
| Wasmtime | wasmtime, wasm time | Bytecode Alliance WASM runtime; backend currently a stub (Feature 006). | auto | proposed |
| sandbox | sand box | Per-thread WASM module instance (sb_wasm_sandbox); one is created per sysbench thread. | auto | proposed |
| runtime backend | backend, runtime driver | A per-engine implementation of the sb_wasm_runtime vtable, e.g. src/sb_wamr.c. | auto | proposed |
| carrier | - | The single int64_t argument/return value used to pass data across the host-WASM call boundary. | auto | proposed |
| WASI-SDK | wasi sdk, wasisdk | Clang-based toolchain that compiles src/wasm/*.c guest workloads to wasm32-wasi. | auto | proposed |
| workload | guest module, wasm module | A compiled WASM module exporting event, invoked once per benchmark iteration. | auto | proposed |
| event | - | The mandatory WASM export invoked per iteration; also the sysbench unit of measured work. | auto | proposed |
| LuaJIT | luajit, lua jit | Just-in-time Lua interpreter bundled in third_party/; powers sysbench's scripting backend. | auto | proposed |
| cram | - | Vendored shell-based integration test framework driving the 46 .t files in tests/t/. | auto | proposed |
