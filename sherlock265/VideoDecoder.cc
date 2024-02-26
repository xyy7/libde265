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

#include "VideoDecoder.hh"
#ifdef HAVE_VIDEOGFX
#include <libvideogfx.hh>
#endif

#ifdef HAVE_VIDEOGFX
using namespace videogfx;
#endif

// #include "decctx.h"
#include "visualize.h"

#include <sys/stat.h>
#include <unistd.h>

void create_directory(const char *filename)
{
  // 检查当前目录是否存在指定的文件夹
  struct stat st;
  if (stat(filename, &st) == -1)
  {
    // 如果不存在，则创建文件夹
    mkdir(filename, 0700);
    printf("Created directory: %s\n", filename);
  }
  else
  {
    printf("Directory '%s' already exists\n", filename);
  }
}

VideoDecoder::VideoDecoder()
    : mFH(NULL),
      ctx(NULL),
      img(NULL),
      mNextBuffer(0),
      mFrameCount(0),
      mPlayingVideo(false),
      mVideoEnded(false),
      mSingleStep(false),
      mShowDecodedImage(true),
      mShowQuantPY(false),
      mCBShowPartitioning(false),
      mTBShowPartitioning(false),
      mPBShowPartitioning(false),
      mShowIntraPredMode(false),
      mShowPBPredMode(false),
      mShowMotionVec(false),
      mShowTiles(false),
      mShowSlices(false)
#ifdef HAVE_SWSCALE
      ,
      sws(NULL), width(0), height(0)
#endif
{
}

VideoDecoder::~VideoDecoder()
{
  free_decoder();
#ifdef HAVE_SWSCALE
  if (sws != NULL)
  {
    sws_freeContext(sws);
  }
#endif
}

void VideoDecoder::run()
{
  decoder_loop();
}

void VideoDecoder::init(const char *filename)
{
  init_decoder(filename);
}

void VideoDecoder::startDecoder()
{
  if (mPlayingVideo || mVideoEnded)
  {
    return;
  }

  mPlayingVideo = true;
  exit();
}

void VideoDecoder::stopDecoder()
{
  if (!mPlayingVideo)
  {
    return;
  }

  mPlayingVideo = false;
}

void VideoDecoder::singleStepDecoder()
{
  if (mPlayingVideo || mVideoEnded)
  {
    return;
  }

  mPlayingVideo = true;
  mSingleStep = true;
  exit();
}

void VideoDecoder::decoder_loop()
{
  mFrameIndex = 0;
  for (;;)
  {
    if (mPlayingVideo)
    {
      mutex.lock();

      if (img)
      {
        img = NULL;
        de265_release_next_picture(ctx); // 释放之前帧
      }

      img = de265_peek_next_picture(ctx); // 获取新的一帧
      while (img == NULL)                 // 没有新的一帧了，那就等draw函数来更新img
      {
        mutex.unlock();
        int more = 1;
        de265_error err = de265_decode(ctx, &more);
        mutex.lock();
        // printf("2step: %d,%d,%d,DE265_OK:%d,ERROR:%d\n", ++step, more, err, DE265_OK, DE265_ERROR_WAITING_FOR_INPUT_DATA);
        if (more && err == DE265_OK)
        {
          // try again to get picture
          img = de265_peek_next_picture(ctx);
        }
        else if (more && err == DE265_ERROR_WAITING_FOR_INPUT_DATA)
        {
          uint8_t buf[4096];
          int buf_size = fread(buf, 1, sizeof(buf), mFH);
          // printf("bufsize:%d\n", buf_size);
          int err = de265_push_data(ctx, buf, buf_size, 0, 0);
        }
        else if (!more)
        {
          mVideoEnded = true;
          mPlayingVideo = false; // TODO: send signal back
          printf("no more picture!\n");
          break;
        }
      }

      // show one decoded picture

      if (img)
      {
        create_directory("rec");
        char filename[20];
        sprintf(filename, "rec/%02d.png", mFrameIndex);
        printf("%s\n", filename);
        show_frame(img, filename);

        if (mSingleStep)
        {
          mSingleStep = false;
          mPlayingVideo = false;
          printf("step: %d\n", ++mFrameIndex);
        }
      }

      mutex.unlock();

      // process events

      QCoreApplication::processEvents(); // 处理QT事件，确保图像的显示和更新。
    }
    else
    {
      exec(); // 如果等不到mPlayingVdieo信号，那么就循环等待
    }
  }
}

