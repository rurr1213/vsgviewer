cmake_minimum_required(VERSION 3.10)

project(vsgviewer)

set(CMAKE_FIND_PACKAGE_NAME_vsg VERBOSE TRUE)  # for explicit vsg finding
set(CMAKE_FIND_PACKAGE_VERBOSE TRUE)          # for general verbose output

include_directories("/home/ravi/miniconda3/envs/jupyterlab_env/targets/x86_64-linux/include/")
include_directories("/home/ravi/dev/video-sdk-samples/Samples/NvCodec")

set(NV_VIDEO_SDK
    /home/ravi/dev/video-sdk-samples
)
set(NVENCODER_SOURCES
    ${NV_VIDEO_SDK}/Samples/NvCodec/NvEncoder/NvEncoder.cpp
    ${NV_VIDEO_SDK}/Samples/NvCodec/NvEncoder/NvEncoderCuda.cpp
)

find_package(vsg REQUIRED)
find_package(vsgXchange REQUIRED)

set(SOURCES
    vsgviewer.cpp
    ${NVENCODER_SOURCES}
    H264NVEncoder.h
    PipeToFFmpeg.h
)

add_executable(vsgviewer ${SOURCES})

link_directories("/home/ravi/miniconda3/envs/jupyterlab_env/targets/x86_64-linux/lib/")
link_directories("/lib/x86_64-linux-gnu/")

target_compile_definitions(vsgviewer PRIVATE vsgXchange_FOUND)
target_link_libraries(vsgviewer
    vsg::vsg
    vsgXchange::vsgXchange
    /lib/x86_64-linux-gnu/libcuda.so.1
)

install(TARGETS vsgviewer RUNTIME DESTINATION bin)
