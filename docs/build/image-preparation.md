# 镜像构建环境准备指南

**Title**: 镜像构建环境准备指南
**Purpose**: 供上层构建系统据此准备容器镜像，使 sysbench 的 Meson 新构建系统能够完整编译和测试
**Status**: Active — 对应 Feature 019 框架轮次，8 个延迟任务（T022–T047）待此环境就绪后执行
**Last updated**: 2026-08-03
**Related**: [overview.md](./overview.md) · [dependencies.md](./dependencies.md) · [migration-status.md](./migration-status.md)

---

## 1. 背景与目标

Feature 019 的构建系统迁移已完成**框架搭建**（39/47 个任务关闭），但当前镜像缺少
Meson、Ninja、WASI-SDK 以及四个 WASM 运行时 SDK，导致 8 个编译/测试/打包任务被延迟。

本文档列出镜像中需要安装的**全部依赖**、每个依赖的**版本要求**和**安装验证方法**，
供上层构建系统据此构建一个完整可用的镜像。环境就绪后，按
[migration-status.md](./migration-status.md) 的顺序执行延迟任务即可。

**基础镜像**：`ubuntu:24.04`（已在 `Dockerfile` 中指定）

---

## 2. 基础构建工具

| 工具 | 最低版本 | apt 包名 | 验证命令 |
|------|---------|----------|---------|
| Meson | **≥ 1.3** | `meson` | `meson --version` |
| Ninja | 任意近期版本 | `ninja-build` | `ninja --version` |
| C 编译器 | GCC ≥ 11 或 Clang ≥ 14 | `gcc` / `clang` | `cc --version` |
| pkg-config | 任意 | `pkg-config` | `pkg-config --version` |
| Python 3 | ≥ 3.8 | `python3` | `python3 --version` |
| Make | GNU Make ≥ 4 | `make` | `make --version` |
| Git | 任意（版本元数据用，缺失不影响构建） | `git` | `git --version` |
| libc 开发头 | 匹配基础镜像 | `libc6-dev` | — |
| libaio 开发头 | 任意 | `libaio-dev` | `pkg-config --exists libaio && echo OK` |
| ca-certificates | 任意 | `ca-certificates` | — |

```dockerfile
RUN apt-get update && apt-get install -y --no-install-recommends \
        meson \
        ninja-build \
        gcc \
        clang \
        pkg-config \
        python3 \
        make \
        libc6-dev \
        libaio-dev \
        git \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*
```

> **注意**：Ubuntu 24.04 的 `meson` 包版本为 1.3.2，满足 ≥ 1.3 要求。如需在更旧的
> 基础镜像（如 ubuntu:22.04）上构建，用 `pip3 install meson>=1.3` 替代 apt 包。

---

## 3. 数据库驱动依赖（可选，但 CI 和 Dockerfile 默认启用）

| 驱动 | apt 包名 | pkg-config 名 | 验证命令 |
|------|----------|--------------|---------|
| MySQL | `libmysqlclient-dev` | `mysqlclient` | `mysql_config --version` |
| PostgreSQL | `libpq-dev` | `libpq` | `pg_config --version` |
| OpenSSL（MySQL 依赖） | `libssl-dev` | `openssl` | `pkg-config --modversion openssl` |
| zstd（MySQL 依赖） | `libzstd-dev` | `libzstd` | `pkg-config --modversion libzstd` |

```dockerfile
RUN apt-get update && apt-get install -y --no-install-recommends \
        libmysqlclient-dev libssl-dev libzstd-dev \
        libpq-dev \
    && rm -rf /var/lib/apt/lists/*
```

---

## 4. Python 嵌入后端依赖（可选，默认关闭）

仅当 `-Dpython=enabled` 时需要。Meson 通过 pkg-config 发现：

| 依赖 | pkg-config 名 | apt 包名（Ubuntu 24.04） | 验证 |
|------|---------------|-------------------------|------|
| Python 3 embed | `python3-embed` | `python3-dev` | `pkg-config --cflags python3-embed` |

