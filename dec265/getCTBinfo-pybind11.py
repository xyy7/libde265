import os

import numpy as np

import dec265

DEBUG = False

## 测试是否正常析构
def testOneTime1():
    dec265.getCTBinfo1()


def testSeveralTimes1(times=10):
    for i in range(10):
        dec265.getCTBinfo1()


## 测试输入[], 能否正常返回
def testOneTime():
    imglist = []
    res = dec265.getCTBinfo(imglist)


def testSeveralTimes(times=10):
    imglist = []
    for i in range(10):
        res = dec265.getCTBinfo(imglist)


## 测试STLBind函数
def testOneTimeSTLbind():
    imglist = dec265.VectorDe265ImagePointer()
    res = dec265.getCTBinfo(imglist)


def testSeveralTimesSTLbind(times=10):
    imglist = dec265.VectorDe265ImagePointer()
    for i in range(10):
        res = dec265.getCTBinfo(imglist)


## 测试输入名字是否正确
def testOneTimeBindImgName(filename="/data/chenminghui/test265/dec265/test.h265"):
    imglist = dec265.VectorDe265ImagePointer()
    res = dec265.getCTBinfo(imglist, filename)


def testSeveralTimeBindImgName(filename="/data/chenminghui/test265/dec265/test.h265"):
    imglist = dec265.VectorDe265ImagePointer()
    for i in range(10):
        res = dec265.getCTBinfo(imglist, filename)


## 测试保存成numpy
def saveCTBinfo(img, saveList, idx):
    if "cb_info" in saveList:
        pass
    if "mv_f" in saveList:
        mv_f = np.array(img.mv_f)
        if DEBUG:
            print(idx, "mv_f", mv_f[:, :, 2].max(), mv_f[:, :, 2].min())
        np.save(f"npy/{idx}_mv_f.npy", mv_f)
    if "mv_b" in saveList:
        mv_b = np.array(img.mv_b)
        if DEBUG:
            print(idx, "mv_b", mv_b[:, :, 2].max(), mv_b[:, :, 2].min())
        np.save(f"npy/{idx}_mv_b.npy", mv_b)
    if "residual" in saveList:
        residuals = np.array(img.residuals)
        if DEBUG:
            print(idx, "residuals", residuals.max(), residuals.min(), residuals.mean())
        np.save(f"npy/{idx}_residuals.npy", residuals)
    if "prediction" in saveList:
        predictions = np.array(img.predictions)
        if DEBUG:
            print(idx, "predictions", predictions.max(), predictions.min(), predictions.mean())
        np.save(f"npy/{idx}_predictions.npy", predictions)

    # size = (n, h, w)
    # cb_info =
    # ctb_info =
    # deblk_info =
    # intraPredMode =
    # intraPredModeC =
    # pb_info =
    # tu_ino =
    # pass


def testSaveOneTimeBindImgName(filename="/data/chenminghui/test265/dec265/test.h265"):
    imglist = dec265.VectorDe265ImagePointer()
    res = dec265.getCTBinfo(imglist, filename)
    saveList = ["mv_f", "mv_b", "residual", "prediction"]
    os.makedirs("npy", exist_ok=True)
    for i, img in enumerate(imglist):
        saveCTBinfo(img, saveList, i)
        if i > 5:
            break


if __name__ == "__main__":
    DEBUG = True
    # testOneTime()
    # testOneTimeSTLbind()

    # testSeveralTimes()
    # testSeveralTimesSTLbind()

    # testOneTimeBindImgName()
    # testSeveralTimeBindImgName()

    testSaveOneTimeBindImgName()
    pass
