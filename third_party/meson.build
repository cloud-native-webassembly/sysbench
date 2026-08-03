# Bundled dependencies — replaces third_party/luajit/Makefile.am (38 lines) and
# third_party/concurrency_kit/Makefile.am (37 lines).
#
# WHY THIS FILE MATTERS (task T026, requirements § Context & Baseline):
# the Autotools rules declared their targets with NO PREREQUISITES, e.g.
#
#     $(builddir)/lib/libluajit-5.1.a:
#             $(MAKE) -C $(srcdir)/luajit clean
#
# Consequences, both measured 2026-08-02:
#   * archive present -> LuaJIT is NEVER rebuilt. Touching
#     third_party/luajit/luajit/src/lj_api.c produced ZERO compiles and left the
#     archive mtime unchanged. A source edit did not reach the binary: a silent
#     STALE-ARTEFACT correctness bug.
#   * archive absent  -> full rebuild, and only then does the unconditional
#     `make clean` fire.
#
# `depend_files` fixes this: the archive rebuilds if and only if a file under the
# dependency's source tree changed (contract C-11, BD-2).
#
# The old rules also ran `make clean` inside $(srcdir), writing into the source
# tree. These wrappers copy-then-build and never mutate the original (BD-1,
# contract C-12, INV-3).

fs = import('fs')

# --- LuaJIT -----------------------------------------------------------------

luajit_choice = get_option('luajit')

if luajit_choice == 'system'
  luajit_dep = dependency('luajit', required: true, method: 'pkg-config')
  conf.set_quoted('SB_WITH_LUAJIT', 'system')
  component_status += {'luajit': 'system'}
else
  conf.set_quoted('SB_WITH_LUAJIT', 'bundled')
  component_status += {'luajit': 'bundled'}

  # Every file under the vendored tree is a prerequisite. This is the whole
  # point: the glob is what makes the rebuild trigger correct.
  luajit_sources = run_command(
    'find', meson.current_source_dir() / 'luajit',
    '-type', 'f', '-name', '*.[ch]',
    check: true,
  ).stdout().strip().split('\n')

  luajit_build = custom_target(
    'luajit',
    output: 'libluajit-5.1.a',
    command: [
      find_program(meson.project_source_root() / 'scripts/build-bundled.sh'),
      'luajit',
      meson.current_source_dir() / 'luajit',
      meson.current_build_dir() / 'luajit-build',
      '@OUTPUT@',
    ],
    depend_files: files(luajit_sources),
    console: true,
  )

  luajit_dep = declare_dependency(
    sources: luajit_build,
    include_directories: include_directories('luajit/src'),
  )
endif

# --- Concurrency Kit --------------------------------------------------------

ck_choice = get_option('concurrency-kit')

if ck_choice == 'system'
  ck_dep = dependency('ck', required: true, method: 'pkg-config')
  conf.set_quoted('SB_WITH_CK', 'system')
  component_status += {'concurrency-kit': 'system'}
else
  conf.set_quoted('SB_WITH_CK', 'bundled')
  component_status += {'concurrency-kit': 'bundled'}

  ck_sources = run_command(
    'find', meson.current_source_dir() / 'ck',
    '-type', 'f', '-name', '*.[ch]',
    check: true,
  ).stdout().strip().split('\n')

  ck_build = custom_target(
    'concurrency-kit',
    output: 'libck.a',
    command: [
      find_program(meson.project_source_root() / 'scripts/build-bundled.sh'),
      'ck',
      meson.current_source_dir() / 'ck',
      meson.current_build_dir() / 'ck-build',
      '@OUTPUT@',
    ],
    depend_files: files(ck_sources),
    console: true,
  )

  ck_dep = declare_dependency(
    sources: ck_build,
    include_directories: include_directories('ck/include'),
  )
endif