```dockerfile
RUN apt-get update && apt-get install -y --no-install-recommends \
        python3-dev \
    && rm -rf /var/lib/apt/lists/*
```

---

## 5. WASI-SDK（构建 WASM 宿主模块的交叉编译器）

**用途**：编译 `src/wasm/` 下的 7 个 C 源文件为 `wasm32-wasi` 模块。这些模块是
sysbench WASM 后端执行的负载程序。仅当 `-Dwasm=enabled` 时需要。

| 属性 | 值 |
|------|-----|
| 上游 | https://github.com/WebAssembly/wasi-sdk |
| 推荐版本 | **wasi-sdk-25** 或更高 |
| 安装路径约定 | `/opt/wasi-sdk`（设置 `WASI_SDK_HOME` 环境变量指向此路径） |
| Meson 选项 | `-Dwasi-sdk-home=/opt/wasi-sdk` 或设置 `WASI_SDK_HOME` |
| 必需内容 | `bin/clang`、`share/wasi-sysroot/` |

**安装方式**（从 GitHub Release 下载预编译包）：

```dockerfile
ENV WASI_SDK_VERSION=25
ENV WASI_SDK_HOME=/opt/wasi-sdk

RUN wget -qO- "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/wasi-sdk-${WASI_SDK_VERSION}-x86_64-linux.tar.gz" \
    | tar xz --strip-components=1 -C ${WASI_SDK_HOME}
```

**验证**：

```shell
${WASI_SDK_HOME}/bin/clang --version
ls ${WASI_SDK_HOME}/share/wasi-sysroot
${WASI_SDK_HOME}/bin/clang --target=wasm32-wasi --sysroot=${WASI_SDK_HOME}/share/wasi-sysroot \
    -Wl,--no-entry -Wl,--export=event -o /tmp/test.wasm -x c - <<'EOF'
int event(long long c) { return c; }
EOF
```

> **aarch64 注意**：wasi-sdk 官方 Release 仅提供 x86_64 预编译包。如需在 aarch64
> 宿主上构建 WASM 模块，需要从源码编译 wasi-sdk，或使用交叉工具链。sysbench 的
> aarch64 支持指**宿主二进制**（native build），WASM 负载模块的交叉编译仍需 x86_64
> 环境或源码编译的 wasi-sdk。

---

## 6. WASM 运行时 SDK（四个引擎，各独立可选）

每个运行时是一个独立的可选组件。可以只安装需要的子集——sysbench 的条件编译矩阵
保证任何子集都能编译通过。

**通用发现规则**（Meson `src/meson.build` 实现）：

1. 验证 `*_HOME` 环境变量是否设置
2. 验证头文件 **和** 库文件同时存在（不再仅凭 CLI 二进制存在就推断可用——这是
   Autotools 构建的已知缺陷，`m4/sb_wasm.m4` 从 `iwasm`/`wasmedge`/`wasmer`/
   `wasmtime` 的 PATH 存在推断可用性，会错误接受不含开发文件的运行时 CLI）
3. 设置 `-I${*_HOME}/include`、`-L${*_HOME}/lib -Wl,-rpath=${*_HOME}/lib`、`-l<name>`

所有运行时均为**动态链接**。

---

### 6.1 WAMR（WebAssembly Micro Runtime）— 默认运行时

| 属性 | 值 |
|------|-----|
| 上游 | https://github.com/bytecodealliance/wasm-micro-runtime |
| 推荐版本 | WAMR 2.2.0 或更高 |
| 环境变量 | `WAMR_HOME` |
| 必需头文件 | `wasm_export.h`（也包含 `wasm_c_api.h`） |
| 必需库文件 | `libiwasm.so` / `libiwasm.a` |
| Meson 选项 | `-Dwamr=enabled`（隐含 `-Dwasm=enabled`） |

**安装方式**（从源码编译，WAMR 不提供预编译 Release 包）：

