# sysbench container image.
#
# Also the vehicle for the base-image rebuild the framework round requires: the
# previous image (ubuntu:xenial + autotools) lacked meson, ninja, WASI-SDK, and
# the WASM runtime SDKs, which is why every build/test task in this round is
# deferred. See docs/build/migration-status.md.

FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        meson \
        ninja-build \
        pkg-config \
        gcc \
        libc6-dev \
        make \
        libaio-dev \
        git \
        python3 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# For MySQL support
RUN apt-get update && apt-get install -y --no-install-recommends \
        libmysqlclient-dev libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# For PostgreSQL support
RUN apt-get update && apt-get install -y --no-install-recommends \
        libpq-dev \
    && rm -rf /var/lib/apt/lists/*

COPY . /usr/src/sysbench
WORKDIR /usr/src/sysbench

RUN meson setup builddir -Dmysql=enabled -Dpgsql=enabled \
    && meson compile -C builddir \
    && meson install -C builddir

WORKDIR /root
RUN rm -rf /usr/src/sysbench

ENTRYPOINT ["sysbench"]
