# libde265

* Fork from github：https://github.com/strukturag/libde265

## Encoder

* [ffmepg](https://www.ffmpeg.org/)
* [HEVC](https://hevc.hhi.fraunhofer.de/)

### Libde265  and Pybind11 install and compile.

* ```
  sudo apt-get update
  sudo apt-get install automake
  sudo apt-get install libtool
  sudo apt-get install cmake gcc g++ qt4-qmake libqt4-dev
  ```

- Install [libvideogfx](http://github.com/farindk/libvideogfx).

  - ```
    git clone https://github.com/farindk/libvideogfx.git
    cd libvideogfx/
    ./autogen.sh
    ./configure
    ```

* Configure libde265.

  * ```
    ./autogen.sh
    ./configure
    ```

* Install pybind11和python3.7-dev.

  * ```
    pip install pybind11
    sudo apt-get install python3.7-dev 
    ```

* Add python library to Makefile.

  * ```
    sed -i '/CXXFLAGS/s/$/ -fPIC ${shell /usr/bin/python3.7-config --cflags}' Makefile
    sed -i '/LIBS/s/$/ -L/usr/lib/python3.7/config-3.7m-x86_64-linux-gnu -lpython3.7' Makefile
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

