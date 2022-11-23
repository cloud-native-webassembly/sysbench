
export LD_LIBRARY_PATH=${LLVM_HOME}/lib64:${GCC_HOME}/lib64
export PATH=${GCC_HOME}/bin:${PATH}
export LANG=C 
sh autogen.sh 
./configure --without-mysql --with-wamr --with-debug
make clean 
make -j32
