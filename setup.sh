#!/bin/bash

BUILD_TYPE="Debug"  # Change this to "Release" as needed

rm -rf build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ..
cmake --build .
cd .. #Return to previous directory

