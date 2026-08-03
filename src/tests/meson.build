# Builtin benchmark libraries — replaces src/tests/Makefile.am and the five
# per-benchmark Makefile.am files (19 + 5x23 = 134 lines).
#
# Each was a noinst_LIBRARIES static convenience archive linked into sysbench
# via sysbench_LDADD (src/Makefile.am:45-49).

builtin_tests = {
  'sbcpu': ['cpu/sb_cpu.c'],
  'sbfileio': ['fileio/sb_fileio.c', 'fileio/crc32.c'],
  'sbmemory': ['memory/sb_memory.c'],
  'sbthreads': ['threads/sb_threads.c'],
  'sbmutex': ['mutex/sb_mutex.c'],
}

builtin_test_libs = []

foreach name, srcs : builtin_tests
  builtin_test_libs += static_library(
    name,
    files(srcs),
    include_directories: include_directories('.', '..'),
    dependencies: [thread_dep, m_dep, rt_dep],
  )
endforeach
