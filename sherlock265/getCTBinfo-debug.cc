#include "VideoPlayer.hh"
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <sys/time.h>

#include <sys/syscall.h> /*必须引用这个文件 */
pid_t gettid(void)
{
    return syscall(SYS_gettid);
}

extern std::mutex myMutex;
extern std::condition_variable cv;
extern bool a_done;
extern bool c_done;

// void covert2python(de265_image *ptr[])
// {
// }
void print_current_time1(const char *extra)
{
    struct timeval tv;
    struct tm *info;
    char buffer[80];

    gettimeofday(&tv, NULL);
    info = localtime(&tv.tv_sec);

    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", info);
    printf("pid:%u,Current time: %s.%06ld %s\n", gettid(), buffer, tv.tv_usec, extra);
}

extern "C"
{
    void getCTBinfo(const char *filename = "/data/chenminghui/test265/testdata/girlshy.h265")
    {
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
    }

    int main(int argc, char **argv)
    {
        getCTBinfo();
        return 0;
    }
}
