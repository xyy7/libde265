import os


# 输入文件名【C sprintf格式】，保存文件名，编解码器，码率
def x265(input, output, codec="-c:v libx265 -f rawvideo", crf="-crf 23"):
    cmd = f"ffmpeg -i {input} {codec} {crf} {output}"
    print(cmd)
    os.system(cmd)


def x265_dataset(datasetName):
    # 输入要求有一系列文件夹
    # 然后将文件夹中的图像序列都转化成相应的格式
    pass


def jpg2pngJustExt(dirName):
    # 将文件后缀从jpg改成png
    imgs = os.listdir(dirName)
    print(imgs)
    imgs.sort()
    for img in imgs:
        os.rename(os.path.join(dirName, img), os.path.join(dirName, img.replace("jpg", "png")))


if __name__ == "__main__":
    root = "testImgSeq"
    # jpg2pngJustExt(root)

    input = "testImgSeq/I%05d.png"
    output = "test.h265"
    x265(input, output)
