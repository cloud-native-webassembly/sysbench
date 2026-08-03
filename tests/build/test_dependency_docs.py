#!/usr/bin/env python3
"""Structural contract test: dependency documentation completeness (Story 4).

Asserts that docs/build/dependencies.md names every external SDK dependency
recorded in data-model.md Entity 4, and that every *_HOME environment variable
the docs promise is actually consulted by an authored Meson file.

The dependency list is derived from data-model.md rather than hard-coded, so
adding an entity updates the expectation automatically.
"""
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
DATA_MODEL = REPO / ".specify/specs/001-modernize-build-system/data-model.md"
DOCS = REPO / "docs/build/dependencies.md"

MESON_FILES = [
    "meson.build",
    "src/meson.build",
    "src/lua/meson.build",
    "src/python/meson.build",
    "src/wasm/meson.build",
    "src/drivers/meson.build",
    "src/tests/meson.build",
    "tests/meson.build",
    "third_party/meson.build",
]


def external_sdk_names() -> list[str]:
    """Extract SDK identifiers from the `name` field row of Entity 4.

    The identifiers are enumerated only in that row's Notes cell; other rows
    describe fields (discovery, home_var, link_mode) and must not be treated as
    dependency names.
    """
    text = DATA_MODEL.read_text(encoding="utf-8")
    section = re.search(
        r"^## Entity 4: External SDK Dependency(.*?)^## ", text, re.M | re.S
    )
    if not section:
        raise SystemExit("FAIL: Entity 4 section not found in data-model.md")
    for line in section.group(1).splitlines():
        if re.match(r"^\|\s*`name`\s*\|", line):
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            return re.findall(r"`([^`]+)`", cells[-1])
    raise SystemExit("FAIL: Entity 4 has no `name` field row")


def main() -> int:
    for path in (DATA_MODEL, DOCS):
        if not path.exists():
            print(f"FAIL: missing required file {path}", file=sys.stderr)
            return 1

    docs_text = DOCS.read_text(encoding="utf-8")
    docs_lower = docs_text.lower()
    errors: list[str] = []

    # --- Assertion 1: every external SDK is documented ----------------------
    sdks = external_sdk_names()
    if not sdks:
        errors.append("parsed zero SDK names from data-model.md Entity 4")

    documented = []
    for sdk in sdks:
        # data-model uses lowercase identifiers; docs use display names.
        needle = sdk.lower().replace("_", "").replace("-", "")
        haystack = docs_lower.replace("_", "").replace("-", "")
        if needle in haystack:
            documented.append(sdk)
        else:
            errors.append(f"external SDK '{sdk}' is not documented in {DOCS.name}")

    # --- Assertion 2: promised *_HOME vars are actually consulted -----------
    promised = sorted(set(re.findall(r"\$?([A-Z][A-Z0-9_]*_HOME)", docs_text)))
    meson_blob = ""
    for rel in MESON_FILES:
        p = REPO / rel
        if p.exists():
            meson_blob += p.read_text(encoding="utf-8")

    wired = []
    for var in promised:
        if var in meson_blob:
            wired.append(var)
        else:
            errors.append(
                f"{var} is promised by the docs but no authored meson.build reads it"
            )

    print(f"external SDKs in data-model Entity 4: {len(sdks)}")
    print(f"  documented: {len(documented)}/{len(sdks)}")
    print(f"*_HOME variables promised by docs:    {len(promised)} {promised}")
    print(f"  wired into meson files: {len(wired)}/{len(promised)}")

    if errors:
        print("\nFAILURES:", file=sys.stderr)
        for e in errors:
            print(f"  - {e}", file=sys.stderr)
        return 1

    print("\nPASS: dependency documentation is complete and consistent")
    return 0


if __name__ == "__main__":
    sys.exit(main())
