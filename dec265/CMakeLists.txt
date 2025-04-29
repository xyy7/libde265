# ---------------------添加Python相关配置---------------------
# 查找系统 Python 3.7
# find_package(Python3 COMPONENTS Development REQUIRED) # python-dev没有提供cmake，需要自己编写
find_program(PYTHON3_EXECUTABLE NAMES python3.7 PATHS /usr/bin)
find_path(PYTHON3_INCLUDE_DIR Python.h PATHS /usr/include/python3.7m)
find_library(PYTHON3_LIBRARY NAMES python3.7m PATHS /usr/lib/x86_64-linux-gnu)

if(NOT PYTHON3_EXECUTABLE OR NOT PYTHON3_INCLUDE_DIR OR NOT PYTHON3_LIBRARY)
    message(FATAL_ERROR "Python 3.7 development files not found!")
endif()

execute_process(
    COMMAND ${PYTHON3_EXECUTABLE}-config --extension-suffix
    OUTPUT_VARIABLE PYTHON_EXT_SUFFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# 设置变量供 CMake 使用
set(Python3_EXECUTABLE ${PYTHON3_EXECUTABLE})
set(Python3_INCLUDE_DIRS ${PYTHON3_INCLUDE_DIR})
set(Python3_LIBRARIES ${PYTHON3_LIBRARY})

find_package(pybind11 REQUIRED)  # 如果需要pybind11


# target_compile_options(dec265 PRIVATE ${Python3_COMPILE_OPTIONS})
# target_compile_definitions(dec265 PRIVATE ${Python3_DEFINITIONS})


# 创建动态库目标
add_library(dec265 SHARED dec265.cc)  # 使用MODULE而不是SHARED以获得更好的Python扩展支持


target_include_directories(dec265 PRIVATE 
    ${Python3_INCLUDE_DIRS}
    ${pybind11_INCLUDE_DIRS}  # 如果需要
)

# 设置C++标准
set_target_properties(dec265 PROPERTIES 
    PREFIX ""  # 移除前缀
    SUFFIX "${PYTHON_EXT_SUFFIX}"  # 使用Python扩展后缀
)

# # 添加链接选项
target_link_libraries(dec265 PRIVATE 
    de265 
    ${Python3_LIBRARIES} 
    -lpthread 
    -lm
)

# 处理SDL2选项
if(SDL2_FOUND)
  target_sources(dec265 PRIVATE sdl.cc)
  target_compile_definitions(dec265 PRIVATE HAVE_SDL)
  target_include_directories(dec265 PRIVATE "${SDL2_INCLUDE_DIRS}")
  target_link_libraries(dec265 PRIVATE ${SDL2_LIBRARIES})
endif()

# 处理MSVC选项
if(MSVC)
  target_sources(dec265 PRIVATE
    ../extra/getopt.c
    ../extra/getopt_long.c
  )
endif()

# 安装规则
# install(TARGETS dec265 DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
    TARGETS dec265
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# 如果不是MSVC，添加hdrcopy目标
if(NOT MSVC)
 add_executable(hdrcopy hdrcopy.cc)
 target_link_libraries(hdrcopy PRIVATE de265)
 install(TARGETS hdrcopy DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()