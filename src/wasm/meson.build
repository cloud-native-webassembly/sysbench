# WASM guest workload modules — replaces src/wasm/Makefile.am (46 lines).
#
# DEFECT BEING FIXED (task T020, research.md Decision 3):
# the Autotools rules at src/wasm/Makefile.am:41-45 are indented with SPACES
# where Make requires TABS, so those recipes are dead — the guest modules are
# NOT built by WASI-SDK today. Verified with `cat -A`. This file implements the
# INTENDED behaviour, not the current behaviour.
#
# Second defect: the guests were declared `bin_PROGRAMS`, which would install
# .wasm files into bindir. They are benchmark inputs, not host executables, so
# they are deliberately NOT installed (BT-1, FR-020).

wasi_sdk_home = get_option('wasi-sdk-home')
if wasi_sdk_home == ''
  wasi_sdk_home = run_command(
    'sh', '-c', 'echo "${WASI_SDK_HOME:-}"', check: false,
  ).stdout().strip()
endif

if wasi_sdk_home == ''
  error(
    'WASM support is enabled but WASI-SDK was not found. Set WASI_SDK_HOME or ' +
    'pass -Dwasi-sdk-home=<path>. The guest workload modules in src/wasm/ ' +
    'require the WASI-SDK clang cross-compiler.'
  )
endif

wasi_clang = wasi_sdk_home / 'bin/clang'
wasi_sysroot = wasi_sdk_home / 'share/wasi-sysroot'

wasm_common_args = [
  '--target=wasm32-wasi',
  '--sysroot=' + wasi_sysroot,
  '-fPIC',
  '-z', 'stack-size=8192',
  '-Wl,--initial-memory=65536',
  '-Wl,--no-entry',
  '-Wl,--export=event',
]

# Every guest exports `event`, which the sysbench event loop invokes once per
# iteration. `pass_data` additionally exports `create_buffer` for the
# host-sandbox data-exchange handshake (Feature 007).
wasm_guests = [
  'build_user_id',
  'normalize_with_base',
  'point_polygen_distance',
  'split_string',
  'string_to_double',
  'string_to_int',
]

wasm_targets = []

foreach guest : wasm_guests
  wasm_targets += custom_target(
    guest + '.wasm',
    input: guest + '.c',
    output: guest + '.wasm',
    command: [wasi_clang] + wasm_common_args + ['-o', '@OUTPUT@', '@INPUT@'],
    build_by_default: true,
    install: false,
  )
endforeach

wasm_targets += custom_target(
  'pass_data.wasm',
  input: 'pass_data.c',
  output: 'pass_data.wasm',
  command: [wasi_clang] + wasm_common_args +
           ['-Wl,--export=create_buffer', '-o', '@OUTPUT@', '@INPUT@'],
  build_by_default: true,
  install: false,
)