```dockerfile
ENV WAMR_HOME=/opt/wamr
ENV WAMR_VERSION=2.2.0

RUN git clone --depth 1 --branch WAMR-${WAMR_VERSION} \
        https://github.com/bytecodealliance/wasm-micro-runtime.git /tmp/wamr \
    && mkdir -p /tmp/wamr/product-mini/platforms/linux/build \
    && cd /tmp/wamr/product-mini/platforms/linux/build \
    && cmake .. \
        -DWAMR_BUILD_INTERP=1 \
        -DWAMR_BUILD_AOT=1 \
        -DWAMR_BUILD_LIBC_BUILTIN=1 \
        -DCMAKE_INSTALL_PREFIX=${WAMR_HOME} \
    && make -j$(nproc) \
    && make install \
    && cp -r /tmp/wamr/core/iwasm/include/* ${WAMR_HOME}/include/ \
    && rm -rf /tmp/wamr
```

**验证**：

```shell
test -f ${WAMR_HOME}/include/wasm_export.h
test -f ${WAMR_HOME}/lib/libiwasm.so
${WAMR_HOME}/bin/iwasm --version || true
```

---

### 6.2 WasmEdge

| 属性 | 值 |
|------|-----|
| 上游 | https://github.com/WasmEdge/WasmEdge |
| 推荐版本 | WasmEdge 0.14.1 或更高 |
| 环境变量 | `WASMEDGE_HOME` |
| 必需头文件 | `wasmedge/wasmedge.h` |
| 必需库文件 | `libwasmedge.so` |
| Meson 选项 | `-Dwasmedge=enabled`（隐含 `-Dwasm=enabled`） |

**安装方式**（官方安装脚本，会安装到 `/usr/local`）：

```dockerfile
ENV WASMEDGE_HOME=/usr/local

RUN curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh \
    | bash -s -- -v 0.14.1 -p ${WASMEDGE_HOME}
```

**验证**：

```shell
test -f ${WASMEDGE_HOME}/include/wasmedge/wasmedge.h
test -f ${WASMEDGE_HOME}/lib/libwasmedge.so
${WASMEDGE_HOME}/bin/wasmedge --version
```

> **已知限制**：WasmEdge 后端当前未实现地址转换（`addr_app_to_native`），因此不支持
> 缓冲区传递型负载（Feature 007）。仅支持标量调用型负载。

---

### 6.3 Wasmer

| 属性 | 值 |
|------|-----|
| 上游 | https://github.com/wasmerio/wasmer |
| 推荐版本 | Wasmer 4.3.7 或更高 |
| 环境变量 | `WASMER_HOME` |
| 必需头文件 | `wasmer.h` |
| 必需库文件 | `libwasmer.so` |
| Meson 选项 | `-Dwasmer=enabled`（隐含 `-Dwasm=enabled`） |

**安装方式**（官方安装脚本）：

```dockerfile
ENV WASMER_HOME=/opt/wasmer

RUN curl -sSfL https://get.wasmer.io | bash -s -- -v 4.3.7 -p ${WASMER_HOME}
```

**验证**：

```shell
test -f ${WASMER_HOME}/include/wasmer.h
test -f ${WASMER_HOME}/lib/libwasmer.so
${WASMER_HOME}/bin/wasmer --version
```

> **注意**：Wasmer 后端当前为 **stub**（`src/sb_wasmer.c` 函数体为空，无 SDK 调用）。
> Feature 005 待实现。安装 SDK 后编译可以成功（条件编译会包含 `sb_wasmer.c`），但
> 运行 `--wasm-runtime=wasmer` 会因 `create_sandbox` 为 NULL 而 segfault
> （`src/sb_wasm.c:231`）。Feature 018 会将此改为明确的错误提示。

---

### 6.4 Wasmtime

