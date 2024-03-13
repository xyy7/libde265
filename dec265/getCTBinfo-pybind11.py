import dec265


def testOneTime1():
    res = dec265.getCTBinfo1()


def testSeveralTimes1(times=10):
    for i in range(10):
        res = dec265.getCTBinfo1()


def testOneTime():
    imglist = []
    res = dec265.getCTBinfo(imglist)


def testSeveralTimes(times=10):
    imglist = []
    for i in range(10):
        res = dec265.getCTBinfo(imglist)


if __name__ == "__main__":
    # testOneTime()
    testSeveralTimes()
