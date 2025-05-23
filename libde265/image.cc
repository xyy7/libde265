/*
 * H.265 video codec.
 * Copyright (c) 2013-2014 struktur AG, Dirk Farin <farin@struktur.de>
 *
 * This file is part of libde265.
 *
 * libde265 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libde265 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libde265.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "image.h"
#include "decctx.h"
#include "en265.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <limits>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_SSE4_1
// SSE code processes 128bit per iteration and thus might read more data
// than is later actually used.
#define MEMORY_PADDING 16
#else
#define MEMORY_PADDING 0
#endif

#define STANDARD_ALIGNMENT 16

#ifdef HAVE___MINGW_ALIGNED_MALLOC
#define ALLOC_ALIGNED(alignment, size) __mingw_aligned_malloc((size), (alignment))
#define FREE_ALIGNED(mem) __mingw_aligned_free((mem))
#elif _WIN32
#define ALLOC_ALIGNED(alignment, size) _aligned_malloc((size), (alignment))
#define FREE_ALIGNED(mem) _aligned_free((mem))
#elif defined(HAVE_POSIX_MEMALIGN)
static inline void *ALLOC_ALIGNED(size_t alignment, size_t size)
{
  void *mem = NULL;
  if (posix_memalign(&mem, alignment, size) != 0)
  {
    return NULL;
  }
  return mem;
};
#define FREE_ALIGNED(mem) free((mem))
#else
#define ALLOC_ALIGNED(alignment, size) memalign((alignment), (size))
#define FREE_ALIGNED(mem) free((mem))
#endif

#define ALLOC_ALIGNED_16(size) ALLOC_ALIGNED(16, size)

static const int alignment = 16;

LIBDE265_API void *de265_alloc_image_plane(struct de265_image *img, int cIdx,
                                           void *inputdata, int inputstride, void *userdata)
{
  int alignment = STANDARD_ALIGNMENT;
  int stride = (img->get_width(cIdx) + alignment - 1) / alignment * alignment;
  int height = img->get_height(cIdx);

  uint8_t *p = (uint8_t *)ALLOC_ALIGNED_16(stride * height + MEMORY_PADDING);

  if (p == NULL)
  {
    return NULL;
  }

  img->set_image_plane(cIdx, p, stride, userdata);

  // copy input data if provided

  if (inputdata != NULL)
  {
    if (inputstride == stride)
    {
      memcpy(p, inputdata, stride * height);
    }
    else
    {
      for (int y = 0; y < height; y++)
      {
        memcpy(p + y * stride, ((char *)inputdata) + inputstride * y, inputstride);
      }
    }
  }

  return p;
}

LIBDE265_API void de265_free_image_plane(struct de265_image *img, int cIdx)
{
  uint8_t *p = (uint8_t *)img->get_image_plane(cIdx);
  assert(p);
  FREE_ALIGNED(p);
}

static int de265_image_get_buffer(de265_decoder_context *ctx,
                                  de265_image_spec *spec, de265_image *img, void *userdata)
{
  const int rawChromaWidth = spec->width / img->SubWidthC;
  const int rawChromaHeight = spec->height / img->SubHeightC;

  int luma_stride = (spec->width + spec->alignment - 1) / spec->alignment * spec->alignment;
  int chroma_stride = (rawChromaWidth + spec->alignment - 1) / spec->alignment * spec->alignment;
  // printf("spec->alignment:%d\n",spec->alignment); // 16

  assert(img->BitDepth_Y >= 8 && img->BitDepth_Y <= 16);
  assert(img->BitDepth_C >= 8 && img->BitDepth_C <= 16);

  int luma_bpl = luma_stride * ((img->BitDepth_Y + 7) / 8); // 如果大于8bit，那么直接用2Byte来进行表示
  int chroma_bpl = chroma_stride * ((img->BitDepth_C + 7) / 8);  //一般来说，没有使用到超过8bit的需求，因此luma_bpl=luma_stride

  int luma_height = spec->height;
  int chroma_height = rawChromaHeight;

  bool alloc_failed = false;
  std::vector<int32_t> residuals[3];
  std::vector<uint8_t> predictions[3];

  uint8_t *p[3] = {0, 0, 0};
  //ALLOC_ALIGNED_16: memalign((alignment), (size))
  p[0] = (uint8_t *)ALLOC_ALIGNED_16(luma_height * luma_bpl + MEMORY_PADDING); //没有使用memory padding

  // printf("residuals predictions Y:%d*%d,UV:%d*%d, lumastride:%d,luma_bitDepth:%d\n", luma_height, luma_bpl, chroma_height, chroma_bpl,luma_stride,img->BitDepth_Y);
  residuals[0] = std::vector<int32_t>(luma_height * luma_bpl);
  predictions[0] = std::vector<uint8_t>(luma_height * luma_bpl);


  if (p[0] == NULL)
  {
    alloc_failed = true;
  }

  if (img->get_chroma_format() != de265_chroma_mono)
  {
    p[1] = (uint8_t *)ALLOC_ALIGNED_16(chroma_height * chroma_bpl + MEMORY_PADDING);
    residuals[1] = std::vector<int32_t>(chroma_height * chroma_bpl);
    predictions[1] = std::vector<uint8_t>(chroma_height * chroma_bpl);
    p[2] = (uint8_t *)ALLOC_ALIGNED_16(chroma_height * chroma_bpl + MEMORY_PADDING);
    residuals[2] = std::vector<int32_t>(chroma_height * chroma_bpl);
    predictions[2] = std::vector<uint8_t>(chroma_height * chroma_bpl);

    if (p[1] == NULL || p[2] == NULL)
    {
      alloc_failed = true;
    }
  }
  else
  {
    p[1] = NULL;
    p[2] = NULL;
    chroma_stride = 0;
  }

  if (alloc_failed)
  {
    for (int i = 0; i < 3; i++)
      if (p[i])
      {
        FREE_ALIGNED(p[i]);
      }

    return 0;
  }
  
  // printf("de265_image_get_buffer:aligment %d, luma stride %d, lumah, bpl %d %d, chroma hw %d %d\n",spec->alignment, luma_stride,luma_height, luma_bpl, chroma_height, chroma_bpl);
  // printf("luma_stride, chroma_stride: %d,%d", luma_stride,chroma_stride);
  img->set_image_plane(0, p[0], luma_stride, NULL);
  img->set_image_plane(1, p[1], chroma_stride, NULL);
  img->set_image_plane(2, p[2], chroma_stride, NULL);
  
  img->set_residuals(residuals); // 效率确实低下，因为不是new出来的，实际上应该直接传递每个plane的size就行
  img->set_predictions(predictions);

  return 1;
}

static void de265_image_release_buffer(de265_decoder_context *ctx,
                                       de265_image *img, void *userdata)
{
  for (int i = 0; i < 3; i++)
  {
    uint8_t *p = (uint8_t *)img->get_image_plane(i);
    if (p)
    {
      FREE_ALIGNED(p);
    }
  }
}

de265_image_allocation de265_image::default_image_allocation = {
    de265_image_get_buffer,
    de265_image_release_buffer};

void de265_image::set_image_plane(int cIdx, uint8_t *mem, int stride, void *userdata)
{
  pixels[cIdx] = mem;
  plane_user_data[cIdx] = userdata;

  if (cIdx == 0)
  {
    this->stride = stride;
  }
  else
  {
    this->chroma_stride = stride;
  }
}

uint32_t de265_image::s_next_image_ID = 0;

de265_image::de265_image()
{
  ID = -1;
  removed_at_picture_id = 0; // picture not used, so we can assume it has been removed

  decctx = NULL;
  // encctx = NULL;

  // encoder_image_release_func = NULL;

  // alloc_functions.get_buffer = NULL;
  // alloc_functions.release_buffer = NULL;

  for (int c = 0; c < 3; c++)
  {
    pixels[c] = NULL;
    pixels_confwin[c] = NULL;
    plane_user_data[c] = NULL;
  }

  width = height = 0;

  pts = 0;
  user_data = NULL;

  ctb_progress = NULL;

  integrity = INTEGRITY_NOT_DECODED;

  picture_order_cnt_lsb = -1; // undefined
  PicOrderCntVal = -1;        // undefined
  PicState = UnusedForReference;
  PicOutputFlag = false;

  nThreadsQueued = 0;
  nThreadsRunning = 0;
  nThreadsBlocked = 0;
  nThreadsFinished = 0;
  nThreadsTotal = 0;

  de265_mutex_init(&mutex);
  de265_cond_init(&finished_cond);
}

de265_error de265_image::alloc_image(int w, int h, enum de265_chroma c,
                                     std::shared_ptr<const seq_parameter_set> sps, bool allocMetadata,
                                     decoder_context *dctx,
                                     // encoder_context* ectx,
                                     de265_PTS pts, void *user_data,
                                     bool useCustomAllocFunc)
{
  // if (allocMetadata) { assert(sps); }
  if (allocMetadata)
  {
    assert(sps);
  }

  if (sps)
  {
    this->sps = sps;
  }

  release(); /* TODO: review code for efficient allocation when arrays are already
                allocated to the requested size. Without the release, the old image-data
                will not be freed. */

  ID = s_next_image_ID++;
  removed_at_picture_id = std::numeric_limits<int32_t>::max();

  decctx = dctx;
  // encctx = ectx;

  // --- allocate image buffer ---

  chroma_format = c;

  width = w;
  height = h;
  chroma_width = w;
  chroma_height = h;

  this->user_data = user_data;
  this->pts = pts;

  de265_image_spec spec;

  int WinUnitX, WinUnitY;

  switch (chroma_format)
  {
  case de265_chroma_mono:
    WinUnitX = 1;
    WinUnitY = 1;
    break;
  case de265_chroma_420:
    WinUnitX = 2;
    WinUnitY = 2;
    break;
  case de265_chroma_422:
    WinUnitX = 2;
    WinUnitY = 1;
    break;
  case de265_chroma_444:
    WinUnitX = 1;
    WinUnitY = 1;
    break;
  default:
    assert(0);
  }

  switch (chroma_format)
  {
  case de265_chroma_420:
    spec.format = de265_image_format_YUV420P8;
    chroma_width = (chroma_width + 1) / 2;
    chroma_height = (chroma_height + 1) / 2;
    SubWidthC = 2;
    SubHeightC = 2;
    break;

  case de265_chroma_422:
    spec.format = de265_image_format_YUV422P8;
    chroma_width = (chroma_width + 1) / 2;
    SubWidthC = 2;
    SubHeightC = 1;
    break;

  case de265_chroma_444:
    spec.format = de265_image_format_YUV444P8;
    SubWidthC = 1;
    SubHeightC = 1;
    break;

  case de265_chroma_mono:
    spec.format = de265_image_format_mono8;
    chroma_width = 0;
    chroma_height = 0;
    SubWidthC = 1;
    SubHeightC = 1;
    break;

  default:
    assert(false);
    break;
  }

  if (chroma_format != de265_chroma_mono && sps)
  {
    assert(sps->SubWidthC == SubWidthC);
    assert(sps->SubHeightC == SubHeightC);
  }

  spec.width = w;
  spec.height = h;
  spec.alignment = STANDARD_ALIGNMENT;

  // conformance window cropping

  int left = sps ? sps->conf_win_left_offset : 0;
  int right = sps ? sps->conf_win_right_offset : 0;
  int top = sps ? sps->conf_win_top_offset : 0;
  int bottom = sps ? sps->conf_win_bottom_offset : 0;

  if ((left + right) * WinUnitX >= width)
  {
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }

  if ((top + bottom) * WinUnitY >= height)
  {
    return DE265_ERROR_CODED_PARAMETER_OUT_OF_RANGE;
  }

  width_confwin = width - (left + right) * WinUnitX;
  height_confwin = height - (top + bottom) * WinUnitY;
  chroma_width_confwin = chroma_width - left - right;
  chroma_height_confwin = chroma_height - top - bottom;
  // printf("logging: alloc_image: real_w:%d,real_h:%d,real_cw:%d,real_ch:%d,pad_w:%d,pad_h:%d,stride:%d,cstride:%d,l:%d,r:%d,t:%d,b:%d\n", width_confwin,height_confwin,chroma_width_confwin,chroma_height_confwin,width,height, stride, chroma_stride,left, right,top,bottom);

  spec.crop_left = left * WinUnitX;
  spec.crop_right = right * WinUnitX;
  spec.crop_top = top * WinUnitY;
  spec.crop_bottom = bottom * WinUnitY;

  spec.visible_width = width_confwin;
  spec.visible_height = height_confwin;

  BitDepth_Y = (sps == NULL) ? 8 : sps->BitDepth_Y;
  BitDepth_C = (sps == NULL) ? 8 : sps->BitDepth_C;

  bpp_shift[0] = (BitDepth_Y <= 8) ? 0 : 1;
  bpp_shift[1] = (BitDepth_C <= 8) ? 0 : 1;
  bpp_shift[2] = bpp_shift[1];

  // allocate memory and set conformance window pointers

  void *alloc_userdata = NULL;
  if (decctx)
    alloc_userdata = decctx->param_image_allocation_userdata;
  // if (encctx) alloc_userdata = encctx->param_image_allocation_userdata; // actually not needed

  /*
  if (encctx && useCustomAllocFunc) {
    encoder_image_release_func = encctx->release_func;

    // if we do not provide a release function, use our own

    if (encoder_image_release_func == NULL) {
      image_allocation_functions = de265_image::default_image_allocation;
    }
    else {
      image_allocation_functions.get_buffer     = NULL;
      image_allocation_functions.release_buffer = NULL;
    }
  }
  else*/
  if (decctx && useCustomAllocFunc)
  {
    image_allocation_functions = decctx->param_image_allocation_functions;
  }
  else
  {
    image_allocation_functions = de265_image::default_image_allocation;
  }

  bool mem_alloc_success = true;

  if (image_allocation_functions.get_buffer != NULL)
  {
    mem_alloc_success = image_allocation_functions.get_buffer(decctx, &spec, this,
                                                              alloc_userdata);

    pixels_confwin[0] = pixels[0] + left * WinUnitX + top * WinUnitY * stride;

    if (chroma_format != de265_chroma_mono)
    {
      pixels_confwin[1] = pixels[1] + left + top * chroma_stride;
      pixels_confwin[2] = pixels[2] + left + top * chroma_stride;
    }
    else
    {
      pixels_confwin[1] = NULL;
      pixels_confwin[2] = NULL;
    }

    // check for memory shortage

    if (!mem_alloc_success)
    {
      return DE265_ERROR_OUT_OF_MEMORY;
    }
  }

  // alloc_functions = *allocfunc;
  // alloc_userdata  = userdata;

  // --- allocate decoding info arrays ---

  if (allocMetadata)
  {
    // intra pred mode

    mem_alloc_success &= intraPredMode.alloc(sps->PicWidthInMinPUs, sps->PicHeightInMinPUs,
                                             sps->Log2MinPUSize);

    mem_alloc_success &= intraPredModeC.alloc(sps->PicWidthInMinPUs, sps->PicHeightInMinPUs,
                                              sps->Log2MinPUSize);

    // cb info

    mem_alloc_success &= cb_info.alloc(sps->PicWidthInMinCbsY, sps->PicHeightInMinCbsY,
                                       sps->Log2MinCbSizeY);

    // pb info

    int puWidth = sps->PicWidthInMinCbsY << (sps->Log2MinCbSizeY - 2);
    int puHeight = sps->PicHeightInMinCbsY << (sps->Log2MinCbSizeY - 2);

    mem_alloc_success &= pb_info.alloc(puWidth, puHeight, 2);

    // tu info

    mem_alloc_success &= tu_info.alloc(sps->PicWidthInTbsY, sps->PicHeightInTbsY,
                                       sps->Log2MinTrafoSize);

    // deblk info

    int deblk_w = (sps->pic_width_in_luma_samples + 3) / 4; //向上取整，去方块滤波信息
    int deblk_h = (sps->pic_height_in_luma_samples + 3) / 4;

    mem_alloc_success &= deblk_info.alloc(deblk_w, deblk_h, 2);

    // CTB info

    if (ctb_info.width_in_units != sps->PicWidthInCtbsY ||
        ctb_info.height_in_units != sps->PicHeightInCtbsY)
    {
      delete[] ctb_progress;

      mem_alloc_success &= ctb_info.alloc(sps->PicWidthInCtbsY, sps->PicHeightInCtbsY,
                                          sps->Log2CtbSizeY);

      ctb_progress = new de265_progress_lock[ctb_info.data_size];
    }

    // check for memory shortage

    if (!mem_alloc_success)
    {
      return DE265_ERROR_OUT_OF_MEMORY;
    }
  }

  return DE265_OK;
}

