#!/usr/bin/env python3
"""Embed a text file into a C header as an unsigned char array.

Byte-compatible replacement for the sed suffix rules in
src/lua/internal/Makefile.am and src/python/Makefile.am.

Emits:

    unsigned char <var>[] =
      "<escaped line>\\n"
      ...
    ;
    size_t <var>_len = sizeof(<var>) - 1;

where <var> is the input filename with '.' replaced by '_'.
"""
import sys
from pathlib import Path


def render(name: str, raw: str) -> str:
    var = name.replace(".", "_")
    body = "".join(
        '  "' + line.replace("\\", "\\\\").replace('"', '\\"') + '\\n"\n'
        for line in raw.splitlines()
    )
    # sed emits no trailing newline for an input that lacks one, which leaves the
    # closing ';' on the same line as the final string literal. src/python/sysbench.py
    # relies on this.
    if raw and not raw.endswith("\n"):
        body = body[:-1]
    return (
        f"unsigned char {var}[] =\n{body};\n"
        f"size_t {var}_len = sizeof({var}) - 1;\n"
    )


def main(argv: list[str]) -> int:
    if len(argv) != 3:
        print(f"usage: {argv[0]} <input> <output|->", file=sys.stderr)
        return 1
    src = Path(argv[1])
    out = render(src.name, src.read_text(encoding="utf-8"))
    if argv[2] == "-":
        sys.stdout.write(out)
    else:
        Path(argv[2]).write_text(out, encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
