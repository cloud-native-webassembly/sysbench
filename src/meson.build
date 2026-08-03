# sysbench executable — replaces src/Makefile.am (88 lines).
#
# Source lists mirror src/Makefile.am:33-43 with the conditional appends from
# lines 53-85 expressed as plain if-blocks instead of AM_CONDITIONAL.

sysbench_sources = files(
  'sysbench.c',
  'sb_timer.c',
  'sb_options.c',
  'sb_logger.c',
  'db_driver.c',
  'sb_histogram.c',
  'sb_rand.c',
  'sb_thread.c',
  'sb_barrier.c',
  'sb_lua.c',
  'sb_util.c',
  'sb_counter.c',
  'sb_file.c',
)

sysbench_deps = [thread_dep, m_dep, rt_dep]
sysbench_link = []

# --- Optional: embedded Python backend -------------------------------------

opt_python = get_option('python')
python_dep = dependency('python3-embed',
                        required: opt_python,
                        method: 'pkg-config',
                        not_found_message: 'python3 embed support not found')

if python_dep.found()
  sysbench_sources += files('sb_python.c')
  sysbench_deps += python_dep
  component_status += {'python': 'compiled-in'}
  dependency_versions += {'python3-embed': python_dep.version()}
else
  component_status += {'python': 'skipped'}
  if opt_python.disabled()
    skipped_reasons += ['python: not requested']
  else
    skipped_reasons += ['python: python3-embed not found via pkg-config']
  endif
endif

# --- Optional: WASM backends ------------------------------------------------
#
# The shared layer plus one file per runtime. Each runtime is independently
# omittable (Constitution Principle V) — a build with a subset of SDKs must
# still succeed.

if wasm_enabled
  sysbench_sources += files('sb_wasm.c')

  wasm_runtime_files = {
    'wamr': 'sb_wamr.c',
    'wasmedge': 'sb_wasmedge.c',
    'wasmer': 'sb_wasmer.c',
    'wasmtime': 'sb_wasmtime.c',
  }

  # Library name each runtime links against, and the header that proves the SDK
  # is usable. Contract C-6 / VD-1: verify the header AND the library — the
  # presence of a runtime CLI on PATH is NOT sufficient evidence, which is the
  # flaw in m4/sb_wasm.m4's discovery.
  wasm_runtime_probe = {
    'wamr': ['iwasm', 'wasm_export.h'],
    'wasmedge': ['wasmedge', 'wasmedge/wasmedge.h'],
    'wasmer': ['wasmer', 'wasmer.h'],
    'wasmtime': ['wasmtime', 'wasmtime.h'],
  }

  foreach rt, srcfile : wasm_runtime_files
    opt_rt = get_option(rt)
    if not opt_rt.disabled()
      libname = wasm_runtime_probe[rt][0]
      header = wasm_runtime_probe[rt][1]
      rt_lib = cc.find_library(libname, required: false)
      rt_hdr = cc.has_header(header)

      if rt_lib.found() and rt_hdr
        sysbench_sources += files(srcfile)
        sysbench_deps += rt_lib
        conf.set10('HAVE_' + rt.to_upper(), true)
        component_status += {rt: 'compiled-in'}
      elif opt_rt.enabled()
        # Explicitly requested but unsatisfiable — fail loudly (contract C-4,
        # FR-013). Never silently ignore an explicit request.
        error(
          'WASM runtime \'' + rt + '\' was explicitly enabled but its SDK is ' +
          'incomplete: library \'' + libname + '\' found=' +
          rt_lib.found().to_string() + ', header \'' + header + '\' found=' +
          rt_hdr.to_string() + '. Set ' + rt.to_upper() +
          '_HOME or install the development files.'
        )
      else
        component_status += {rt: 'skipped'}
        skipped_reasons += [rt + ': SDK not found (library ' + libname + ' / header ' + header + ')']
      endif
    else
      component_status += {rt: 'skipped'}
      skipped_reasons += [rt + ': not requested']
    endif
  endforeach
endif

# --- Generated sources ------------------------------------------------------

subdir('lua')
subdir('python')
subdir('drivers')
subdir('tests')

if wasm_enabled
  subdir('wasm')
endif

sysbench_sources += lua_embedded_headers
sysbench_sources += python_embedded_headers

# --- Executable -------------------------------------------------------------

sysbench_exe = executable(
  'sysbench',
  sysbench_sources,
  include_directories: include_directories('.'),
  dependencies: sysbench_deps,
  link_with: builtin_test_libs + driver_libs,
  c_args: [
    '-DDATADIR="' + get_option('prefix') / get_option('datadir') / 'sysbench' + '"',
    '-DLIBDIR="' + get_option('prefix') / get_option('libdir') / 'sysbench' + '"',
  ],
  install: true,
)
