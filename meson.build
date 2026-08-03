# sysbench — root build definition
#
# Replaces configure.ac (496 lines) + 21 m4/ macros (3,961 lines).
# Dependency discovery lives here and in the per-directory meson.build files,
# not in vendored macro code — see docs/build/overview.md.
#
# STATUS: migration scaffolding. Autotools remains authoritative until gate G-2
# passes (see docs/build/migration-status.md and requirements FR-018).

project(
  'sysbench',
  'c',
  version: '1.1.0',
  license: 'GPL-2.0-or-later',
  meson_version: '>=1.3',
  default_options: [
    'c_std=c99',
    'warning_level=2',
    'werror=false',
    'buildtype=release',
  ],
)

cc = meson.get_compiler('c')
conf = configuration_data()

conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('PACKAGE_NAME', meson.project_name())
conf.set_quoted('PACKAGE_VERSION', meson.project_version())
conf.set_quoted('VERSION', meson.project_version())

# ---------------------------------------------------------------------------
# Header checks — replaces AC_CHECK_HEADERS (configure.ac:213-231)
# ---------------------------------------------------------------------------

check_headers = [
  'errno.h',
  'fcntl.h',
  'math.h',
  'pthread.h',
  'sched.h',
  'signal.h',
  'stdlib.h',
  'string.h',
  'sys/aio.h',
  'sys/ipc.h',
  'sys/time.h',
  'sys/mman.h',
  'sys/shm.h',
  'thread.h',
  'unistd.h',
  'limits.h',
  'libgen.h',
]

foreach h : check_headers
  conf.set('HAVE_' + h.underscorify().to_upper(), cc.has_header(h))
endforeach

# ---------------------------------------------------------------------------
# Function checks — replaces AC_CHECK_FUNCS (configure.ac:270-287)
# ---------------------------------------------------------------------------

m_dep = cc.find_library('m', required: false)
rt_dep = cc.find_library('rt', required: false)

check_funcs = [
  'alarm',
  'clock_gettime',
  'directio',
  'fdatasync',
  'gettimeofday',
  'isatty',
  'memalign',
  'memset',
  'posix_memalign',
  'pthread_cancel',
  'pthread_yield',
  'setvbuf',
  'sqrt',
  'strdup',
  'thr_setconcurrency',
  'valloc',
  'mmap',
  'strerror_r',
]

foreach f : check_funcs
  conf.set('HAVE_' + f.to_upper(), cc.has_function(f, dependencies: [m_dep, rt_dep]))
endforeach

# pthread_once is mandatory — configure.ac:289-291 makes its absence a hard error.
thread_dep = dependency('threads')
if not cc.has_function('pthread_once', dependencies: thread_dep)
  error('pthread_once() is not available on this platform')
endif

# ---------------------------------------------------------------------------
# Type and declaration checks — replaces AC_CHECK_SIZEOF / AC_CHECK_DECLS
# ---------------------------------------------------------------------------

conf.set('SIZEOF_SIZE_T', cc.sizeof('size_t'))
conf.set('SIZEOF_BOOL', cc.sizeof('bool', prefix: '#include <stdbool.h>'))

conf.set10('HAVE_DECL_SHM_HUGETLB',
           cc.has_header_symbol('sys/shm.h', 'SHM_HUGETLB'))
conf.set10('HAVE_DECL_O_SYNC',
           cc.has_header_symbol('fcntl.h', 'O_SYNC'))

if cc.has_header_symbol('sys/shm.h', 'SHM_HUGETLB')
  conf.set('HAVE_LARGE_PAGES', 1)
  conf.set('HUGETLB_USE_PROC_MEMINFO', 1)
endif

if not cc.has_header_symbol('fcntl.h', 'O_SYNC')
  conf.set('O_SYNC', 'O_FSYNC')
endif

# ---------------------------------------------------------------------------
# TLS keyword — replaces AX_TLS (m4/ax_tls.m4, 72 lines).
# configure.ac:238 makes the absence of any working keyword fatal.
# ---------------------------------------------------------------------------

tls_keyword = ''
foreach kw : ['_Thread_local', '__thread', '__declspec(thread)']
  if tls_keyword == ''
    if cc.compiles('@0@ int foo; int main(void){foo = 1; return 0;}'.format(kw),
                   name: 'TLS keyword ' + kw)
      tls_keyword = kw
    endif
  endif
endforeach
if tls_keyword == ''
  error('no thread-local storage keyword supported by this compiler')
endif
conf.set('TLS', tls_keyword)

