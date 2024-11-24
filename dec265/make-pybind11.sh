cd ../libde265
make clean
make -j16
cd ../dec265
rm dec265.cc
ln -s dec265-pybind11.cc dec265.cc
make clean
make -j16
# compile *.so for pybind11
export LD_LIBRARY_PATH=../libde265/.libs:$LD_LIBRARY_PATH
g++ -g -O2 -Werror=return-type -Werror=unused-result -Werror=reorder -std=gnu++11 -DDE265_LOG_ERROR -fPIC $(python3 -m pybind11 --includes) -shared -o dec265`python3-config --extension-suffix` dec265-dec265.o  ../libde265/.libs/libde265.so -lstdc++ -lpthread -lm  
# python getCTBinfo-pybind11.py