de265_image::~de265_image()
{
  // printf("release this %p\n",this);
  release();

  // free progress locks
  if (ctb_progress)
  {
    delete[] ctb_progress;
  }

  de265_cond_destroy(&finished_cond);
  de265_mutex_destroy(&mutex);
}

void de265_image::release()
{
  // free image memory

  if (pixels[0])
  {
    /*
    if (encoder_image_release_func != NULL) {
      encoder_image_release_func(encctx, this,
                                 encctx->param_image_allocation_userdata);
    }
    else*/
    // printf("free pixels memory release_buffer %p %p\n",decctx, image_allocation_functions.release_buffer);
    {
      if(image_allocation_functions.release_buffer !=nullptr){
          image_allocation_functions.release_buffer(decctx, this,
                                                decctx ? decctx->param_image_allocation_userdata : NULL);
      }
      
    }

    for (int i = 0; i < 3; i++)
    {
      pixels[i] = NULL;
      pixels_confwin[i] = NULL;
    }
  }

  // free slices
  for (size_t i = 0; i < slices.size(); i++)
  {
    delete slices[i];
  }
  slices.clear();
}

void de265_image::fill_image(int y, int cb, int cr)
{
  if (y >= 0)
  {
    memset(pixels[0], y, stride * height);
  }

  if (cb >= 0)
  {
    memset(pixels[1], cb, chroma_stride * chroma_height);
  }

  if (cr >= 0)
  {
    memset(pixels[2], cr, chroma_stride * chroma_height);
  }
}

