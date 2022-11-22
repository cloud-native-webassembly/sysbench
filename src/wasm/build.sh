export LD_LIBRARY_PATH=${LLVM_HOME}/lib64:${GCC_HOME}/lib64:${LD_LIBRARY_PATH}
export PATH=${LLVM_HOME}/bin:${GCC_HOME}/bin:${PATH}

clang --target=wasm32-wasi -fPIC \
      --sysroot=${WASI_SDK_HOME}/share/wasi-sysroot \
      -nostdlib \
      -z stack-size=8192 \
      -Wl,--initial-memory=65536 \
      -Wl,--no-entry \
      -Wl,--allow-undefined \
      -Wl,--export=event \
      -Wl,--export=buffer \
      -o sb_wasm_test.wasm sb_wasm_test.c