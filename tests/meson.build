# Test suite — replaces tests/Makefile.am (60 lines).
#
# The cram harness (46 .t files under t/) is driven by tests/test_run.sh, which
# is preserved as-is. Contract C-13 requires the harness environment to survive
# the migration unchanged.

# tests/include/config.sh is generated from its .in template with the same four
# substitutions Autotools performed (contract C-10).
test_conf = configuration_data()
test_conf.set('PACKAGE_VERSION', meson.project_version())
test_conf.set('SB_GIT_SHA', git_sha)

if component_status.get('mysql', 'skipped') == 'compiled-in'
  test_conf.set('USE_MYSQL', '1')
else
  test_conf.set('USE_MYSQL', '0')
endif

if component_status.get('pgsql', 'skipped') == 'compiled-in'
  test_conf.set('USE_PGSQL', '1')
else
  test_conf.set('USE_PGSQL', '0')
endif

configure_file(
  input: 'include/config.sh.in',
  output: 'config.sh',
  configuration: test_conf,
)

# test_run.sh locates the sysbench binary by walking a source-relative dirlist
# (tests/test_run.sh:24-36), none of which resolves inside a Meson build tree.
# Passing the binary's directory on PATH satisfies its lookup without editing the
# harness.
cram_env = environment()
cram_env.set('SBTEST_ROOTDIR', meson.current_source_dir())
cram_env.set('SBTEST_SCRIPTDIR', meson.project_source_root() / 'src/lua')
cram_env.set('SBTEST_SUITEDIR', meson.current_source_dir() / 't')
cram_env.set('SBTEST_INCDIR', meson.current_source_dir() / 'include')
cram_env.set('SBTEST_CONFIG', meson.current_build_dir() / 'config.sh')
cram_env.prepend('PATH', meson.project_build_root() / 'src')
cram_env.prepend('PATH', meson.project_source_root() / 'third_party/cram/scripts')
cram_env.prepend('PYTHONPATH', meson.project_source_root() / 'third_party/cram')

test(
  'cram',
  find_program('test_run.sh'),
  env: cram_env,
  depends: [sysbench_exe],
  timeout: 900,
  workdir: meson.current_source_dir(),
)

# Test files install to datadir/sysbench/tests/{t,include}, reproducing the
# nonstandard install-data-local rule at tests/Makefile.am:28-56 which globbed
# *.t *.sh *.lua across both subdirectories (contract C-15).
install_subdir('t',
  install_dir: get_option('datadir') / 'sysbench/tests',
  exclude_files: ['.gitignore'],
)

install_subdir('include',
  install_dir: get_option('datadir') / 'sysbench/tests',
  exclude_files: ['.gitignore'],
)