de265_error de265_image::copy_image(const de265_image *src)
{
  /* TODO: actually, since we allocate the image only for internal purpose, we
     do not have to call the external allocation routines for this. However, then
     we have to track for each image how to release it again.
     Another option would be to safe the copied data not in an de265_image at all.
  */

  de265_error err = alloc_image(src->width, src->height, src->chroma_format, src->sps, false,
                                src->decctx, /*src->encctx,*/ src->pts, src->user_data, false);
  if (err != DE265_OK)
  {
    return err;
  }

  copy_lines_from(src, 0, src->height);

  return err;
}

// end = last line + 1
void de265_image::copy_lines_from(const de265_image *src, int first, int end)
{
  if (end > src->height)
    end = src->height;

  assert(first % 2 == 0);
  assert(end % 2 == 0);

  int luma_bpp = (sps->BitDepth_Y + 7) / 8;
  int chroma_bpp = (sps->BitDepth_C + 7) / 8;

  if (src->stride == stride)
  {
    memcpy(pixels[0] + first * stride * luma_bpp,
           src->pixels[0] + first * src->stride * luma_bpp,
           (end - first) * stride * luma_bpp);
  }
  else
  {
    for (int yp = first; yp < end; yp++)
    {
      memcpy(pixels[0] + yp * stride * luma_bpp,
             src->pixels[0] + yp * src->stride * luma_bpp,
             src->width * luma_bpp);
    }
  }

  int first_chroma = first / src->SubHeightC;
  int end_chroma = end / src->SubHeightC;

  if (src->chroma_format != de265_chroma_mono)
  {
    if (src->chroma_stride == chroma_stride)
    {
      memcpy(pixels[1] + first_chroma * chroma_stride * chroma_bpp,
             src->pixels[1] + first_chroma * chroma_stride * chroma_bpp,
             (end_chroma - first_chroma) * chroma_stride * chroma_bpp);
      memcpy(pixels[2] + first_chroma * chroma_stride * chroma_bpp,
             src->pixels[2] + first_chroma * chroma_stride * chroma_bpp,
             (end_chroma - first_chroma) * chroma_stride * chroma_bpp);
    }
    else
    {
      for (int y = first_chroma; y < end_chroma; y++)
      {
        memcpy(pixels[1] + y * chroma_stride * chroma_bpp,
               src->pixels[1] + y * src->chroma_stride * chroma_bpp,
               src->chroma_width * chroma_bpp);
        memcpy(pixels[2] + y * chroma_stride * chroma_bpp,
               src->pixels[2] + y * src->chroma_stride * chroma_bpp,
               src->chroma_width * chroma_bpp);
      }
    }
  }
}

