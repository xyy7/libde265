/*
  libde265 example application "sherlock265".

  MIT License

  Copyright (c) 2013-2014 struktur AG, Dirk Farin <farin@struktur.de>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#include "VideoPlayer.hh"
#include <unistd.h>
#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex myMutex;
std::condition_variable cv;
bool a_done;
bool c_done;

VideoPlayer::VideoPlayer(const char *filename)
{
  mDecoder = new VideoDecoder;
  mDecoder->init(filename);
  mDecoder->start();
  // mDecoder->showSlices(true); // 似乎不能正确展示
  // mDecoder->showTiles(true); // 似乎不能正确展示
  for (;;)
  {
    std::unique_lock<std::mutex> lock(myMutex);

    bool err = mDecoder->singleStepDecoder();
    mDecoder->showCBPartitioning(true); // 绘制是多线程的
    // mDecoder->showCBPartitioning(false);  //如果在这里修改，那么可能这里执行的速度比绘制的速度快的多，直接给改回来了
    mDecoder->showPBPartitioning(true);
    // mDecoder->showPBPartitioning(false);
    mDecoder->showTBPartitioning(true);
    // mDecoder->showTBPartitioning(false);
    mDecoder->showIntraPredMode(true);
    // mDecoder->showIntraPredMode(false);
    mDecoder->showPBPredMode(true);
    // mDecoder->showPBPredMode(false);
    mDecoder->showMotionVec(true);
    // mDecoder->showMotionVec(false);
    mDecoder->showQuantPY(true);
    // mDecoder->showQuantPY(false);
    mDecoder->showDecodedImage(true);

    c_done = false;
    a_done = true;
    cv.notify_one();
    if (err == false)
    {
      printf("break all now!\n");
      break;
    }

    // c_done = false;
    // a_done = true;
    // cv.notify_one();
    while (!c_done)
    {
      cv.wait(lock);
    }
  }
}
