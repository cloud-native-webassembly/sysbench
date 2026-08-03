# Quickstart: Modernize Build System

**Requirement**: `001-modernize-build-system` → Feature 019 Modern Build System  
**Date**: 2026-08-01

Command sequences for the migration. Each is marked with its verification status.

**Legend**: ✅ executed and verified in the authoring environment · ⏳ specified, pending
verification against the real tool (Meson is not installed here; contract C-16 requires
verification during implementation).

---

## Step 1 — Capture the baseline ✅ *(gate G-1, blocks all other work)*

Run this **before authoring any Meson file**. Once Autotools is deleted the baseline is
unrecoverable, and SC-001–SC-004 become permanently unverifiable.

### 1a. Line-count baseline ✅ verified

```shell
# Project-authored porting surface
wc -l < configure.ac                                    # 496
wc -l < autogen.sh                                      # 3
find . -name 'Makefile.am' \
     -not -path './third_party/luajit/luajit/*' \
     -not -path './third_party/cram/*' \
     -print0 | xargs -0 cat | wc -l                     # 614
cat m4/sb_*.m4 m4/ac_check_*.m4 | wc -l                 # 792
                                                        # TOTAL = 1,905

# Vendored macros (deleted, not ported)
ls m4/acx_pthread.m4 m4/ax_*.m4 m4/extensions.m4 \
   m4/host-cpu-c-abi.m4 m4/lib-*.m4 m4/pkg.m4 | wc -l   # 13 files
wc -l m4/acx_pthread.m4 m4/ax_*.m4 m4/extensions.m4 \
      m4/host-cpu-c-abi.m4 m4/lib-*.m4 m4/pkg.m4 | tail -1   # 3,169
```

**Verified 2026-08-01**: all figures reproduce exactly as shown.

### 1b. Timing baseline ⏳

Record on one fixed machine, 3 runs each, report the **median**, note cache state.

```shell
# Record machine identity first
nproc; uname -a; cc --version | head -1

# Full clean build
sh autogen.sh && ./configure --without-mysql --with-wasm --with-wamr
time make -j"$(nproc)"

# No-op rebuild (the headline SC-001 number)
time make -j"$(nproc)"

# Single-file-change rebuild (SC-002)
touch src/sb_wasm.c && time make -j"$(nproc)"
```

### 1c. Install and test baselines ⏳

```shell
make install DESTDIR=/tmp/sb-baseline-install
( cd /tmp/sb-baseline-install && find . -type f | sort ) > /tmp/baseline-files.txt   # SC-011
make check 2>&1 | tee /tmp/baseline-tests.txt                                       # SC-007
```

---

## Step 2 — Configure and build with Meson ⏳

```shell
# Minimal build
meson setup builddir
meson compile -C builddir

# WASM-enabled build (the common development configuration)
meson setup builddir-wasm -Dwasm=enabled -Dwamr=enabled -Dwasmedge=enabled
meson compile -C builddir-wasm

# Reconfigure an existing build directory
meson configure builddir-wasm -Dpython=enabled
meson compile -C builddir-wasm
```

`meson setup` prints the configuration summary required by contract C-3 — every optional
component with its resolution and, when skipped, the reason.

**No `autogen.sh` step exists.** That is the point of C-1: configuration is one command.

---

## Step 3 — Run the tests ⏳

```shell
meson test -C builddir-wasm                    # full cram suite
meson test -C builddir-wasm --verbose          # with output
meson test -C builddir-wasm --print-errorlogs  # failures only
```

Compare the test count and pass set against `/tmp/baseline-tests.txt` from Step 1c (SC-007).

---

## Step 4 — Verify incrementality ⏳ *(gate G-2)*

This is the check that proves the project's core complaint is fixed.

```shell
meson compile -C builddir-wasm                      # ensure fully built

# No-op: MUST do zero work
meson compile -C builddir-wasm                      # expect "no work to do"

# Single file: MUST recompile exactly one object, then relink
touch src/sb_wasm.c
meson compile -C builddir-wasm --verbose            # expect 1 compile + 1 link

# Bundled deps MUST NOT rebuild when untouched
meson compile -C builddir-wasm --verbose | grep -ci luajit    # expect 0
```

The last check is the regression guard for the current defect at
`third_party/luajit/Makefile.am:27`, which runs `make clean` unconditionally on every build.

---

## Step 5 — Verify out-of-tree purity ⏳ *(contract C-12)*

```shell
git status --porcelain          # MUST be empty after a full build
```

