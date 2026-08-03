# Lua scripting support — replaces src/lua/Makefile.am (32 lines) and
# src/lua/internal/Makefile.am (37 lines).
#
# The embedded-header generation replaces the hand-rolled sed suffix rule at
# src/lua/internal/Makefile.am:25-36 with scripts/embed-file.py, which is
# verified byte-identical against that rule (contract C-7, task T005).

embed_file = find_program(meson.project_source_root() / 'scripts/embed-file.py')

embed_header = generator(
  embed_file,
  output: '@PLAINNAME@.h',
  arguments: ['@INPUT@', '@OUTPUT@'],
)

lua_internal_sources = files(
  'internal/sysbench.lua',
  'internal/sysbench.sql.lua',
  'internal/sysbench.rand.lua',
  'internal/sysbench.cmdline.lua',
  'internal/sysbench.histogram.lua',
)

lua_embedded_headers = embed_header.process(lua_internal_sources)

# Benchmark scripts installed to pkgdatadir, per src/lua/Makefile.am:19-31.
install_data(
  files(
    'bulk_insert.lua',
    'oltp_delete.lua',
    'oltp_insert.lua',
    'oltp_read_only.lua',
    'oltp_read_write.lua',
    'oltp_point_select.lua',
    'oltp_update_index.lua',
    'oltp_update_non_index.lua',
    'oltp_write_only.lua',
    'select_random_points.lua',
    'select_random_ranges.lua',
    'oltp_common.lua',
  ),
  install_dir: get_option('datadir') / 'sysbench',
)
