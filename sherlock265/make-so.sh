gcc -DQT_SHARED -I/usr/include/qt4 -I/usr/include/qt4/QtGui -I/usr/include/qt4 -I/usr/include/qt4/QtCore -I../libde265 -I.. -std=c++0x -fPIC -shared -I/usr/include/x86_64-linux-gnu -g -O2 -Werror=return-type -Werror=unused-result -Werror=reorder -std=gnu++11 -DDE265_LOG_ERROR -o libgetCTBinfo.so getCTBinfo.cc sherlock265-VideoPlayer.o sherlock265-VideoDecoder.o sherlock265-VideoWidget.o sherlock265-moc_VideoPlayer.o sherlock265-moc_VideoDecoder.o sherlock265-moc_VideoWidget.o  -lQtGui -lQtCore -lswscale ../libde265/.libs/libde265.so -lstdc++ -lpthread -lm