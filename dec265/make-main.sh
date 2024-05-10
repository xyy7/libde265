cd ../libde265
make -j8
cd ../dec265
rm dec265.cc
ln -s dec265-main.cc dec265.cc
make clean
make -j8 
./dec265 /data/chenminghui/test265/testdata/girlshy.h265
