sh autogen.sh 
./configure --without-mysql --with-wamr --with-debug
make clean 
make -j32
