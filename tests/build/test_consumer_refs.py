#!/usr/bin/env python3
"""Structural contract test: downstream consumer migration (SC-008, gate G-6).

Asserts that no live reference to an Autotools entry point remains outside
vendored third_party/ code and spec/documentation prose.

EXPECTED STATE THIS ROUND: this test FAILS by design. Autotools is still
authoritative (FR-018) and its deletion is task T047, deferred until gates
G-2..G-5 pass. The test documents the target state and becomes the gate G-6
acceptance check.
"""
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]

# Entry points that must not survive the migration.
#
# NOTE: `dh_auto_configure` is deliberately NOT listed. It is buildsystem-agnostic
# — debhelper dispatches it to whichever buildsystem it detects, and after the
# migration it dispatches to Meson (Debian::Debhelper::Buildsystem::meson). Its
# presence in debian/rules is correct post-migration, so flagging it would be a
# false positive.
PATTERNS = [
    r"autogen\.sh",
    r"autoreconf",
    r"\./configure",
    r"%configure",
    r"plugin: autotools",
]

# Paths exempt from the assertion, each for a stated reason.
EXEMPT_PREFIXES = [
    ("third_party/", "vendored upstream code, not ours to change"),
    (".specify/", "spec and design artefacts describe the migration itself"),
    ("docs/", "documentation legitimately describes both build systems"),
    ("README.md", "documents the Autotools path while it remains authoritative"),
    ("tests/build/", "this test file names the patterns it searches for"),
]

# AI instruction files are all symlinks to .specify/instructions.md — the same
# document surfaced under seven names. Counting each separately would inflate the
# violation count sevenfold for a single piece of documentation prose.
EXEMPT_FILES = {
    "scripts/build-bundled.sh": "runs Concurrency Kit's own ./configure, not the project's",
    "AGENTS.md": "symlink to .specify/instructions.md (documentation)",
    "CLAUDE.md": "symlink to .specify/instructions.md (documentation)",
    "QWEN.md": "symlink to .specify/instructions.md (documentation)",
    "QODER.md": "symlink to .specify/instructions.md (documentation)",
    ".github/copilot-instructions.md": "symlink to .specify/instructions.md (documentation)",
    ".qoder/project_rules.md": "symlink to .specify/instructions.md (documentation)",
    ".claude/project_rules.md": "symlink to .specify/instructions.md (documentation)",
}


def tracked_files() -> list[str]:
    out = subprocess.run(
        ["git", "-C", str(REPO), "ls-files"],
        capture_output=True, text=True, check=True,
    ).stdout
    return [l for l in out.splitlines() if l]


def is_exempt(path: str) -> str | None:
    if path in EXEMPT_FILES:
        return EXEMPT_FILES[path]
    for prefix, reason in EXEMPT_PREFIXES:
        if path.startswith(prefix):
            return reason
    return None


def main() -> int:
    violations: list[tuple[str, int, str]] = []
    exempted = 0

    for rel in tracked_files():
        if is_exempt(rel):
            exempted += 1
            continue
        p = REPO / rel
        if not p.is_file():
            continue
        try:
            text = p.read_text(encoding="utf-8")
        except (UnicodeDecodeError, OSError):
            continue
        for lineno, line in enumerate(text.splitlines(), 1):
            stripped = line.lstrip()
            # Skip commentary — a comment explaining what was removed is not a
            # live invocation.
            if stripped.startswith(("#", "//", "*", ">")):
                continue
            for pat in PATTERNS:
                if re.search(pat, line):
                    violations.append((rel, lineno, line.strip()))
                    break

    print(f"tracked files scanned: {len(tracked_files()) - exempted}")
    print(f"exempted:              {exempted}")
    print(f"live autotools refs:   {len(violations)}")

    if violations:
        print("\nRemaining live references (expected until gate G-6 / task T047):")
        for rel, lineno, line in violations:
            print(f"  {rel}:{lineno}: {line}")
        print(
            "\nFAIL (BY DESIGN this round): Autotools is still authoritative "
            "per FR-018. This test becomes the gate G-6 acceptance check.",
            file=sys.stderr,
        )
        return 1

    print("\nPASS: no live Autotools references remain (SC-008 satisfied)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