void de265_image::exchange_pixel_data_with(de265_image &b)
{
  for (int i = 0; i < 3; i++)
  {
    std::swap(pixels[i], b.pixels[i]);
    std::swap(pixels_confwin[i], b.pixels_confwin[i]);
    std::swap(plane_user_data[i], b.plane_user_data[i]);
  }

  std::swap(stride, b.stride);
  std::swap(chroma_stride, b.chroma_stride);
  std::swap(image_allocation_functions, b.image_allocation_functions);
}

void de265_image::thread_start(int nThreads)
{
  de265_mutex_lock(&mutex);

  // printf("nThreads before: %d %d\n",nThreadsQueued, nThreadsTotal);

  nThreadsQueued += nThreads;
  nThreadsTotal += nThreads;

  // printf("nThreads after: %d %d\n",nThreadsQueued, nThreadsTotal);

  de265_mutex_unlock(&mutex);
}

void de265_image::thread_run(const thread_task *task)
{
  // printf("run thread %s\n", task->name().c_str());

  de265_mutex_lock(&mutex);
  nThreadsQueued--;
  nThreadsRunning++;
  de265_mutex_unlock(&mutex);
}

void de265_image::thread_blocks()
{
  de265_mutex_lock(&mutex);
  nThreadsRunning--;
  nThreadsBlocked++;
  de265_mutex_unlock(&mutex);
}

