#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# mrv2
# Copyright Contributors to the mrv2 Project. All rights reserved.



#
# Script to compile the glfw demo (no care of platform, builds in
# ./build directory
#

mkdir -p build
cd build

export PATH="$PWD/install/bin:${PATH}"

cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=$PWD/install -D CMAKE_PREFIX_PATH=$PWD/install -D TLRENDER_OCIO=OFF -D TLRENDER_FREETYPE=OFF -D TLRENDER_EXR=OFF -D TLRENDER_TIFF=OFF -D BUILD_FFMPEG=ON -D TLRENDER_AUDIO=ON -D TLRENDER_GL=ON -D TLRENDER_PROGRAMS=OFF -D TLRENDER_EXAMPLES=ON -D TLRENDER_TESTS=OFF ../etc/SuperBuild

cmake --build . --config Release

cd -


