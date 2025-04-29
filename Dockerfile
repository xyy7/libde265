FROM ubuntu:18.04

# 设置环境变量避免交互式安装提示
ENV DEBIAN_FRONTEND=noninteractive

# 设置默认命令（可选）
CMD ["/bin/bash"]


# 设置清华源并更新包列表
RUN sed -i 's|http://archive.ubuntu.com|http://mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list && \
    sed -i 's|http://security.ubuntu.com|http://mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list && \
    apt-get update

# 安装依赖包，添加 -y 自动确认
RUN apt-get install -y cmake gcc-7 g++-7 python3.7 python3.7-dev python3-pip && \
    apt-get install -y --fix-missing cmake gcc-7 g++-7 python3.7 python3.7-dev python3-pip && \
    # 创建python3.7的软链接
    ln -s /usr/bin/python3.7 /usr/bin/python && \
    # 升级pip并安装python包
    python -m pip install --upgrade pip && \
    python -m pip install numpy pybind11
	
# 设置环境变量
ENV pybind11_DIR=/usr/local/lib/python3.7/dist-packages/pybind11/share/cmake/pybind11/

# 安装git并克隆libde265仓库
RUN apt-get install -y git && \
    cd /root && \
    git clone https://github.com/xyy7/libde265.git && \
    cd libde265 && \
    mkdir build && \
    cd build && \
    cmake .. && \
    make -j16 && \  
	cp dec265/dec265.cpython-37m-x86_64-linux-gnu.so ../dec265/ && \
    cd ../dec265/ && \
    python getCTBinfo-pybind11.py
    