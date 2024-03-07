# 获取C++的结构体，在python里面将该结构体转化成python可用
# 或者传一个python结构体过去，让C++将结构体中的进行赋值，然后python可以直接使用
# 返回每一帧
# 返回一个列表

import gc
import os
from ctypes import *

gc.disable()
sotest = cdll.LoadLibrary("/data/chenminghui/test265/sherlock265/libgetCTBinfo.so")

# sotest.print_msg("hello,my shared object used by python!")
# print("4+5=%s" % sotest.add_Integer(4, 5))
sotest.getCTBinfo()
