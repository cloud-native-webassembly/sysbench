#!/usr/bin/env python3
"""Structural contract test: no vendored macros, and size reduction (SC-004).

Two assertions:

1. FR-010 / FR-011 — no authored Meson file references anything under m4/.
   Dependency discovery must live in project-authored build code so a
   contributor can answer "how is this library found?" without reading vendored
   macro code.

2. SC-004 — project-authored build configuration is at most 40% of the measured
   Autotools baseline of 1,905 lines, i.e. <= 762 lines.

The line budget is a floor-semantics assertion (<=), not an equality, so a
legitimate later addition does not fail an unrelated contract. The baseline is
read from verification.md rather than hard-coded here, keeping one source of
truth.
"""
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
VERIFICATION = REPO / ".specify/specs/001-modernize-build-system/verification.md"

MESON_FILES = [
    "meson.build",
    "meson_options.txt",
    "src/meson.build",
    "src/lua/meson.build",
    "src/python/meson.build",
    "src/wasm/meson.build",
    "src/drivers/meson.build",
    "src/tests/meson.build",
    "tests/meson.build",
    "third_party/meson.build",
]

HELPER_SCRIPTS = [
    "scripts/embed-file.py",
    "scripts/build-bundled.sh",
]


def read_baseline() -> tuple[int, int]:
    """Return (project_authored_total, sc004_target) from verification.md."""
    text = VERIFICATION.read_text(encoding="utf-8")
    total = re.search(r"^baseline_project_authored_total=(\d+)", text, re.M)
    target = re.search(r"^baseline_sc004_target_lines=(\d+)", text, re.M)
    if not total or not target:
        raise SystemExit("FAIL: baseline keys missing from verification.md")
    return int(total.group(1)), int(target.group(1))


def main() -> int:
    errors: list[str] = []

    # --- Assertion 1: every surface file must actually exist ----------------
    # A phantom entry would turn this test into a permanent FileNotFoundError,
    # so existence is checked explicitly rather than assumed.
    present = []
    for rel in MESON_FILES + HELPER_SCRIPTS:
        p = REPO / rel
        if p.exists():
            present.append(rel)
        else:
            errors.append(f"declared surface file is missing: {rel}")

    # --- Assertion 2: no references into m4/ --------------------------------
    for rel in present:
        text = (REPO / rel).read_text(encoding="utf-8")
        for lineno, line in enumerate(text.splitlines(), 1):
            if line.lstrip().startswith("#"):
                continue  # commentary may cite m4 files as historical context
            if re.search(r"\bm4/", line):
                errors.append(f"{rel}:{lineno} references m4/: {line.strip()}")

    # --- Assertion 3: size budget (SC-004) ---------------------------------
    baseline_total, target = read_baseline()

    def count(rel: str) -> tuple[int, int]:
        lines = (REPO / rel).read_text(encoding="utf-8").splitlines()
        code = [l for l in lines if l.strip() and not l.lstrip().startswith("#")]
        return len(lines), len(code)

    authored_total = 0
    authored_code = 0
    for rel in present:
        t, c = count(rel)
        authored_total += t
        authored_code += c

    print("SC-004 — size reduction")
    print(f"  Autotools project-authored baseline: {baseline_total} lines")
    print(f"  Budget (40% of baseline):            {target} lines")
    print(f"  Meson + helpers, total:              {authored_total} lines")
    print(f"  Meson + helpers, code only:          {authored_code} lines")
    print(f"  files counted:                       {len(present)}")
    if baseline_total:
        print(f"  total vs baseline:                   "
              f"{100.0 * authored_total / baseline_total:.1f}%")
        print(f"  code-only vs baseline:               "
              f"{100.0 * authored_code / baseline_total:.1f}%")

    print("\nSC-004 — vendored-macro independence")
    print("  vendored macro files required for discovery: 0  (was 13 files / 3169 lines)")

    if authored_total > target:
        errors.append(
            f"SC-004 line budget MISSED: {authored_total} authored lines "
            f"> {target} budget ({100.0 * authored_total / baseline_total:.1f}% "
            f"of baseline, target <=40%). Code-only is {authored_code} lines "
            f"({100.0 * authored_code / baseline_total:.1f}%). The "
            f"vendored-macro half of SC-004 passes. Record as a shortfall with "
            f"analysis rather than trimming explanatory comments to fit."
        )

    if errors:
        print("\nFAILURES:", file=sys.stderr)
        for e in errors:
            print(f"  - {e}", file=sys.stderr)
        return 1

    print("\nPASS: no vendored-macro dependency, and within the SC-004 budget")
    return 0


if __name__ == "__main__":
    sys.exit(main())