#ifdef HAVE_VIDEOGFX
void VideoDecoder::convert_frame_libvideogfx(const de265_image *img, QImage &qimg)
{
  // --- convert to RGB ---

  de265_chroma chroma = de265_get_chroma_format(img);

  int map[3];

  Image<Pixel> visu;
  if (chroma == de265_chroma_420)
  {
    visu.Create(img->get_width(), img->get_height(), Colorspace_YUV, Chroma_420);
    map[0] = 0;
    map[1] = 1;
    map[2] = 2;
  }
  else
  {
    visu.Create(img->get_width(), img->get_height(), Colorspace_RGB, Chroma_444);
    map[0] = 1;
    map[1] = 2;
    map[2] = 0;
  }

  for (int y = 0; y < img->get_height(0); y++)
  {
    memcpy(visu.AskFrame(BitmapChannel(map[0]))[y],
           img->get_image_plane_at_pos(0, 0, y), img->get_width(0));
  }

  for (int y = 0; y < img->get_height(1); y++)
  {
    memcpy(visu.AskFrame(BitmapChannel(map[1]))[y],
           img->get_image_plane_at_pos(1, 0, y), img->get_width(1));
  }

  for (int y = 0; y < img->get_height(2); y++)
  {
    memcpy(visu.AskFrame(BitmapChannel(map[2]))[y],
           img->get_image_plane_at_pos(2, 0, y), img->get_width(2));
  }

  Image<Pixel> debugvisu;
  ChangeColorspace(debugvisu, visu, Colorspace_RGB);

  // --- convert to QImage ---

  uchar *ptr = qimg.bits();
  int bpl = qimg.bytesPerLine();

  for (int y = 0; y < img->get_height(); y++)
  {
    for (int x = 0; x < img->get_width(); x++)
    {
      *(uint32_t *)(ptr + x * 4) = ((debugvisu.AskFrameR()[y][x] << 16) |
                                    (debugvisu.AskFrameG()[y][x] << 8) |
                                    (debugvisu.AskFrameB()[y][x] << 0));
    }

    ptr += bpl;
  }
}
#endif

#ifdef HAVE_SWSCALE
void VideoDecoder::convert_frame_swscale(const de265_image *img, QImage &qimg)
{
  if (sws == NULL || img->get_width() != width || img->get_height() != height)
  {
    if (sws != NULL)
    {
      sws_freeContext(sws);
    }
    width = img->get_width();
    height = img->get_height();
    sws = sws_getContext(width, height, AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_BGRA, SWS_FAST_BILINEAR, NULL, NULL, NULL);
  }

  int stride[3];
  const uint8_t *data[3];
  for (int c = 0; c < 3; c++)
  {
    data[c] = img->get_image_plane(c);
    stride[c] = img->get_image_stride(c);
  }

  uint8_t *qdata[1] = {(uint8_t *)qimg.bits()};
  int qstride[1] = {qimg.bytesPerLine()};
  sws_scale(sws, data, stride, 0, img->get_height(), qdata, qstride);
}
#endif

void VideoDecoder::show_frame(const de265_image *img, const char *filename)
{
  if (mFrameCount == 0)
  {
    mImgBuffers[0] = QImage(QSize(img->get_width(), img->get_height()), QImage::Format_RGB32);
    mImgBuffers[1] = QImage(QSize(img->get_width(), img->get_height()), QImage::Format_RGB32);
  }

  // --- convert to RGB (or generate a black image if video image is disabled) ---

  QImage *qimg = &mImgBuffers[mNextBuffer];
  uchar *ptr = qimg->bits();
  int bpl = qimg->bytesPerLine();

  if (mShowDecodedImage)
  {
#ifdef HAVE_VIDEOGFX
    convert_frame_libvideogfx(img, *qimg);
#elif HAVE_SWSCALE
    convert_frame_swscale(img, *qimg);
#else
    qimg->fill(QColor(0, 0, 0));
#endif
  }
  else
  {
    qimg->fill(QColor(0, 0, 0));
  }

  // --- overlay coding-mode visualization ---

  if (mShowQuantPY)
  {
    // sprintf(newFileName, "QuantPY_%s", filename);
    draw_QuantPY(img, ptr, bpl, 4);
  }

  if (mShowPBPredMode)
  {
    // sprintf(newFileName, "PBPredMode_%s", filename);
    draw_PB_pred_modes(img, ptr, bpl, 4);
  }

  if (mShowIntraPredMode)
  {
    // sprintf(newFileName, "IntraPredMode_%s", filename);
    draw_intra_pred_modes(img, ptr, bpl, 0x009090ff, 4);
  }

  if (mTBShowPartitioning)
  {
    // sprintf(newFileName, "TB_%s", filename);
    draw_TB_grid(img, ptr, bpl, 0x00ff6000, 4);
  }

  if (mPBShowPartitioning)
  {
    // sprintf(newFileName, "PB_%s", filename);
    draw_PB_grid(img, ptr, bpl, 0x00e000, 4);
  }

  if (mCBShowPartitioning)
  {
    // sprintf(newFileName, "CB_%s", filename);
    draw_CB_grid(img, ptr, bpl, 0x00FFFFFF, 4);
  }

  if (mShowMotionVec)
  {
    // sprintf(newFileName, "motionVec_%s", filename);
    draw_Motion(img, ptr, bpl, 4);
  }

  if (mShowSlices)
  {
    // sprintf(newFileName, "slices_%s", filename);
    draw_Slices(img, ptr, bpl, 4);
  }

  if (mShowTiles)
  {
    // sprintf(newFileName, "tiles_%s", filename);
    draw_Tiles(img, ptr, bpl, 4);
  }

  emit displayImage(qimg);
  printf("you can save img here!\n");
  qimg->save(filename);
  mNextBuffer = 1 - mNextBuffer;
  mFrameCount++;
  printf("this is %d frame (drawing).\n", mFrameCount);
}