| 属性 | 值 |
|------|-----|
| 上游 | https://github.com/bytecodealliance/wasmtime |
| 推荐版本 | Wasmtime 28.0.0 或更高 |
| 环境变量 | `WASMTIME_HOME` |
| 必需头文件 | `wasmtime.h`（C API） |
| 必需库文件 | `libwasmtime.so` |
| Meson 选项 | `-Dwasmtime=enabled`（隐含 `-Dwasm=enabled`） |

**安装方式**（从 GitHub Release 下载预编译包）：

```dockerfile
ENV WASMTIME_HOME=/opt/wasmtime
ENV WASMTIME_VERSION=28.0.0

RUN mkdir -p ${WASMTIME_HOME} \
    && wget -qO- "https://github.com/bytecodealliance/wasmtime/releases/download/v${WASMTIME_VERSION}/wasmtime-${WASMTIME_VERSION}-x86_64-linux.tar.xz" \
    | tar xJ --strip-components=1 -C ${WASMTIME_HOME}
```

**验证**：

```shell
test -f ${WASMTIME_HOME}/include/wasmtime.h
test -f ${WASMTIME_HOME}/lib/libwasmtime.so
${WASMTIME_HOME}/bin/wasmtime --version
```

> **注意**：Wasmtime 后端当前为 **stub**（`src/sb_wasmtime.c` 函数体为空，甚至未包含
> `wasmtime.h`）。Feature 006 待实现。与 Wasmer 相同，安装 SDK 后编译可以成功，但
> 运行时会 segfault。Feature 018 会将此改为明确的错误提示。

---

## 7. 辅助工具（可选，运行时/调试用）

这些工具不是构建必需的，但用于运行 WASM 基准测试和调试：

| 工具 | 用途 | apt 包名 / 安装方式 | 环境变量 |
|------|------|---------------------|---------|
| WABT | WASM 二进制工具（`wasm2wat`、`wasm-objdump` 等） | `wabt` | `WABT_HOME=/usr` |
| LLVM | WAMR AOT 编译需要 | 从 LLVM 官方安装 | `LLVM_HOME` |
| GCC（带 multilib） | WAMR AOT 编译需要 32 位支持 | `gcc-multilib` | `GCC_HOME=/usr` |
| Valgrind | 内存泄漏检测（Feature 012 的 gate） | `valgrind` | — |

```dockerfile
RUN apt-get update && apt-get install -y --no-install-recommends \
        wabt \
        valgrind \
        gcc-multilib \
    && rm -rf /var/lib/apt/lists/*
```

---

## 8. 环境变量汇总

镜像中需设置以下环境变量（放入 `/etc/profile.d/` 或 Dockerfile `ENV`）：

```dockerfile
ENV WASI_SDK_HOME=/opt/wasi-sdk
ENV WAMR_HOME=/opt/wamr
ENV WASMEDGE_HOME=/usr/local
ENV WASMER_HOME=/opt/wasmer
ENV WASMTIME_HOME=/opt/wasmtime
ENV LD_LIBRARY_PATH=${WAMR_HOME}/lib:${WASMEDGE_HOME}/lib:${WASMER_HOME}/lib:${WASMTIME_HOME}/lib
```

---

## 9. 构建验证命令

环境就绪后，依次运行以下命令验证 sysbench 完整构建：

### 9.1 基础构建（无 WASM）

```shell
cd /usr/src/sysbench
meson setup builddir-base -Dmysql=enabled -Dpgsql=enabled
meson compile -C builddir-base
meson test -C builddir-base --print-errorlogs
```

预期：编译成功，测试通过，`builddir-base/src/sysbench --version` 输出版本号。

### 9.2 WASM 构建（完整四运行时）

```shell
meson setup builddir-wasm \
    -Dmysql=disabled \
    -Dwasm=enabled \
    -Dwamr=enabled \
    -Dwasmedge=enabled \
    -Dwasmer=enabled \
    -Dwasmtime=enabled
meson compile -C builddir-wasm
meson test -C builddir-wasm --print-errorlogs
```

### 9.3 配置矩阵（SC-006，12 个配置）

