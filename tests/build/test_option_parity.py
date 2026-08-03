#!/usr/bin/env python3
"""Structural contract test: option capability parity (SC-005, FR-002).

Asserts that every user-facing capability recorded in
contracts/option-mapping.md is either declared as a project option in
meson_options.txt, backed by a documented Meson built-in, or recorded as a
deliberate drop.

Counts are derived from the parsed contract table, never hard-coded, so adding a
capability updates the expectation automatically.
"""
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
CONTRACT = REPO / ".specify/specs/001-modernize-build-system/contracts/option-mapping.md"
OPTIONS = REPO / "meson_options.txt"


def declared_options() -> set[str]:
    text = OPTIONS.read_text(encoding="utf-8")
    return set(re.findall(r"^option\(\s*'([a-z0-9-]+)'", text, re.M))


def contract_rows() -> list[tuple[str, str]]:
    """Return (autotools_option, disposition) for every numbered contract row."""
    text = CONTRACT.read_text(encoding="utf-8")
    rows = []
    for line in text.splitlines():
        m = re.match(r"^\|\s*(\d+)\s*\|\s*`([^`]+)`\s*\|(.*)$", line)
        if m:
            rows.append((m.group(2), m.group(3)))
    return rows


def dropped_rationales() -> dict[str, str]:
    """Parse the 'Recorded drops' section into {option: rationale}."""
    text = CONTRACT.read_text(encoding="utf-8")
    section = re.search(
        r"^## Recorded drops.*?$(.*?)^## ", text, re.M | re.S
    )
    if not section:
        return {}
    out = {}
    for line in section.group(1).splitlines():
        m = re.match(r"^\|\s*`([^`]+)`\s*\|\s*(.+?)\s*\|\s*$", line)
        if m:
            out[m.group(1)] = m.group(2)
    return out


def main() -> int:
    for path in (CONTRACT, OPTIONS):
        if not path.exists():
            print(f"FAIL: missing required file {path}", file=sys.stderr)
            return 1

    declared = declared_options()
    rows = contract_rows()
    rationales = dropped_rationales()

    if not rows:
        print("FAIL: parsed zero rows from the contract table", file=sys.stderr)
        return 1

    errors: list[str] = []
    project_backed = 0
    builtin_backed = 0
    dropped = 0

    for autotools_opt, rest in rows:
        meson_names = re.findall(r"`([a-z0-9-]+)`", rest)
        is_builtin = "builtin" in rest
        is_dropped = "DROPPED" in rest

        if is_dropped:
            dropped += 1
            rationale = rationales.get(autotools_opt, "")
            if len(rationale) < 80:
                errors.append(
                    f"{autotools_opt}: no substantive rationale in the "
                    f"'Recorded drops' section"
                )
            continue

        if is_builtin:
            builtin_backed += 1
            continue

        matched = [n for n in meson_names if n in declared]
        if matched:
            project_backed += 1
        else:
            errors.append(
                f"{autotools_opt}: no declared project option "
                f"(candidates {meson_names or '[]'} not in meson_options.txt)"
            )

    total = project_backed + builtin_backed + dropped
    print(f"contract rows parsed:   {len(rows)}")
    print(f"  project-option backed: {project_backed}")
    print(f"  Meson-builtin backed:  {builtin_backed}")
    print(f"  deliberately dropped:  {dropped}")
    print(f"  accounted for:         {total}/{len(rows)}")

    if total != len(rows):
        errors.append(f"only {total} of {len(rows)} capabilities accounted for")

    if errors:
        print("\nFAILURES:", file=sys.stderr)
        for e in errors:
            print(f"  - {e}", file=sys.stderr)
        return 1

    print("\nPASS: every capability is accounted for (SC-005)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
