# add_executable (dec265 dec265.cc)

# target_link_libraries (dec265 PRIVATE de265)

# add_compile_options(-fPIC)
# # 包含 Python 头文件 -I/usr/include/python3.7m -I/usr/include/python3.7m  -Wno-unused-result -Wsign-compare -g -fdebug-prefix-map=/build/python3.7-uZpGrP/python3.7-3.7.5=. -specs=/usr/share/dpkg/no-pie-compile.specs -fstack-protector -Wformat -Werror=format-security  -DNDEBUG -g -fwrapv -O3 -Wall
# target_include_directories(dec265 PRIVATE "")


# if(SDL2_FOUND)
#   target_sources(dec265 PRIVATE sdl.cc)
#   target_compile_definitions(dec265 PRIVATE HAVE_SDL)
#   target_include_directories (dec265 PRIVATE "${SDL2_INCLUDE_DIRS}")
#   target_link_libraries (dec265 PRIVATE ${SDL2_LIBRARIES})
# endif()

# if(MSVC)
#   target_sources(dec265 PRIVATE
#     ../extra/getopt.c
#     ../extra/getopt_long.c
#   )
# endif()

# install (TARGETS dec265 DESTINATION ${CMAKE_INSTALL_BINDIR})

# #if(NOT MSVC)
# #  # hdrcopy uses internal APIs that are not available when compiled for Windows
# #  add_executable (hdrcopy hdrcopy.cc)
# #
# #  target_link_libraries (hdrcopy PRIVATE de265)
# #
# #  install (TARGETS hdrcopy DESTINATION ${CMAKE_INSTALL_BINDIR})
# #endif()
