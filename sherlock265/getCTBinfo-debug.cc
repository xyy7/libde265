#include "VideoPlayer.hh"
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "image.h"

extern std::mutex myMutex;
extern std::condition_variable cv;
extern bool a_done;
extern bool c_done;
extern de265_image *decodedImg[100];
extern int decodedImgCount;
// void covert2python(de265_image *ptr[])
// {
// }
extern "C"
{

    void deleteDecodedImg(de265_image **decodedImg)
    {
        ;
    }
    de265_image **getCTBinfo()
    {
        decodedImgCount = 0;
        deleteDecodedImg(decodedImg); // 需要注意这里释放的时间

        const char *filename = "/data/chenminghui/test265/testdata/girlshy.h265";
        VideoDecoder *mDecoder = new VideoDecoder;

        mDecoder->init(filename);
        mDecoder->start();

        for (;;)
        {
            bool err = mDecoder->singleStepDecoder();

            std::unique_lock<std::mutex> lock(myMutex); // 相当于myMutex.lock()
            printf("a lock playing video %d \n", mDecoder->mPlayingVideo);
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
                printf("a unlock playing video %d \n", mDecoder->mPlayingVideo);
                cv.wait(lock); // 相当于myMutex.unlock()+sleep()
            }
            printf("I have collected %d decodedImg.!!!\n", decodedImgCount);
        }
        // TODO: 需要进行深拷贝
        printf("I have collected %d decodedImg.!!!\n", decodedImgCount);
        delete mDecoder;
        printf("everything is over\n");
        // decodedImg[0]->get_all_metadata();
        for (int i = 0; i < decodedImgCount; ++i)
        {
            printf("%d pointer:%p\n", i, decodedImg[i]);
            // if (decodedImg[i] == nullptr)
            //     continue;
            // else
            // {
            //     // decodedImg[i]->get_all_metadata();
            //     break;
            // }
        }
        return decodedImg;
    }
    void deleteMdecoder(VideoDecoder *mDecoder)
    {
        delete mDecoder;
    }
    int main(int argc, char **argv)
    {
        getCTBinfo();
        return 0;
    }
}
