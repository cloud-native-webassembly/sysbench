sh autogen.sh 
./configure --without-mysql --with-wamr --with-wasmedge --with-debug
make clean 
make -j32
