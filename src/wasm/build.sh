export CLANG=${WASI_SDK_HOME}/bin/clang
export WASI_SDK_OPTS="--target=wasm32-wasi --sysroot=${WASI_SDK_HOME}/share/wasi-sysroot"
export WASM_OPTS="-fPIC -z stack-size=8192 -Wl,--initial-memory=65536 -Wl,--no-entry"
export WASM_EXPORTS="-Wl,--export=event "

bin=${1:-wasm}

${CLANG} ${WASI_SDK_OPTS} ${WASM_OPTS} ${WASM_EXPORTS} -o ${bin} ${bin}.c