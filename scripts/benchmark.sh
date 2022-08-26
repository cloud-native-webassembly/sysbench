export UDF_HOME=${PWD}/udf
export WORKSPACE=/workspace
export PY_PERF=${WORKSPACE}/python.perf.data
export PY_SVG=/www/python.svg
export WASM_PERF=${WORKSPACE}/wasm.perf.data
export WASM_SVG=/www/wasm.svg
export PATH=/opt/flamegraph:${PATH}

perf record -g -o ${PY_PERF} -- ./src/sysbench --time=120 --type=python fibonacci run
perf script -i ${PY_PERF} | stackcollapse-perf.pl | flamegraph.pl > ${PY_SVG}
perf record -g -o ${WASM_PERF} -- ./src/sysbench --time=120 --type=wasm ${UDF_HOME}/fibonacci.wasm run
perf script -i ${WASM_PERF} | stackcollapse-perf.pl | flamegraph.pl > ${WASM_SVG}
