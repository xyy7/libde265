cd ../libde265
make clean
make -j8
cd ../dec265
rm dec265.cc
ln -s dec265-pybind11.cc dec265.cc
make clean
make -j8 
g++ -g -O2 -Werror=return-type -Werror=unused-result -Werror=reorder -std=gnu++11  -I/data/chenminghui/anaconda3/envs/invcomp/include -DDE265_LOG_ERROR -fPIC $(python3 -m pybind11 --includes) -shared -o dec265`python3-config --extension-suffix` dec265-dec265.o  ../libde265/.libs/libde265.so -lstdc++ -lpthread -lm  
python getCTBinfo-pybind11.py
