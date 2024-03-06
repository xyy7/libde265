#include "VideoPlayer.hh"
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>

extern std::mutex myMutex;
extern std::condition_variable cv;
extern bool a_done;
extern bool c_done;

// void covert2python(de265_image *ptr[])
// {
// }
extern "C"
{
    void getCTBinfo(const char *filename = "/data/chenminghui/test265/testdata/girlshy.h265")
    {
        printf("python can call C function now...\n");
        // VideoDecoder *mDecoder = new VideoDecoder;
        // const de265_image *decodedImg[100];
        // mDecoder->init(filename);
        // mDecoder->start();

        // int decodedImgCount = 0;
        // for (;;)
        // {
        //     std::unique_lock<std::mutex> lock(myMutex);
        //     bool err = mDecoder->singleStepDecoder();
        //     c_done = false;
        //     a_done = true;
        //     cv.notify_one();
        //     if (err == false)
        //     {
        //         printf("break all now!\n");
        //         break;
        //     }
        //     while (!c_done)
        //     {
        //         cv.wait(lock);
        //     }
        //     // decodedImg[decodedImgCount++] = mDecoder->img;
        //     decodedImgCount++;
        // }
        // // TODO: 需要进行深拷贝
        // printf("I have collected %d decodedImg.!!!", decodedImgCount);
        // delete mDecoder;
    }
}
