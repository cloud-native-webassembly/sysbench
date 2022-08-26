
if [ -z "${PYTHON_HOME}" ]; then
    export PYTHON_HOME=/opt/python
    echo "PYTHON_HOME is not defined, using: ${PYTHON_HOME}"
fi

if [ -z "${WASM_HOME}" ]; then
    export WASM_HOME=/opt/wasmedge/0.10.1
    echo "WASM_HOME is not defined, using: ${WASM_HOME}"
fi

export LANG=C && sh autogen.sh && ./configure --without-mysql --with-python
make clean 
make -j32
