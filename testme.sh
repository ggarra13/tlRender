#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# mrv2
# Copyright Contributors to the mrv2 Project. All rights reserved.



#
#
# Test build script for tlRender.
# It builds almost no dependencies and will install the
# main executable on build/install/bin.
#
#

mkdir -p build
cd build


cmake -D CMAKE_BUILD_TYPE=Release -D CMAKE_INSTALL_PREFIX=$PWD/install -D CMAKE_PREFIX_PATH=$PWD/install -D TLRENDER_OCIO=OFF -D TLRENDER_FREETYPE=OFF -D TLRENDER_EXR=OFF -D TLRENDER_TIFF=OFF -D BUILD_FFMPEG=OFF -D TLRENDER_AUDIO=OFF -D TLRENDER_GL=OFF -D TLRENDER_PROGRAMS=OFF -D TLRENDER_EXAMPLES=OFF -D TLRENDER_TESTS=OFF ../etc/SuperBuild

cmake --build . --config Release

cd -


