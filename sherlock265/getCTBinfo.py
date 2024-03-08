# 获取C++的结构体，在python里面将该结构体转化成python可用
# 或者传一个python结构体过去，让C++将结构体中的进行赋值，然后python可以直接使用
# 返回每一帧
# 返回一个列表

import ctypes
from ctypes import *
from time import sleep


class Decoder(ctypes.Structure):
    # 如果只是进行指针传递，不需要具体的数据类型，定义C++，也能够识别出指针类型
    # _fields_ = [("x", ctypes.c_int), ("y", ctypes.c_int)]
    pass


sotest = cdll.LoadLibrary("/data/chenminghui/test265/sherlock265/libgetCTBinfo.so")
sotest.getCTBinfo.restype = ctypes.POINTER(Decoder)  # 传出只能是一个值
sotest.deleteMdecoder.argtypes = [ctypes.POINTER(Decoder)]  # 传入需要一个列表


for i in range(2000):
    print(i, "=" * 20)
    res = sotest.getCTBinfo()
    sotest.deleteMdecoder(res)  # 主动释放进行内存的垃圾回收
    # sleep(1)  # 如果依赖python的gc，gc不会调用析构函数，且没有睡眠[时间太短]会直接爆内存

res = sotest.getCTBinfo()
sotest.deleteMdecoder(res)
