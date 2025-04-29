import os

import numpy as np

import dec265

DEBUG = False

def yuvToArray(listTuple, notSample, w, h):
    if notSample:
        return np.array(listTuple).reshape([h, w, -1])
    return np.array(listTuple[0]).reshape([h, w])  # TODO 处理yuv420

## 测试保存成numpy
def saveCTBinfo(img, saveList, idx):
    if DEBUG:
        print("w,h,cw,ch, chroma-format:", img.width_confwin, img.height_confwin, img.chroma_width_confwin, img.chroma_height_confwin,img.chroma_format)

    if "mv_f" in saveList: # 第3个维度代表refIdx
        mv_f = np.array(img.mv_f)[:,:,:2].clip(-128,127).astype('int8').transpose(1,0,2) 
        if DEBUG:
            print(idx, "mv_f", mv_f[:, :, 2].max(), mv_f[:, :, 2].min(), mv_f.shape)
        np.save(f"npy/{idx}_mv_f.npy", mv_f)
    if "mv_b" in saveList: # 第3个维度代表refIdx
        mv_b = np.array(img.mv_b)[:,:,:2].clip(-128,127).astype('int8').transpose(1,0,2) 
        if DEBUG:
            print(idx, "mv_b", mv_b.max(), mv_b.min(), mv_b.shape)
        np.save(f"npy/{idx}_mv_b.npy", mv_b)
    
    if "qp_y" in saveList: 
        quantPYs = np.array(img.quantPYs).clip(-128,127).astype('int8').transpose(1,0)
        if DEBUG:
            print(idx, "quantPYs", quantPYs.max(), quantPYs.min(), quantPYs.mean(), quantPYs.shape)
        np.save(f"npy/{idx}quantPYs.npy", quantPYs)

    if "residual" in saveList:   # 返回的是YUV格式
        if DEBUG:
            print("residual[0]/[1]/[2]:", len(img.residuals[0]), len(img.residuals[1]), len(img.residuals[2]))
        residuals = yuvToArray(
            img.residuals, img.width_confwin == img.chroma_width_confwin and img.width_confwin == img.chroma_height_confwin, img.width_confwin, img.height_confwin
        )
        residuals = residuals.clip(-128,127).astype('int8')
        if DEBUG:
            print(idx, "residuals", residuals.max(), residuals.min(), residuals.mean(), residuals.shape)
        np.save(f"npy/{idx}_residuals.npy", residuals)

    if "prediction" in saveList:  # 返回的是YUV格式
        predictions = yuvToArray(
            img.predictions, img.width_confwin == img.chroma_width_confwin and img.width_confwin == img.chroma_height_confwin, img.width_confwin, img.height_confwin
        )
        predictions = predictions.clip(0,255).astype('uint8')
        if DEBUG:
            print(idx, "predictions", predictions.max(), predictions.min(), predictions.mean(), predictions.shape)
        np.save(f"npy/{idx}_predictions.npy", predictions)
    
    if "decoded" in saveList:
        np.save(f"npy/{idx}_decoded.npy", (predictions.astype('int32')+residuals.astype('int32')).clip(0,255).astype('uint8'))

    if idx == 1 and DEBUG:
        exit()

def saveSliceType(img, slice_types):
    slice_types.append(img.slice_type)


def testSaveOneTimeBindImgName(filename="../testdata/girlshy.h265"):
    imglist = dec265.VectorDe265ImagePointer()
    slice_types = []
    dec265.getCTBinfo(imglist, filename)
    saveList = ["mv_b", "residual", "prediction", "qp_y","decoded"]
    os.makedirs("npy", exist_ok=True)
    print("frames: ", len(imglist))
    for i, img in enumerate(imglist):
        if DEBUG:
            print(f"process:{i} img...")
        saveCTBinfo(img, saveList, i)
        saveSliceType(img, slice_types)
    np.save("npy/slice_types.npy",np.array(slice_types))



if __name__ == "__main__":
    DEBUG = False
    testSaveOneTimeBindImgName(filename="../testdata/girlshy.h265")
   