void de265_image::thread_unblocks()
{
  de265_mutex_lock(&mutex);
  nThreadsBlocked--;
  nThreadsRunning++;
  de265_mutex_unlock(&mutex);
}

void de265_image::thread_finishes(const thread_task *task)
{
  // printf("finish thread %s\n", task->name().c_str());

  de265_mutex_lock(&mutex);

  nThreadsRunning--;
  nThreadsFinished++;
  assert(nThreadsRunning >= 0);

  if (nThreadsFinished == nThreadsTotal)
  {
    de265_cond_broadcast(&finished_cond, &mutex);
  }

  de265_mutex_unlock(&mutex);
}

void de265_image::wait_for_progress(thread_task *task, int ctbx, int ctby, int progress)
{
  const int ctbW = sps->PicWidthInCtbsY;

  wait_for_progress(task, ctbx + ctbW * ctby, progress);
}

void de265_image::wait_for_progress(thread_task *task, int ctbAddrRS, int progress)
{
  if (task == NULL)
  {
    return;
  }

  de265_progress_lock *progresslock = &ctb_progress[ctbAddrRS];
  if (progresslock->get_progress() < progress)
  {
    thread_blocks();

    assert(task != NULL);
    task->state = thread_task::Blocked;

    /* TODO: check whether we are the first blocked task in the list.
       If we are, we have to conceal input errors.
       Simplest concealment: do not block.
    */

    progresslock->wait_for_progress(progress);
    task->state = thread_task::Running;
    thread_unblocks();
  }
}

void de265_image::wait_for_completion()
{
  de265_mutex_lock(&mutex);
  while (nThreadsFinished != nThreadsTotal)
  {
    de265_cond_wait(&finished_cond, &mutex);
  }
  de265_mutex_unlock(&mutex);
}

bool de265_image::debug_is_completed() const
{
  return nThreadsFinished == nThreadsTotal;
}

void de265_image::clear_metadata()
{
  // TODO: maybe we could avoid the memset by ensuring that all data is written to
  // during decoding (especially log2CbSize), but it is unlikely to be faster than the memset.

  cb_info.clear();
  // tu_info.clear();  // done on the fly
  ctb_info.clear();
  deblk_info.clear();

  // --- reset CTB progresses ---

  for (int i = 0; i < ctb_info.data_size; i++)
  {
    ctb_progress[i].reset(CTB_PROGRESS_NONE);
  }
}

void de265_image::get_all_metadata()
{
  {
    printf("nThreadsQueued:%d,nThreadsRunning:%d,nThreadsBlocked:%d,nThreadsFinished:%d,nThreadsTotal:%d\n", nThreadsQueued, nThreadsRunning, nThreadsBlocked, nThreadsFinished, nThreadsTotal);
    printf("BitDepth_Y:%d,BitDepth_C:%d,SubWidthC:%d,SubHeightC:%d\n", BitDepth_Y, BitDepth_C, SubWidthC, SubHeightC);
    printf("============print ctb info============\n");
    ctb_info.printMyself();
    printf("============print ctb info============\n");

    printf("============print pb_info info============\n");
    pb_info.printMyself();
    printf("============print pb_info info============\n");

    printf("============print cb_info info============\n");
    cb_info.printMyself();
    printf("============print cb_info info============\n");

    printf("============print intraPredMode info============\n");
    intraPredMode.printMyself();
    printf("============print intraPredMode info============\n");

    printf("============print intraPredModeC info============\n");
    intraPredModeC.printMyself();
    printf("============print intraPredModeC info============\n");

    printf("============print tu_info info============\n");
    tu_info.printMyself();
    printf("============print tu_info info============\n");

    printf("============print deblk_info info============\n");
    deblk_info.printMyself();
    printf("============print deblk_info info============\n");
  }
}

void de265_image::set_mv_info(int x, int y, int nPbW, int nPbH, const PBMotion &mv)
{
  int log2PuSize = 2;

  int xPu = x >> log2PuSize;
  int yPu = y >> log2PuSize;
  int wPu = nPbW >> log2PuSize;
  int hPu = nPbH >> log2PuSize;

  int stride = pb_info.width_in_units;

  for (int pby = 0; pby < hPu; pby++)
    for (int pbx = 0; pbx < wPu; pbx++)
    {
      pb_info[xPu + pbx + (yPu + pby) * stride] = mv;
    }
}

