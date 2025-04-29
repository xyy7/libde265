# 获取C++的结构体，在python里面将该结构体转化成python可用
# 或者传一个python结构体过去，让C++将结构体中的进行赋值，然后python可以直接使用
# 返回每一帧
# 返回一个列表

import ctypes
import os
from ctypes import *
from time import sleep

# class ReturnClass(ctypes.Structure):
#     _fields_ = [("decodedImg", ctypes.POINTER(ctypes.Structure)), ("mDecoder", ctypes.POINTER(ctypes.Structure))]


class Decoder(ctypes.Structure):
    # 如果只是进行指针传递，不需要具体的数据类型，定义C++，也能够识别出指针类型
    # _fields_ = [("x", ctypes.c_int), ("y", ctypes.c_int)]
    pass


class CB_ref_info(ctypes.Structure):
    _fields_ = [
        ("log2CbSize", ctypes.c_ubyte),
        ("PartMode", ctypes.c_ubyte),
        ("ctDepth", ctypes.c_ubyte),
        ("cu_transquant_bypass", ctypes.c_ubyte),
        ("pcm_flag", ctypes.c_ubyte),
        ("PredMode", ctypes.c_ubyte),
        ("QP_Y", ctypes.c_char),
    ]


# MetaDataArray<CTB_info>
class MetaDataArray(ctypes.Structure):
    _fields_ = [
        ("data_size", ctypes.c_int),
        ("log2unitSize", ctypes.c_int),
        ("width_in_units", ctypes.c_int),
        ("height_in_units", ctypes.c_int),
        ("data", ctypes.POINTER(CB_ref_info)),  # 这里在C++中是模板
    ]


class De265Image(ctypes.Structure):
    #   MetaDataArray<CTB_info> ctb_info;
    #   MetaDataArray<CB_ref_info> cb_info;
    #   MetaDataArray<PBMotion> pb_info;
    #   MetaDataArray<uint8_t> intraPredMode;
    #   MetaDataArray<uint8_t> intraPredModeC;
    #   MetaDataArray<uint8_t> tu_info;
    #   MetaDataArray<uint8_t> deblk_info;
    _fields_ = [("cb_info", MetaDataArray)]


sotest = cdll.LoadLibrary("/data/chenminghui/test265/sherlock265/libgetCTBinfo.so")
# sotest.getCTBinfo.restype = ctypes.POINTER(Decoder)  # 传出只能是一个值
sotest.getCTBinfo.restype = ctypes.POINTER(ctypes.POINTER(De265Image))  # 传出只能是一个值
# sotest.deleteMdecoder.argtypes = [ctypes.POINTER(Decoder)]  # 传入需要一个列表


for i in range(2):
    print(i, "=" * 20, os.getpid())
    res = sotest.getCTBinfo()  # res[1][1].cb_info.data.contents
    # sotest.deleteMdecoder(res)  # 主动释放进行内存的垃圾回收
    # sleep(1)  # 如果依赖python的gc，gc不会调用析构函数，且没有睡眠[时间太短]会直接爆内存

# res = sotest.getCTBinfo()
# sotest.deleteMdecoder(res)
