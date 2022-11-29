
cmd="${SYSBENCH} ${WAMR_OPTS} $@ src/wasm/wasm run"
echo cmd
${cmd}