void de265_image::convert_mv_info(){
  const seq_parameter_set &sps = this->get_sps();

  int minCbSize = sps.MinCbSizeY;  //8
  const int W = this->get_width();
  const int H = this->get_height();
  mv_b.resize(H, std::vector<std::array<int, 3>>(W));
  mv_f.resize(H, std::vector<std::array<int, 3>>(W));

  for (int y0=0;y0<sps.PicHeightInMinCbsY;y0++)  //30*8 240 
    for (int x0=0;x0<sps.PicWidthInMinCbsY;x0++) //40*8 320
      {
        int log2CbSize = this->get_log2CbSize_cbUnits(x0,y0);
        if (log2CbSize==0) {
          continue;
        }
        //xb,yb,左上角位置，不同的partmode，同一个宏块里面，左上角位置也不同
        int xb = x0*minCbSize; 
        int yb = y0*minCbSize;

        int CbSize = 1<<log2CbSize;

        enum DrawModeRepeat what = PBMotionVectorsRepeat;
        enum PartMode partMode = this->get_PartMode(xb, yb);

        int HalfCbSize = (1<<(log2CbSize-1));

        switch (partMode) {
        case PART_2Nx2N:
        PB_repeat(xb,yb,CbSize,CbSize, what);
        break;
        case PART_NxN:
        PB_repeat(xb,           yb,           CbSize/2,CbSize/2, what);
        PB_repeat(xb+HalfCbSize,yb,           CbSize/2,CbSize/2, what);
        PB_repeat(xb           ,yb+HalfCbSize,CbSize/2,CbSize/2, what);
        PB_repeat(xb+HalfCbSize,yb+HalfCbSize,CbSize/2,CbSize/2, what);
        break;
        case PART_2NxN:
        PB_repeat(xb,           yb,           CbSize  ,CbSize/2, what);
        PB_repeat(xb,           yb+HalfCbSize,CbSize  ,CbSize/2, what);
        break;
        case PART_Nx2N:
        PB_repeat(xb,           yb,           CbSize/2,CbSize, what);
        PB_repeat(xb+HalfCbSize,yb,           CbSize/2,CbSize, what);
        break;
        case PART_2NxnU:
        PB_repeat(xb,           yb,           CbSize  ,CbSize/4,   what);
        PB_repeat(xb,           yb+CbSize/4  ,CbSize  ,CbSize*3/4, what);
        break;
        case PART_2NxnD:
        PB_repeat(xb,           yb,           CbSize  ,CbSize*3/4, what);
        PB_repeat(xb,           yb+CbSize*3/4,CbSize  ,CbSize/4,   what);
        break;
        case PART_nLx2N:
        PB_repeat(xb,           yb,           CbSize/4  ,CbSize, what);
        PB_repeat(xb+CbSize/4  ,yb,           CbSize*3/4,CbSize, what);
        break;
        case PART_nRx2N:
        PB_repeat(xb,           yb,           CbSize*3/4,CbSize, what);
        PB_repeat(xb+CbSize*3/4,yb,           CbSize/4  ,CbSize, what);
        break;
        default:
        assert(false);
        break;
        }
    }
}

template< typename T> //pixels是WxH处于同一个平面,每个通道一维数组
void de265_image::crop1D(std::vector<T> &vec, int left, int top, int pad_stride, int real_height, int real_width){
  // 去除左上角
  vec.erase(vec.begin(),vec.begin()+left + top * pad_stride);
  // 一行一行将vec复制到tmp
  vector<T> tmp;
  // tmp.reserve(real_width*real_height);
  // printf("crop1D:init %d",tmp.size());
  for (int y = 0; y < real_height; y++)
  {
    // copy(vec.begin()+y * pad_stride, vec.begin()+y * pad_stride+real_width, tmp.begin()+y*real_width); //reserve似乎复制不过来
    tmp.insert(tmp.end(), vec.begin()+y * pad_stride, vec.begin()+y * pad_stride+real_width);
  }
  // printf("crop1D: result:%d,WxH:%d\n", tmp.size(), real_height * real_width);
  // 恢复原来的大小
  vec.swap(tmp);
}

template< typename T> //WxH,直接是二维数组
void de265_image::crop2D(std::vector<std::vector<T>>& vec, int left, int top, int real_width, int real_height){
  vec.erase(vec.begin(),vec.begin()+left); // 裁剪左边
  vec.erase(vec.begin()+real_width,vec.end()); //裁剪右边
  for (int i = 0; i < vec.size();++i)
  {
    vec[i].erase(vec[i].begin(), vec[i].begin() + top);  //裁剪上边
    vec[i].erase(vec[i].begin()+real_height, vec[i].end()); //裁剪下边
  }
  // printf("crop2D:W=%lu,H=%lu\n", vec.size(), vec[0].size());
}

