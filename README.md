# libde265

* Fork from github：https://github.com/strukturag/libde265

## Encoder

* [ffmepg](https://www.ffmpeg.org/)
* [HEVC](https://hevc.hhi.fraunhofer.de/)

### Libde265  and Pybind11 install and compile.

* ```
  sudo apt-get install automake libtool libdpkg-perl
  sudo apt-get install cmake gcc-7 g++-7 qt4-qmake libqt4-dev
  ```

* Configure libde265.

  * ```
    ./autogen.sh
    ./configure
    ```

* Install pybind11和python3.7-dev.

  * ```
    sudo apt-get install python3.7 python3.7-dev 
    pip install "pybind11[global]"
    ```

* Add python library to dec265/Makefile.

  * ```
    -fPIC ${shell python3.7-config --cflags} //append after CXXFLAGS
    -L/usr/lib/python3.7/config-3.7m-x86_64-linux-gnu -lpython3.7  // append after LIBS
    ```

* Compile dynamic library (*.so) for pybind11.

  * ```
    cd dec265
    sh make-pybind11.sh
    ```

## Save coding prior.

* ```
  cd dec265
  python getCTBinfo-pybind11.py
  ```