# ---------------------------------------------------------------------------
# Function attributes — replaces AX_GCC_FUNC_ATTRIBUTE
# (m4/ax_gcc_func_attribute.m4, 239 lines)
# ---------------------------------------------------------------------------

conf.set10('HAVE_FUNC_ATTRIBUTE_FORMAT',
           cc.has_function_attribute('format'))
conf.set10('HAVE_FUNC_ATTRIBUTE_UNUSED',
           cc.has_function_attribute('unused'))

# ---------------------------------------------------------------------------
# Version metadata — replaces SB_GIT_SHA (configure.ac:389-399).
# MUST succeed when git metadata is absent (FR-005, contract C-10).
# ---------------------------------------------------------------------------

git_sha = ''
git_prog = find_program('git', required: false)
if git_prog.found()
  git_res = run_command(git_prog, 'rev-parse', '--short', 'HEAD', check: false)
  if git_res.returncode() == 0
    git_sha = '-' + git_res.stdout().strip()
  endif
endif
conf.set_quoted('SB_GIT_SHA', git_sha)

# ---------------------------------------------------------------------------
# Options — capability parity with the 22 Autotools options.
# See .specify/specs/001-modernize-build-system/contracts/option-mapping.md
# ---------------------------------------------------------------------------

opt_wasm = get_option('wasm')
opt_wamr = get_option('wamr')
opt_wasmedge = get_option('wasmedge')
opt_wasmer = get_option('wasmer')
opt_wasmtime = get_option('wasmtime')

# Parent gating (contract C-5, INV-1): requesting a runtime implies the master
# gate. This makes the m4/sb_wasm.m4:227-234 defect — where --with-wamr without
# --with-wasm is silently ignored — unexpressible.
wasm_children = {
  'wamr': opt_wamr,
  'wasmedge': opt_wasmedge,
  'wasmer': opt_wasmer,
  'wasmtime': opt_wasmtime,
}

any_child_requested = false
foreach name, opt : wasm_children
  if opt.enabled()
    any_child_requested = true
  endif
endforeach

if any_child_requested and opt_wasm.disabled()
  error(
    'A WASM runtime was explicitly enabled while the \'wasm\' master gate is ' +
    'disabled. Enable -Dwasm=enabled, or disable the runtime. ' +
    '(Autotools silently ignored this combination — see FR-013.)'
  )
endif

wasm_enabled = opt_wasm.enabled() or (opt_wasm.auto() and any_child_requested)
conf.set10('HAVE_WASM', wasm_enabled)

# Accumulators for the configuration summary (contract C-3, INV-4). Each
# subdirectory records its optional components here as it resolves them:
#   component_status += {'<name>': 'compiled-in' | 'skipped'}
#   skipped_reasons  += ['<name>: <why>']
component_status = {}
skipped_reasons = []
# Records each discovered dependency's version where the provider exposes one,
# so a build reports what it was actually built against (Story 4 acceptance
# scenario 3, task T041).
dependency_versions = {}

if wasm_enabled
  component_status += {'wasm': 'compiled-in'}
else
  component_status += {'wasm': 'skipped'}
  skipped_reasons += ['wasm: not requested (-Dwasm=enabled to enable)']
endif

# ---------------------------------------------------------------------------
# Subdirectories
# ---------------------------------------------------------------------------

subdir('third_party')
subdir('src')
subdir('tests')

configure_file(output: 'config.h', configuration: conf)

# ---------------------------------------------------------------------------
# Configuration summary — contract C-3, FR-012, INV-4.
# Every optional component reports its resolution, and a reason when skipped.
# Replaces the AC_MSG_RESULT block at configure.ac:425-496.
# ---------------------------------------------------------------------------

summary(
  {
    'C standard': get_option('c_std'),
    'Build type': get_option('buildtype'),
    'Warnings as errors': get_option('werror'),
    'Sanitizer': get_option('b_sanitize'),
    'Coverage': get_option('b_coverage'),
    'TLS keyword': tls_keyword,
    'Version': meson.project_version() + git_sha,
  },
  section: 'Build configuration',
)

summary(
  {
    'LuaJIT': get_option('luajit'),
    'Concurrency Kit': get_option('concurrency-kit'),
  },
  section: 'Bundled dependencies',
)

summary(component_status, section: 'Optional components')

if skipped_reasons.length() > 0
  summary({'Skipped': '; '.join(skipped_reasons)}, section: 'Optional components')
endif

if dependency_versions.keys().length() > 0
  summary(dependency_versions, section: 'Dependency versions')
endif
