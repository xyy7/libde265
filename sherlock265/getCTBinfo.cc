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
    VideoDecoder *getCTBinfo()
    {
        const char *filename = "/data/chenminghui/test265/testdata/girlshy.h265";
        VideoDecoder *mDecoder = new VideoDecoder;
        const de265_image *decodedImg[100];
        mDecoder->init(filename);
        mDecoder->start();

        int decodedImgCount = 0;
        for (;;)
        {
            bool err = mDecoder->singleStepDecoder();

            std::unique_lock<std::mutex> lock(myMutex); // 相当于myMutex.lock()
            // printf("a lock playing video %d \n", mDecoder->mPlayingVideo);
            a_done = true;
            c_done = false;

            cv.notify_one();
            if (err == false)
            {
                printf("break all now!\n");
                break;
            }
            while (!c_done)
            {
                // printf("a unlock playing video %d \n", mDecoder->mPlayingVideo);
                cv.wait(lock); // 相当于myMutex.unlock()+sleep()
            }
            printf("I have collected %d decodedImg.!!!\n", ++decodedImgCount);
        }
        // TODO: 需要进行深拷贝
        printf("I have collected %d decodedImg.!!!\n", decodedImgCount);
        // delete mDecoder;
        printf("everything is over\n");
        return mDecoder;
    }
    void deleteMdecoder(VideoDecoder *mDecoder)
    {
        delete mDecoder;
    }
}
