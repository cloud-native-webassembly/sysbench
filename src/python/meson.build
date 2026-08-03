# Embedded Python backend — replaces src/python/Makefile.am (35 lines).
#
# Reuses the embed_header generator declared in src/lua/meson.build, which wraps
# scripts/embed-file.py — verified byte-identical to the sed rule this replaces
# (contract C-7, task T005). src/python/sysbench.py has no trailing newline, so
# the closing ';' lands on the final string literal's line; the script preserves
# that exactly.

python_embedded_headers = []

if python_dep.found()
  python_embedded_headers = embed_header.process(files('sysbench.py'))
endif
