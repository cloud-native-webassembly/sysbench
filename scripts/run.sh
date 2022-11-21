export LD_LIBRARY_PATH=${LLVM_HOME}/lib64:${GCC_HOME}/lib64
./src/sysbench --type=wasm --wasm-runtime=wamr --verbosity=5 src/wasm/sb_wasm_test.wasm run
