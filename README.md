# libde265

* fork from github：https://github.com/strukturag/libde265

## Encoder

* [ffmepg](https://www.ffmpeg.org/)
* [HEVC](https://hevc.hhi.fraunhofer.de/)

### Libde265  and Pybind11 install and compile.

* Configure libde265.

  * ```
    ./autogen.sh
    ./configure
    ```

* Install pybind11和python3.x-dev:

  * ```
    pip install pybind11
    sudo apt install python3.7-dev 
    ```

* 查看头文件位置，查看库文件位置，makefile中添加链接库：【直接使用虚拟环境中的python是不行的】

  * ```
    -fPIC ${shell /usr/bin/python3.7-config --cflags} //命令行执行时不需要shell #加在CXXFLAGS后面
    -L/usr/lib/python3.7/config-3.7m-x86_64-linux-gnu -lpython3.7  #加在LIBS后面
    ```

  * ```
    g++ -g -O2 -Werror=return-type -Werror=unused-result -Werror=reorder -std=gnu++11  -I/data/chenminghui/anaconda3/envs/invcomp/include -DDE265_LOG_ERROR -fPIC $(python3 -m pybind11 --includes) -shared -o dec265`python3-config --extension-suffix` dec265-dec265.o  ../libde265/.libs/libde265.so -lstdc++ -lpthread -lm 
    ```

* ```
  cd dec265
  sh make-pybind11.sh
  ```

## Save coding prior.

* ```
  cd dec265
  python getCTBinfo-pybind11.py
  ```



