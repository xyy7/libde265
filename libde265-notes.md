## 码流变化：

* rgb==》yuv==》bin==》byte==》NAL ==》slice/pps/sps/vps/sei==>CTU==>PU==>TU

## 解码步骤

### dec265-main.cc(调用层次)

```
main
	fopen
	fread
	de265_push_data
	de265_decode
		ctx->decode
		ctx->decode_NAL # 也包括了VCL
			nal_hdr.read
			read_slice_NAL
				add_slice_segment_header
				decode_some
				
			read_vps_NAL
			read_sps_NAL
			read_pps_NAL
			read_sei_NAL
			
		decode_some
			decode_slice_unit_parallel
				decode_slice_unit_sequential
					read_slice_segment_data
						decode_substream
							read_coding_tree_unit
								read_coding_quadtree
									read_coding_unit
										decode_quantization_parameters
											get_QPY
											set_QPY
											
										decode_prediction_unit
										read_prediction_unit
										read_transform_tree
											read_transform_unit
											residual_coding
											decode_TU
												scale_coefficients
													scale_coefficients_internal
														add_residual
														transform_coefficients
			
			run_postprocessing_filters_xx
			process_sei
			push_picture_to_output_queue
			pop_front
				
	img = de265_get_next_picture(ctx)
	tmp = new de265_image
	*tmp = *img
	tmp->convert_info # 获取残差，以及运动向量
		residuals[cIdx][pos] = pixels[cIdx][pos]-predictions[cIdx][pos]==>crop1D
		PB_repeat==>crop2D # mv和quanty是一样的操作
```

### 1. 读取bin文件

* 解析：除了真实数据（NAL单元等），还有对齐、标志，以及类型表示，都需要进行解析。

### 2. 读取NAL单元

* **NAL（Network Abstraction Layer，网络抽象层）** 和 **VCL（Video Coding Layer，视频编码层）**

1. 读取NAL的length
2. 读取NAL==》alloc单元==》set_data==》去除填充字节==》push进NAL队列
   1. **NAL的动态池化思想**：
      1. 有free_NAL_list，如果存在，那么可以直接使用。
      2. 销毁NAL单元：如果free_NAL_list没有满，那么可以直接使用；如果满了才进行销毁。

### 3. NAL层层解析（NAL-slice-CTU-PU-TU）

1.  读取buffer字节
2.  **把buffer里面的东西，push到NAL_parser中** （有各种input_push_state来判断是否构建一个完整的NAL单元）
    1. NAL_parser属于decoder_context
3.  **decoder_context负责解码，解码完的部分存放在dpb中：【buffer\池化技术】**
    1. dpb（decoded picture buffer）：如果没有需要解码的东西了，那么将reorder_buffer里面的img全部输入到output队列中
    2. 关键就是解码NAL单元 decode_NAL：
       1. 使用bitreader读取NAL头部
       2. **当知道nal_unit_type<32的时候，读取slice**
          1. 读取slice头部
          2. **每张img可能由多个slice组成**【按照sequence可以更容易了解编码流程】
          3. **每个slice由多个CTB组成【开始进行递归解码】**
             1. 如果由SAO信息，那么也要读取SAO信息
          4. **每个CTU由多个CU组成**（需要根据flag判断是否分块）
             1. 读取量化等级，读取是否跳过量化flag
          5. **读取PU**：
             1. 帧间预测模式：读取MVP==》MV，引用帧
             2. 帧内预测模式
             3. 读取预测模式是如何分块的
          6. **读取TU（残差）**：
             1. PB是否继续拆分成TU
             2. 读取扫描顺序【zscan等】
             3. 解码DC系数和AC系数
             4. **缩放残差系数**
       3. nal_unit_type>=32的时候，读取vps、sps、pps、sei等
4.  **decoder_context从dpb的 output队列中拿到解码完的图片**【YUV格式】

### 4. de265_image数据结构

* libde265中的数据结构，按照TU、PU单元存储：

  * ```
    MetaDataArray<CTB_info> ctb_info;
    MetaDataArray<CB_ref_info> cb_info;    
    MetaDataArray<PBMotion> pb_info;        //运动向量/帧间预测模式需要保存
    MetaDataArray<uint8_t> intraPredMode;   
    MetaDataArray<uint8_t> intraPredModeC; 
    uint8_t *pixels[3];
    ```

* 新增数据结构

  * ```
    std::array<std::vector<int32_t>,3> residuals;
    std::array<std::vector<uint8_t>,3> predictions;
    std::vector<std::vector<std::array<int, 3>>> mv_f;
    std::vector<std::vector<std::array<int, 3>>> mv_b;
    ```

#### 难点：

* **原有数据结构中，没有直接存储预测图像、残差图像的数据结构：**
  * 预测图像和残差图像只是TU处理过程的中间过程产生的，需要分析TU的解码，并保存下来
* **原有数据结构中，保存的运动向量是不规则的宏块大小，没法直接使用：**
  * 需要根据预测模式对运动向量进行扩展

#### 解码图像=预测图像+残差图像（TU解码时分离）

* pixels=prediction+residual

