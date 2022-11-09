clang --target=wasm32-wasi \
      --sysroot=${WASI_SDK_HOME}/share/wasi-sysroot \
      -nostdlib -z stack-size=8192 -Wl,--initial-memory=65536 -Wl,--no-entry -Wl,--allow-undefined \
      -Wl,--export=event \
      -o sb_wasm_test.wasm sb_wasm_test.c