A non-empty result means something wrote into the source tree. The known offender is the LuaJIT
wrapper's in-`srcdir` `make clean`.

---

## Step 6 — Run the configuration matrix ⏳ *(gate G-2, SC-006)*

```shell
# Positive cases: all 12 MUST succeed
for cfg in \
  "" \
  "-Dmysql=enabled" \
  "-Dpgsql=enabled" \
  "-Dmysql=enabled -Dpgsql=enabled" \
  "-Dwasm=enabled -Dwamr=enabled" \
  "-Dwasm=enabled -Dwasmedge=enabled" \
  "-Dwasm=enabled -Dwasmer=enabled" \
  "-Dwasm=enabled -Dwasmtime=enabled" \
  "-Dwasm=enabled -Dwamr=enabled -Dwasmedge=enabled -Dwasmer=enabled -Dwasmtime=enabled" \
  "-Dpython=enabled" \
  "-Dluajit=system -Dconcurrency-kit=system" \
  "-Dmysql=enabled -Dpgsql=enabled -Dpython=enabled -Dwasm=enabled -Dwamr=enabled"
do
  rm -rf /tmp/m && meson setup /tmp/m $cfg >/dev/null 2>&1 \
    && meson compile -C /tmp/m >/dev/null 2>&1 \
    && echo "PASS: ${cfg:-<no options>}" \
    || echo "FAIL: ${cfg:-<no options>}"
done
```

```shell
# Negative case N-1: the current silent-ignore defect MUST NOT reproduce
rm -rf /tmp/n1 && meson setup /tmp/n1 -Dwamr=enabled -Dwasm=disabled
# MUST either imply wasm, or fail naming the gate — never silently produce a WASM-less binary
```

---

## Step 7 — Verify consumer migration ⏳ *(gate G-4, SC-008)*

```shell
# Zero live Autotools references must remain
grep -rE 'autogen\.sh|autoreconf|\./configure|dh_auto_configure|%configure|plugin: autotools' \
  --include='*.yml' --include='*.yaml.in' --include='*.spec' \
  --include='Dockerfile' --include='rules' --include='*.sh' --include='*.md' . \
  | grep -v third_party | grep -v '\.specify/'
```

✅ **Verified 2026-08-01**: this command currently matches **9 files** — the 7 consumers plus
`autogen.sh` itself and `README.md`. After G-6 it MUST return nothing.

---

## Step 8 — Verify install parity ⏳ *(gate G-5, SC-011)*

```shell
meson install -C builddir-wasm --destdir /tmp/sb-meson-install
( cd /tmp/sb-meson-install && find . -type f | sort ) > /tmp/meson-files.txt
diff /tmp/baseline-files.txt /tmp/meson-files.txt
```

The **only** expected difference: 7 `.wasm` guest modules absent from `bindir` — the deliberate
FR-020 correction.

---

## Step 9 — Delete Autotools ⏳ *(gate G-6, single isolated commit)*

Only after G-1…G-5 all pass.

```shell
git rm configure.ac autogen.sh
git rm $(find . -name 'Makefile.am' -not -path './third_party/luajit/luajit/*' \
                                    -not -path './third_party/cram/*')
git rm -r m4/

# Confirm a build works with no Autotools tooling present (SC-012)
meson setup /tmp/clean-check && meson compile -C /tmp/clean-check
```

Commit deletions **alone** — mixing a functional change into this commit breaks the documented
rollback path in `contracts/migration-gates.md`.

---

## Verified reference: embedded-header compatibility ✅

Contract C-7 requires generated headers to stay byte-identical. The replacement script was
verified against all 6 real inputs during planning:

```shell
# Reference output from the current sed rule
var=$(echo sysbench.rand.lua | sed 's/\./_/g')
( echo "unsigned char ${var}[] =" && \
  sed -e 's/\\/\\\\/g' -e 's/"/\\"/g' -e 's/^/  "/g' -e 's/$/\\n"/g' sysbench.rand.lua && \
  echo ";" && echo "size_t ${var}_len = sizeof(${var}) - 1;" ) > ref.h

python3 scripts/embed-file.py sysbench.rand.lua new.h
diff ref.h new.h        # MUST be empty
```

**Result**: byte-identical for all 5 Lua files and the Python file.

**Non-obvious rule this surfaced**: `src/python/sysbench.py` has **no trailing newline** (last
byte `0x29`). `sed` therefore emits the closing `;` on the same line as the final string literal.
A naive line-joining implementation fails only on that one file. The first attempt did fail this
way; the rule is now normative in C-7.
