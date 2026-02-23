cmake_minimum_required(VERSION 3.16)
project(pt3_app LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)

# ========================
# 源文件
# ========================
set(SOURCES
    main.cpp
    audio.cpp
    v4l2.cpp
    drmdisplay.cpp
    send2ai.cpp
)

add_executable(app ${SOURCES})

# ========================
# Whisper 路径
# ========================
set(WHISPER_ROOT /home/firefly/hzlwkspace/whisper/whisper.cpp-master)

target_include_directories(app PRIVATE
    ${WHISPER_ROOT}/include
    ${WHISPER_ROOT}/ggml/include
    /usr/include/freetype2
)

# libwhisper.so 在 build/src
target_link_directories(app PRIVATE
    ${WHISPER_ROOT}/build/src
)

# 运行时自动找到 libwhisper.so
set_target_properties(app PROPERTIES
    BUILD_RPATH ${WHISPER_ROOT}/build/src
)

# ========================
# 查找系统库
# ========================

find_package(PkgConfig REQUIRED)

pkg_check_modules(DRM REQUIRED libdrm)
pkg_check_modules(GBM REQUIRED gbm)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(GLES2 REQUIRED glesv2)

find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(JPEG REQUIRED)
find_package(Freetype REQUIRED)

# ========================
# 链接库
# ========================

target_link_libraries(app
    whisper
    ${DRM_LIBRARIES}
    ${GBM_LIBRARIES}
    ${EGL_LIBRARIES}
    ${GLES2_LIBRARIES}
    CURL::libcurl
    JPEG::JPEG
    Freetype::Freetype
    asound
    Threads::Threads
)