#pragma optimize( "", off )
void de265_image::convert_info(){
  //copy from alloc_image:
  int WinUnitX, WinUnitY;

  switch (chroma_format)
  {
  case de265_chroma_mono:
    WinUnitX = 1;
    WinUnitY = 1;
    break;
  case de265_chroma_420:
    WinUnitX = 2;
    WinUnitY = 2;
    break;
  case de265_chroma_422:
    WinUnitX = 2;
    WinUnitY = 1;
    break;
  case de265_chroma_444:
    WinUnitX = 1;
    WinUnitY = 1;
    break;
  default:
    assert(0);
  }

  int left = sps ? sps->conf_win_left_offset : 0;
  int right = sps ? sps->conf_win_right_offset : 0;
  int top = sps ? sps->conf_win_top_offset : 0;
  int bottom = sps ? sps->conf_win_bottom_offset : 0;
  // printf("logging: convert_info: real_w:%d,real_h:%d,real_cw:%d,real_ch:%d,pad_w:%d,pad_h:%d,stride:%d,cstride:%d,l:%d,r:%d,t:%d,b:%d\n", width_confwin,height_confwin,chroma_width_confwin,chroma_height_confwin,width,height, stride, chroma_stride,left, right,top,bottom);

  for (int cIdx = 0; cIdx < 3; cIdx++)
  {
    for (int y = 0; y < get_height(cIdx); y++)
    {
      for (int x = 0; x < get_width(cIdx); x++)
      {
        int stride = get_image_stride(cIdx);
        int pos = y * stride + x;
        residuals[cIdx][pos] = pixels[cIdx][pos]-predictions[cIdx][pos];
      }
    }
    if (get_chroma_format() == de265_chroma_mono){
      break;
    }
  }

  // residuals/predictions是在pixels进行处理(pred和resi)转换的时候保存的,因此是跟pixels相同大小,需要处理成跟实际大小
  crop1D(residuals[0], left * WinUnitX, top * WinUnitY, stride, height_confwin, width_confwin); // WinUnitX,WinUnitY是chroma下采样倍率
  if (chroma_format != de265_chroma_mono)
  {
    crop1D(residuals[1], left, top, chroma_stride, chroma_height_confwin, chroma_width_confwin);
    crop1D(residuals[2], left, top, chroma_stride, chroma_height_confwin, chroma_width_confwin);
  }
  crop1D(predictions[0], left*WinUnitX, top*WinUnitY, stride, height_confwin, width_confwin);
  if (chroma_format != de265_chroma_mono)
  {
    crop1D(predictions[1], left, top, chroma_stride, chroma_height_confwin, chroma_width_confwin);
    crop1D(predictions[2], left, top, chroma_stride, chroma_height_confwin, chroma_width_confwin);
  }


  enum DrawModeRepeat what = PBMotionVectorsRepeat;
  const seq_parameter_set &sps = this->get_sps();

  int minCbSize = sps.MinCbSizeY;  //8
  const int W = this->get_width();  //返回的只是pixels的pad完的宽和高
  const int H = this->get_height();
  
  // 将pb_info转换repeat成vector+图像的形式，以方便后续处理
  // MetaDataArray<PBMotion> pb_info;        //运动向量/帧间预测模式需要保存
  // std::vector<std::vector<std::array<int, 3>>> mv_f;
  // std::vector<std::vector<std::array<int, 3>>> mv_b;
  // printf("mv:%dx%d, %dx%d,minCbSize:%d\n", W, H,sps.PicWidthInMinCbsY,sps.PicHeightInMinCbsY,minCbSize); //mv:384x288, 24x18,minCbSize:16
  mv_b.resize(W, std::vector<std::array<int, 3>>(H));
  mv_f.resize(W, std::vector<std::array<int, 3>>(H));
  quantPYs.resize(W,std::vector<int>(H));
  // printf("cb_info: ");
  // cb_info.printMyself(); //cb_info: datasize:432, log2unitSize:4, width_in_units:24, height_in_units:18
  // printf("pb_info: ");
  // pb_info.printMyself(); //pb_info: datasize:6912, log2unitSize:2 width_in_units:96, height_in_units:72

  for (int y0=0;y0<sps.PicHeightInMinCbsY;y0++)  //30*8 240  visualize.cc draw_tree_grid
    for (int x0=0;x0<sps.PicWidthInMinCbsY;x0++) //40*8 320
      {
        int log2CbSize = this->get_log2CbSize_cbUnits(x0,y0);
        if (log2CbSize==0) {
          continue;
        }
        //xb,yb,左上角位置，不同的partmode，同一个宏块里面，左上角位置也不同
        int xb = x0*minCbSize; 
        int yb = y0*minCbSize;

        int CbSize = 1<<log2CbSize;
        PB_repeat(xb, yb, CbSize, CbSize, QuantP_YRepeat); // 量化默认是2Nx2N visualize.cc
        enum PartMode partMode = this->get_PartMode(xb, yb);
        int HalfCbSize = (1<<(log2CbSize-1));
        switch (partMode) {
          case PART_2Nx2N:
          PB_repeat(xb,yb,CbSize,CbSize, what);
          break;
          case PART_NxN:
          PB_repeat(xb,           yb,           CbSize/2,CbSize/2, what);
          PB_repeat(xb+HalfCbSize,yb,           CbSize/2,CbSize/2, what);
          PB_repeat(xb           ,yb+HalfCbSize,CbSize/2,CbSize/2, what);
          PB_repeat(xb+HalfCbSize,yb+HalfCbSize,CbSize/2,CbSize/2, what);
          break;
          case PART_2NxN:
          PB_repeat(xb,           yb,           CbSize  ,CbSize/2, what);
          PB_repeat(xb,           yb+HalfCbSize,CbSize  ,CbSize/2, what);
          break;
          case PART_Nx2N:
          PB_repeat(xb,           yb,           CbSize/2,CbSize, what);
          PB_repeat(xb+HalfCbSize,yb,           CbSize/2,CbSize, what);
          break;
          case PART_2NxnU:
          PB_repeat(xb,           yb,           CbSize  ,CbSize/4,   what);
          PB_repeat(xb,           yb+CbSize/4  ,CbSize  ,CbSize*3/4, what);
          break;
          case PART_2NxnD:
          PB_repeat(xb,           yb,           CbSize  ,CbSize*3/4, what);
          PB_repeat(xb,           yb+CbSize*3/4,CbSize  ,CbSize/4,   what);
          break;
          case PART_nLx2N:
          PB_repeat(xb,           yb,           CbSize/4  ,CbSize, what);
          PB_repeat(xb+CbSize/4  ,yb,           CbSize*3/4,CbSize, what);
          break;
          case PART_nRx2N:
          PB_repeat(xb,           yb,           CbSize*3/4,CbSize, what);
          PB_repeat(xb+CbSize*3/4,yb,           CbSize/4  ,CbSize, what);
          break;
          default:
          assert(false);
          break;
        }
    }
  crop2D(mv_b, left, top, width_confwin, height_confwin);
  crop2D(mv_f, left, top, width_confwin, height_confwin);
  crop2D(quantPYs, left, top,  width_confwin, height_confwin);
}
#pragma optimize("", on )