```C++
int xP = x0;
int yP = y0;
const pic_parameter_set *pps = tctx->shdr->pps.get();
const seq_parameter_set *sps = pps->sps.get();
const int SubWidthC = sps->SubWidthC;
const int SubHeightC = sps->SubHeightC;
for (int cIdx = 0; cIdx < 3; cIdx++)
{
  for (int y = 0; y < nCS_L; y++)
  {
    for (int x = 0; x < nCS_L; x++)
    {
      int stride = tctx->img->get_image_stride(cIdx);
      int start_pos = xP+yP*stride;
        tctx->img->predictions[cIdx].at(start_pos + y * stride + x) =
          tctx->img->pixels[cIdx][start_pos + y * stride + x];
    }
  }
  if (img->get_chroma_format() == de265_chroma_mono){
    break;
  }
  if(cIdx==0){
      xP /= SubWidthC;
      nCS_L /= SubWidthC;

      yP /= SubHeightC;
      nCS_L /= SubHeightC;
  }
}
```

* 残差是经过YUV变换，然后DCT变换，再减去预测像素值，再除以量化表，再进行四舍五入量化，得到，然后进行解码的。

```C++
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
```

#### 运动向量（根据PB预测模式扩展）

```
class PBMotion
{

public:
  operator int() const
  {
    return 0;
  }

  std::array<uint8_t, 2> predFlag; // which of the two vectors is actually used
  std::array<int8_t, 2> refIdx;    // index into RefPicList
  std::array<MotionVector, 2> mv;  // the absolute motion vectors
  
};

class PBMotionCoding
{
public:
  // index into RefPicList
  int8_t refIdx[2];

  // motion vector difference
  int16_t mvd[2][2]; // [L0/L1][x/y]  (only in top left position - ???)

  // enum InterPredIdc, whether this is prediction from L0,L1, or BI
  uint8_t inter_pred_idc : 2;

  // which of the two MVPs is used
  uint8_t mvp_l0_flag : 1;
  uint8_t mvp_l1_flag : 1;

  // whether merge mode is used
  uint8_t merge_flag : 1;
  uint8_t merge_idx : 3;
};
```

* **是否需要经过scale处理？**
  * **"Absolute" 代表的是相对于某个参考点的位移，而不是描述向量的长度或方向的符号**。它指的是这些运动向量是相对真实的像素位移，而不是经过任何缩放、归一化等处理后的结果。
  * 保存的是具有1/4像素精度的运动向量，实际使用的时候，需要`/4`
  * **可视化：**需要搜索光流可视化，运动向量可视化可能搜索不到

```
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

        enum DrawMode what = PBMotionVectors;
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

void de265_image::PB_repeat(int x0,int y0, int w,int h, enum DrawMode what){
    const PBMotion& mvi = this->get_mv_info(x0,y0);
    // printf("PB_repeat...%d,%d,%d,%d\n",x0,y0,w,h);
    // printf("mvb.size():%d. mvb[0].size():%d ,%d,%d\n", mv_b.size(),mv_b[0].size(),x0+w,y0+h);
    if (mvi.predFlag[0])
    {
      for (int x = x0; x < x0 + w;++x){
        for (int y = y0; y < y0 + h;++y){
          std::array<int,3> pixel_mv = { mvi.mv[0].x, mvi.mv[0].y,mvi.refIdx[0]};
          mv_b[y][x] = pixel_mv;
        }
      }
    }
    if (mvi.predFlag[1]) {
      for (int x = x0; x < x0 + w;++x){
        for (int y = y0; y < y0 + h;++y){
          std::array<int,3> pixel_mv = { mvi.mv[1].x, mvi.mv[1].y,mvi.refIdx[1]};
          mv_f[y][x] = pixel_mv;
        }
      }
    }
}
```

#### crop回原尺寸

```
// 下采样倍数
SubWidthC = 1;
SubHeightC = 1;

WinUnitX = 1;
WinUnitY = 1;


// 真正的宽高
width_confwin，height_confwin
chroma_width_confwin， chroma_height_confwin

// left、top这些指的是在chroma上裁剪的数量，因为如果是在luma上裁剪的数量，那么left可能就是浮点数，那么会更难存储
```

### 5.通过pybind11构建动态库*.so(传回python)

* 通过pybind11定义好C++和python的类的通信方式，然后python就可以调用C++的函数，使用C++的类

```C++
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/stl_bind.h>
namespace py = pybind11;
PYBIND11_MAKE_OPAQUE(std::vector<de265_image*>);

PYBIND11_MODULE(dec265, m)
{
  m.doc() = "pybind11 example plugin";
  py::class_<de265_image>(m, "de265_image")
      .def_readwrite("mv_f", &de265_image::mv_f)
      .def_readwrite("mv_b", &de265_image::mv_b)
      .def_readwrite("slice_type", &de265_image::slice_type)
      .def_readwrite("predictions", &de265_image::predictions)
      .def_readwrite("residuals", &de265_image::residuals)
      .def_readwrite("quantPYs", &de265_image::quantPYs);    
}
```

```python
import dec265

dec265.getCTBinfo(imglist, filename)
```

## 项目构建

### autogen

* ```
  sudo apt-get install automake libtool libdpkg-perl
  sudo apt-get install gcc-7 g++-7
  sudo apt-get install python3.7 python3.7-dev python3-pip
  pip install pybind11
  ```

* ```
  cd dec265
  sh make-pybind11.sh
  ```

### cmake

* ```
  sudo apt-get install cmake gcc-7 g++-7
  sudo apt-get install python3.7 python3.7-dev python3-pip
  pip install pybind11
  ```

* ```
  cd libde265
  mkdir build && cd build
  cmake .. && make -j16
  ```

* ```
  cp dec265/dec265*.so ../dec265
  cd ../dec265
  python getCTBinfo-pybind11.py
  ```

### Docker

* ```
  docker build -t libde265-image -f Dockerfile .
  docker run --name libde265-container -it libde265-image /bin/bash
  ```

  

