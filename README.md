# libde265

* Fork from github：https://github.com/strukturag/libde265

## Build

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

  

