---
id: "20260801T132903Z-speckit-instructions"
unit_id: "/speckit.instructions"
unit_type: "command"
run_id: "sysbench-instructions-20260801T132832Z"
scope: "local"
partial: false
created: "2026-08-01T13:29:03Z"
summary: "Full reconcile with empty arguments on a repo with no pre-existing .specify/instructions.md — effectively a bootstrap. Setup script succeeded, glossary was created non-destructively, and all 7 compati"
---

## Review
Full reconcile with empty arguments on a repo with no pre-existing .specify/instructions.md — effectively a bootstrap. Setup script succeeded, glossary was created non-destructively, and all 7 compatibility symlinks were established and verified to write through. Filled all template placeholders and added three project-specific sections (Architecture, Developer Workflows, Conventions & Pitfalls) capturing knowledge that requires reading multiple files. Preserved all three registry marker ranges and every non-derivable template section byte-for-byte. Notably, the mandated Documentation Map existence check earned its keep: it caught two rows (CONTRIBUTING.md, docs/) pointing at files that do not exist in this repo.

## Optimization Points
- The command's Action 5 tolerance-band logic is written for the refresh case; the bootstrap case (no pre-existing instructions.md) falls through it awkwardly. Actions 2 and 3 both say "skip if no file existed", but Action 5's "iterate over the base file's sections" then operates on a pure-template file where every auto-derivable section is a placeholder. An explicit bootstrap branch ("fill every placeholder from analysis; add project-specific sections") would remove the ambiguity.
- The Documentation Map guidance mandates a scripted existence check, which correctly caught that CONTRIBUTING.md and docs/ do not exist here. But it does not say what to DO with a dead row: delete it, or keep it marked absent. Keeping them (marked "does not exist") was chosen so agents stop citing them and so Feature 017 has an anchor, but the command should state the preferred policy.
- Nothing in the command prompts verification of facts the agent itself writes into the file. Three claims I generated were wrong on first pass (C99, LuaJIT 2.1, and a regen command that cannot run in this project) and were only caught by a deliberate re-check. A "verify generated facts against source before writing" step would align this command with the Input Sanity section it is itself installing.