void de265_image::PB_repeat(int x0,int y0, int w,int h, enum DrawModeRepeat what){
    if(what == PBMotionVectorsRepeat){
      const PBMotion& mvi = this->get_mv_info(x0,y0);
      // printf("PB_repeat...%d,%d,%d,%d\n",x0,y0,w,h);
      // printf("mvb.size():%d. mvb[0].size():%d ,%d,%d\n", mv_b.size(),mv_b[0].size(),x0+w,y0+h);
      // std::cout << mvi<<endl;
      if (mvi.predFlag[0]==1)  // 避免有的时候解码出乱码，理论上不应该解码出乱码
      {
        for (int x = x0; x < x0 + w;++x){
          for (int y = y0; y < y0 + h;++y){
            std::array<int,3> pixel_mv = { static_cast<int>(mvi.mv[0].x), static_cast<int>(mvi.mv[0].y), static_cast<int>(mvi.refIdx[0])};
            mv_b[x][y] = pixel_mv;
          }
        }
      }
      if (mvi.predFlag[1]==1) {
        for (int x = x0; x < x0 + w;++x){
          for (int y = y0; y < y0 + h;++y){
            std::array<int,3> pixel_mv = { static_cast<int>(mvi.mv[1].x), static_cast<int>(mvi.mv[1].y),static_cast<int>(mvi.refIdx[1])};
            mv_f[x][y] = pixel_mv;
          }
        }
      }
    }
    else if (what == QuantP_YRepeat)
    {
      int qp = get_QPY(x0, y0);
      for (int x = x0; x < x0 + w;++x){
          for (int y = y0; y < y0 + h;++y){
            quantPYs[x][y] = qp;
          }
        }
    }
}


bool de265_image::available_zscan(int xCurr, int yCurr, int xN, int yN) const
{
  if (xN < 0 || yN < 0)
    return false;
  if (xN >= sps->pic_width_in_luma_samples ||
      yN >= sps->pic_height_in_luma_samples)
    return false;

  int minBlockAddrN = pps->MinTbAddrZS[(xN >> sps->Log2MinTrafoSize) +
                                       (yN >> sps->Log2MinTrafoSize) * sps->PicWidthInTbsY];
  int minBlockAddrCurr = pps->MinTbAddrZS[(xCurr >> sps->Log2MinTrafoSize) +
                                          (yCurr >> sps->Log2MinTrafoSize) * sps->PicWidthInTbsY];

  if (minBlockAddrN > minBlockAddrCurr)
    return false;

  int xCurrCtb = xCurr >> sps->Log2CtbSizeY;
  int yCurrCtb = yCurr >> sps->Log2CtbSizeY;
  int xNCtb = xN >> sps->Log2CtbSizeY;
  int yNCtb = yN >> sps->Log2CtbSizeY;

  if (get_SliceAddrRS(xCurrCtb, yCurrCtb) !=
      get_SliceAddrRS(xNCtb, yNCtb))
  {
    return false;
  }

  if (pps->TileIdRS[xCurrCtb + yCurrCtb * sps->PicWidthInCtbsY] !=
      pps->TileIdRS[xNCtb + yNCtb * sps->PicWidthInCtbsY])
  {
    return false;
  }

  return true;
}

bool de265_image::available_pred_blk(int xC, int yC, int nCbS, int xP, int yP,
                                     int nPbW, int nPbH, int partIdx, int xN, int yN) const
{
  logtrace(LogMotion, "C:%d;%d P:%d;%d N:%d;%d size=%d;%d\n", xC, yC, xP, yP, xN, yN, nPbW, nPbH);

  int sameCb = (xC <= xN && xN < xC + nCbS &&
                yC <= yN && yN < yC + nCbS);

  bool availableN;

  if (!sameCb)
  {
    availableN = available_zscan(xP, yP, xN, yN);
  }
  else
  {
    availableN = !(nPbW << 1 == nCbS && nPbH << 1 == nCbS && // NxN
                   partIdx == 1 &&
                   yN >= yC + nPbH && xN < xC + nPbW); // xN/yN inside partIdx 2
  }

  if (availableN && get_pred_mode(xN, yN) == MODE_INTRA)
  {
    availableN = false;
  }

  return availableN;
}