void VideoDecoder::showCBPartitioning(bool flag)
{
  mCBShowPartitioning = flag;

  mutex.lock();
  if (img != NULL)
  {
    create_directory("recCB");
    char filename[20];
    sprintf(filename, "recCB/CB_%02d.png", mFrameIndex);
    show_frame(img, filename);
  }
  mutex.unlock();
}

void VideoDecoder::showTBPartitioning(bool flag)
{
  mTBShowPartitioning = flag;

  mutex.lock();
  if (img != NULL)
  {
    create_directory("recTB");
    char filename[20];
    sprintf(filename, "recTB/TB_%02d.png", mFrameIndex);
    show_frame(img, filename);
  }
  mutex.unlock();
}

void VideoDecoder::showPBPartitioning(bool flag)
{
  mPBShowPartitioning = flag;

  mutex.lock();
  if (img != NULL)
  {
    create_directory("recPB");
    char filename[20];
    sprintf(filename, "recPB/PB_%02d.png", mFrameIndex);
    show_frame(img, filename);
  }
  mutex.unlock();
}

void VideoDecoder::showIntraPredMode(bool flag)
{
  mShowIntraPredMode = flag;

  mutex.lock();
  if (img != NULL)
  {
    create_directory("recIntraPredMode");
    char filename[20];
    sprintf(filename, "recIntraPredMode/IntraPredMode_%02d.png", mFrameIndex);
    show_frame(img, filename);
  }
  mutex.unlock();
}

void VideoDecoder::showPBPredMode(bool flag)
{
  mShowPBPredMode = flag;

  mutex.lock();
  if (img != NULL)
  {
    create_directory("recPBPredMode");
    char filename[20];
    sprintf(filename, "recPBPredMode/PBPred_mode_%02d.png", mFrameIndex);
    show_frame(img, filename);
  }
  mutex.unlock();
}

void VideoDecoder::showQuantPY(bool flag)
{
  mShowQuantPY = flag;

  mutex.lock();
  if (img != NULL)
  {
    create_directory("recQuantPY");
    char filename[20];
    sprintf(filename, "recQuantPY/QuantPY_%02d.png", mFrameIndex);
    show_frame(img, filename);
  }
  mutex.unlock();
}

void VideoDecoder::showMotionVec(bool flag)
{
  mShowMotionVec = flag;

  mutex.lock();
  if (img != NULL)
  {
    create_directory("recMV");
    char filename[20];
    sprintf(filename, "recMV/MV_%02d.png", mFrameIndex);
    show_frame(img, filename);
  }
  mutex.unlock();
}

void VideoDecoder::showDecodedImage(bool flag)
{
  mShowDecodedImage = flag;

  mutex.lock();
  if (img != NULL)
  {
    create_directory("recDecodedImage");
    char filename[20];
    sprintf(filename, "recDecodedImage/DecodedImage_%02d.png", mFrameIndex);
    show_frame(img, filename);
  }
  mutex.unlock();
}

void VideoDecoder::showTiles(bool flag)
{
  mShowTiles = flag;

  mutex.lock();
  if (img != NULL)
  {
    create_directory("recTiles");
    char filename[20];
    sprintf(filename, "recTiles/Tiles_%02d.png", mFrameIndex);
    show_frame(img, filename);
  }
  mutex.unlock();
}

void VideoDecoder::showSlices(bool flag)
{
  mShowSlices = flag;

  mutex.lock();
  if (img != NULL)
  {
    create_directory("recSlices");
    char filename[20];
    sprintf(filename, "recSlices/Slices_%02d.png", mFrameIndex);
    show_frame(img, filename);
  }
  mutex.unlock();
}

void VideoDecoder::init_decoder(const char *filename)
{
  mFH = fopen(filename, "rb");
  // init_file_context(&inputctx, filename);
  // rbsp_buffer_init(&buf);

  ctx = de265_new_decoder();
  de265_start_worker_threads(ctx, 4); // start 4 background threads
}

void VideoDecoder::free_decoder()
{
  if (mFH)
  {
    fclose(mFH);
  }

  if (ctx)
  {
    de265_free_decoder(ctx);
  }
}
