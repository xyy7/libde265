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


def yuvRepeat(listTuple, chroma_format):
    # de265_chroma_mono=0,
    # de265_chroma_420=1,
    # de265_chroma_422=2,
    # de265_chroma_444=3

    # TODO:处理不了enum
    if chroma_format == 0 or chroma_format == 3:
        return np.array(listTuple)
    # listTuple[0] is luma listtuple[1]/[2] is chroma
    if chroma_format == 1:
        # repeat the chroma to luma shape
        pass
        # return np.array(listTuple[0])

    if chroma_format == 2:
        # repeat the chroma to luma shape
        pass
        return np.array(listTuple[0])


def yuvToArray(listTuple, notSample, w, h):
    if notSample:
        return np.array(listTuple).reshape([w, h, -1])
    return np.array(listTuple[0]).reshape([w, h])


## 测试保存成numpy
def saveCTBinfo(img, saveList, idx):
    if DEBUG:
        print("w,h,cw,ch, chroma-format:", img.width_confwin, img.height_confwin, img.chroma_width_confwin, img.chroma_height_confwin,img.chroma_format)


    if "mv_f" in saveList:
        mv_f = np.array(img.mv_f)
        if DEBUG:
            print(idx, "mv_f", mv_f[:, :, 2].max(), mv_f[:, :, 2].min(), mv_f.shape)
        np.save(f"npy/{idx}_mv_f.npy", mv_f)
    if "mv_b" in saveList:
        mv_b = np.array(img.mv_b)
        if DEBUG:
            print(idx, "mv_b", mv_b[:, :, 2].max(), mv_b[:, :, 2].min(), mv_b.shape)
        np.save(f"npy/{idx}_mv_b.npy", mv_b)

    if "residual" in saveList:
        if DEBUG:
            print("residual[0]/[1]/[2]:", len(img.residuals[0]), len(img.residuals[1]), len(img.residuals[2]))
        residuals = yuvToArray(
            img.residuals, img.width_confwin == img.chroma_width_confwin and img.width_confwin == img.chroma_height_confwin, img.width_confwin, img.height_confwin
        )
        if DEBUG:
            print(idx, "residuals", residuals.max(), residuals.min(), residuals.mean(), residuals.shape)
        np.save(f"npy/{idx}_residuals.npy", residuals)

    if "prediction" in saveList:
        predictions = yuvToArray(
            img.predictions, img.width_confwin == img.chroma_width_confwin and img.width_confwin == img.chroma_height_confwin, img.width_confwin, img.height_confwin
        )
        if DEBUG:
            print(idx, "predictions", predictions.max(), predictions.min(), predictions.mean(), predictions.shape)
        np.save(f"npy/{idx}_predictions.npy", predictions)

    if "qp_y" in saveList:
        quantPYs = np.array(img.quantPYs)
        if DEBUG:
            print(idx, "quantPYs", quantPYs.max(), quantPYs.min(), quantPYs.mean(), quantPYs.shape)
        np.save(f"npy/{idx}quantPYs.npy", quantPYs)

    if idx == 1 and DEBUG:
        exit()

def saveSliceType(img, slice_types):
    slice_types.append(img.slice_type)


def testSaveOneTimeBindImgName(filename="/data/chenminghui/test265/dec265/test.h265"):
    imglist = dec265.VectorDe265ImagePointer()
    slice_types = []
    res = dec265.getCTBinfo(imglist, filename)
    saveList = ["mv_f", "mv_b", "residual", "prediction", "qp_y"]
    os.makedirs("npy", exist_ok=True)
    print("frames: ", len(imglist))
    for i, img in enumerate(imglist):
        saveCTBinfo(img, saveList, i)
        saveSliceType(img, slice_types)
    np.save("slice_types",np.array(slice_types))



if __name__ == "__main__":
    DEBUG = True
    # testOneTime()
    # testOneTimeSTLbind()

    # testSeveralTimes()
    # testSeveralTimesSTLbind()

    # testOneTimeBindImgName()

    # testSeveralTimeBindImgName()
    testSaveOneTimeBindImgName(filename="/data/chenminghui/test265/testdata/girlshy.h265")
    # testSaveOneTimeBindImgName(filename="/data/chenminghui/CompUpSamplingDataset/Vimeo90k/sequences/00049/0311_23.bin")
    # testSaveOneTimeBindImgName(filename="/data/chenminghui/CompUpSamplingDataset/Vid4/BDx4_not_compressed/calendar_23.bin")
    pass
