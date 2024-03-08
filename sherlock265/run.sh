cd ../libde265
make -j 8 
cd ../sherlock265
make -j 8
sh make-debug.sh
./getCTBinfo
#./sherlock265 /data/chenminghui/libde265/testdata/girlshy.h265
