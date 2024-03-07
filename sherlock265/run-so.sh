cd ../libde265
make -j 8 
cd ../sherlock265
make -j 8
sh make-so.sh
python getCTBinfo.py
#./sherlock265 /data/chenminghui/libde265/testdata/girlshy.h265