```shell
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

### 9.4 负面用例（SC-010，6 个）

```shell
# N-1: 请求 wamr 但禁用 wasm 主门 → 必须报错，不得静默忽略
rm -rf /tmp/n1 && meson setup /tmp/n1 -Dwamr=enabled -Dwasm=disabled 2>&1 | grep -q "ERROR" \
  && echo "PASS: N-1" || echo "FAIL: N-1"

# N-2: 请求 wamr 但未安装 WAMR SDK → 必须报错
rm -rf /tmp/n2 && meson setup /tmp/n2 -Dwamr=enabled 2>&1 | grep -q "wamr" \
  && echo "PASS: N-2" || echo "FAIL: N-2"
```

### 9.5 增量构建验证（SC-001/SC-002）

```shell
# 无操作重建 → 零编译
meson compile -C builddir-wasm  # 预期 "no work to do"

# 单文件修改 → 恰好重编译一个 TU
touch src/sb_timer.c
meson compile -C builddir-wasm --verbose  # 预期 1 个 cc 调用

# 源码树纯净性
git status --porcelain  # 预期无变更
```

---

## 10. 打包验证

### Debian 包

```shell
apt-get install -y debhelper devscripts
dpkg-buildpackage -us -uc -b
```

### RPM 包

```shell
apt-get install -y rpm
rpmbuild -ba rpm/sysbench.spec
```

### Snap

```shell
apt-get install -y snapcraft
snapcraft snap
```

---

## 11. 完整 Dockerfile 参考

以下是将上述所有依赖整合的完整 Dockerfile，供上层构建系统直接使用或参考：

```dockerfile
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

