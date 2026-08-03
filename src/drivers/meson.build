# Database drivers — replaces src/drivers/Makefile.am plus the mysql and pgsql
# Makefile.am files (27 + 22 + 22 = 71 lines).
#
# Discovery follows contract C-6 / VD-1: verify the header AND the library.
# Explicit path options take precedence over pkg-config, which takes precedence
# over the vendor *_config helper.

driver_libs = []

# --- MySQL ------------------------------------------------------------------

opt_mysql = get_option('mysql')
mysql_inc = get_option('mysql-includes')
mysql_libdir = get_option('mysql-libs')

mysql_dep = dependency('mysqlclient',
                       required: false,
                       method: 'pkg-config',
                       not_found_message: 'mysqlclient not found via pkg-config')

if not mysql_dep.found() and not opt_mysql.disabled()
  mysql_config = find_program('mysql_config', required: false)
  if mysql_config.found()
    mysql_cflags = run_command(mysql_config, '--cflags', check: false).stdout().split()
    mysql_libs = run_command(mysql_config, '--libs', check: false).stdout().split()
    mysql_dep = declare_dependency(compile_args: mysql_cflags, link_args: mysql_libs)
  endif
endif

if not opt_mysql.disabled() and mysql_dep.found()
  mysql_args = []
  if mysql_inc != ''
    mysql_args += '-I' + mysql_inc
  endif
  mysql_link = []
  if mysql_libdir != ''
    mysql_link += '-L' + mysql_libdir
  endif

  driver_libs += static_library(
    'sbmysql',
    files('mysql/drv_mysql.c'),
    include_directories: include_directories('..'),
    dependencies: mysql_dep,
    c_args: mysql_args,
    link_args: mysql_link,
  )
  conf.set('USE_MYSQL', 1)
  component_status += {'mysql': 'compiled-in'}
  if mysql_dep.type_name() == 'pkgconfig'
    dependency_versions += {'mysqlclient': mysql_dep.version()}
  endif
elif opt_mysql.enabled()
  error(
    'MySQL support was explicitly enabled but the client library was not found. ' +
    'Install libmysqlclient-dev, or set -Dmysql-includes= and -Dmysql-libs=.'
  )
else
  component_status += {'mysql': 'skipped'}
  if opt_mysql.disabled()
    skipped_reasons += ['mysql: not requested']
  else
    skipped_reasons += ['mysql: client library not found']
  endif
endif

# --- PostgreSQL -------------------------------------------------------------

opt_pgsql = get_option('pgsql')
pgsql_inc = get_option('pgsql-includes')
pgsql_libdir = get_option('pgsql-libs')

pgsql_dep = dependency('libpq',
                       required: false,
                       method: 'pkg-config',
                       not_found_message: 'libpq not found via pkg-config')

if not pgsql_dep.found() and not opt_pgsql.disabled()
  pg_config = find_program('pg_config', required: false)
  if pg_config.found()
    pg_incdir = run_command(pg_config, '--includedir', check: false).stdout().strip()
    pg_libdir = run_command(pg_config, '--libdir', check: false).stdout().strip()
    pq_lib = cc.find_library('pq', dirs: [pg_libdir], required: false)
    if pq_lib.found()
      pgsql_dep = declare_dependency(compile_args: ['-I' + pg_incdir], dependencies: pq_lib)
    endif
  endif
endif

if not opt_pgsql.disabled() and pgsql_dep.found()
  pgsql_args = []
  if pgsql_inc != ''
    pgsql_args += '-I' + pgsql_inc
  endif
  pgsql_link = []
  if pgsql_libdir != ''
    pgsql_link += '-L' + pgsql_libdir
  endif

  driver_libs += static_library(
    'sbpgsql',
    files('pgsql/drv_pgsql.c'),
    include_directories: include_directories('..'),
    dependencies: pgsql_dep,
    c_args: pgsql_args,
    link_args: pgsql_link,
  )
  conf.set('USE_PGSQL', 1)
  component_status += {'pgsql': 'compiled-in'}
  if pgsql_dep.type_name() == 'pkgconfig'
    dependency_versions += {'libpq': pgsql_dep.version()}
  endif
elif opt_pgsql.enabled()
  error(
    'PostgreSQL support was explicitly enabled but libpq was not found. ' +
    'Install libpq-dev, or set -Dpgsql-includes= and -Dpgsql-libs=.'
  )
else
  component_status += {'pgsql': 'skipped'}
  if opt_pgsql.disabled()
    skipped_reasons += ['pgsql: not requested']
  else
    skipped_reasons += ['pgsql: libpq not found']
  endif
endif
