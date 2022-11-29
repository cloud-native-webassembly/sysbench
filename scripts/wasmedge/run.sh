
cmd="${SYSBENCH} ${WASMEDGE_OPTS} $@ src/wasm/wasm run"
echo cmd
${cmd}