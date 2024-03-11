cd ../libde265
make -j8
cd ../dec265
rm dec265.cc
ln -s dec265-ctypes.cc dec265.cc
make -j8 
g++ -g -O2 -Werror=return-type -Werror=unused-result -Werror=reorder -std=gnu++11  -I/data/chenminghui/anaconda3/envs/invcomp/include -DDE265_LOG_ERROR -fPIC $(python3 -m pybind11 --includes) -shared -o libdec265.so  dec265-dec265.o  ../libde265/.libs/libde265.so -lstdc++ -lpthread -lm
# ./dec265 /data/chenminghui/test265/testdata/girlshy.h265
python getCTBinfo-ctypes.py