# --- 基础构建工具 ---
RUN apt-get update && apt-get install -y --no-install-recommends \
        meson ninja-build gcc clang pkg-config python3 make \
        libc6-dev libaio-dev git ca-certificates curl wget xz-utils \
    && rm -rf /var/lib/apt/lists/*

# --- 数据库驱动 ---
RUN apt-get update && apt-get install -y --no-install-recommends \
        libmysqlclient-dev libssl-dev libzstd-dev libpq-dev \
    && rm -rf /var/lib/apt/lists/*

# --- Python 嵌入后端（可选）---
RUN apt-get update && apt-get install -y --no-install-recommends \
        python3-dev \
    && rm -rf /var/lib/apt/lists/*

# --- WASI-SDK ---
ENV WASI_SDK_HOME=/opt/wasi-sdk
RUN wget -qO- "https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-25/wasi-sdk-25-x86_64-linux.tar.gz" \
    | tar xz --strip-components=1 -C ${WASI_SDK_HOME}

# --- WAMR ---
ENV WAMR_HOME=/opt/wamr
RUN git clone --depth 1 --branch WAMR-2.2.0 \
        https://github.com/bytecodealliance/wasm-micro-runtime.git /tmp/wamr \
    && mkdir -p /tmp/wamr/product-mini/platforms/linux/build \
    && cd /tmp/wamr/product-mini/platforms/linux/build \
    && cmake .. -DWAMR_BUILD_INTERP=1 -DWAMR_BUILD_AOT=1 -DWAMR_BUILD_LIBC_BUILTIN=1 \
        -DCMAKE_INSTALL_PREFIX=${WAMR_HOME} \
    && make -j$(nproc) && make install \
    && cp -r /tmp/wamr/core/iwasm/include/* ${WAMR_HOME}/include/ \
    && rm -rf /tmp/wamr

# --- WasmEdge ---
ENV WASMEDGE_HOME=/usr/local
RUN curl -sSf https://raw.githubusercontent.com/WasmEdge/WasmEdge/master/utils/install.sh \
    | bash -s -- -v 0.14.1 -p ${WASMEDGE_HOME}

# --- Wasmer ---
ENV WASMER_HOME=/opt/wasmer
RUN curl -sSfL https://get.wasmer.io | bash -s -- -v 4.3.7 -p ${WASMER_HOME}

# --- Wasmtime ---
ENV WASMTIME_HOME=/opt/wasmtime
RUN mkdir -p ${WASMTIME_HOME} \
    && wget -qO- "https://github.com/bytecodealliance/wasmtime/releases/download/v28.0.0/wasmtime-28.0.0-x86_64-linux.tar.xz" \
    | tar xJ --strip-components=1 -C ${WASMTIME_HOME}

# --- 辅助工具 ---
RUN apt-get update && apt-get install -y --no-install-recommends \
        wabt valgrind gcc-multilib \
    && rm -rf /var/lib/apt/lists/*

# --- 动态库路径 ---
ENV LD_LIBRARY_PATH=${WAMR_HOME}/lib:${WASMEDGE_HOME}/lib:${WASMER_HOME}/lib:${WASMTIME_HOME}/lib

# --- 构建 sysbench ---
COPY . /usr/src/sysbench
WORKDIR /usr/src/sysbench
RUN meson setup builddir -Dmysql=enabled -Dpgsql=enabled \
        -Dwasm=enabled -Dwamr=enabled -Dwasmedge=enabled \
    && meson compile -C builddir \
    && meson install -C builddir

WORKDIR /root
RUN rm -rf /usr/src/sysbench

ENTRYPOINT ["sysbench"]
```

---

## 12. 版本固定建议

当前所有 WASM 运行时 SDK 均未固定版本。Feature 016（Pinned SDK Provisioning）专门
负责版本固定。在此之前，建议镜像构建时在 Dockerfile 中写明具体版本号（如上文所
示），并在升级时显式更新版本号，避免隐式漂移影响基准测试结果的可比性
（Constitution Principle VI: Measurement Integrity）。

| SDK | 推荐版本 | 版本来源 |
|-----|---------|---------|
| WASI-SDK | wasi-sdk-25 | GitHub Release tag |
| WAMR | WAMR-2.2.0 | Git tag |
| WasmEdge | 0.14.1 | 安装脚本 `-v` 参数 |
| Wasmer | 4.3.7 | 安装脚本 `-v` 参数 |
| Wasmtime | 28.0.0 | GitHub Release tag |

---

## 13. 已知限制与注意事项

1. **Wasmer 和 Wasmtime 后端当前为 stub**。安装 SDK 后可以编译通过（条件编译包含
   对应 `.c` 文件），但运行 `--wasm-runtime=wasmer` 或 `--wasm-runtime=wasmtime` 会
   segfault。Feature 005/006 负责实现这两个后端，Feature 018 负责将 segfault 改为
   明确的错误提示。在此期间，镜像中安装这两个 SDK 是为了验证编译路径，不是功能
   验证。

2. **WasmEdge 不支持地址转换**。`src/sb_wasmedge.c` 未实现 `addr_app_to_native`/
   `addr_native_to_app`，因此缓冲区传递型负载（如 `pass_data`）在 WasmEdge 上无法
   运行。仅 WAMR 支持完整的数据交换（Feature 007）。

3. **WAMR 的 `wasm_runtime_full_init` 当前在每线程的 `create_sandbox` 中调用**，而非
   进程级初始化。这是 Feature 003 的已知问题，Feature 012 会修复。不影响编译，但
   多线程运行时可能有状态问题。

4. **aarch64 宿主不支持 WASM 模块交叉编译**。WASI-SDK 官方仅提供 x86_64 预编译包。
   aarch64 上的 sysbench 可以正常构建和运行（宿主二进制），但 WASM 负载模块需要在
   x86_64 环境或源码编译的 wasi-sdk 上预编译。

5. **`scripts/build-bundled.sh` 中的 `CK_CONFIGURE_FLAGS`**：在 aarch64 上构建
   Concurrency Kit 时，Meson 的 `third_party/meson.build` 目前不传递 CK 的
   `--enable-lse` 标志。如果镜像目标是 aarch64，需要确保 CK 的 LSE 原子指令探测
   正确工作。可以通过 `CK_CONFIGURE_FLAGS=--enable-lse` 环境变量传入。
