cmake_minimum_required(VERSION 3.10)

project(vsgviewer)

set(CMAKE_FIND_PACKAGE_NAME_vsg VERBOSE TRUE)  # for explicit vsg finding
set(CMAKE_FIND_PACKAGE_VERBOSE TRUE)          # for general verbose output

find_package(vsg REQUIRED)
find_package(vsgXchange REQUIRED)

set(SOURCES
    vsgviewer.cpp
)

add_executable(vsgviewer ${SOURCES})

target_compile_definitions(vsgviewer PRIVATE vsgXchange_FOUND)
target_link_libraries(vsgviewer vsg::vsg vsgXchange::vsgXchange)

install(TARGETS vsgviewer RUNTIME DESTINATION bin)
