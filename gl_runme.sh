#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# mrv2 
# Copyright Contributors to the mrv2 Project. All rights reserved.




export TLRENDER_ASAN=OFF
export TLRENDER_AV1=OFF
export TLRENDER_FFMPEG=ON
export TLRENDER_FFMPEG_MINIMAL=ON
export TLRENDER_JPEG=OFF
export TLRENDER_NET=OFF
export TLRENDER_QT6=OFF
export TLRENDER_QT5=OFF
export TLRENDER_RAW=OFF
export TLRENDER_STB=OFF
export TLRENDER_TIFF=OFF
export TLRENDER_USD=OFF
export TLRENDER_VPX=OFF
export TLRENDER_WAYLAND=ON
export TLRENDER_X11=ON
export TLRENDER_YASM=OFF

export BUILD_DIR=_build

mkdir -p $BUILD_DIR

cd $BUILD_DIR

cmake ../etc/SuperBuild -D CMAKE_VERBOSE_MAKEFILE=ON \
      -D CMAKE_BUILD_TYPE=Debug \
      -D CMAKE_INSTALL_PREFIX=$PWD/install \
      -D CMAKE_PREFIX_PATH=$PWD/install \
      -D TLRENDER_PROGRAMS=ON \
      -D TLRENDER_EXAMPLES=OFF \
      -D TLRENDER_TESTS=OFF \
      -D TLRENDER_ASAN=${TLRENDER_ASAN} \
      -D TLRENDER_AV1=${TLRENDER_AV1} \
      -D TLRENDER_FFMPEG=${TLRENDER_FFMPEG} \
      -D TLRENDER_FFMPEG_MINIMAL=${TLRENDER_FFMPEG_MINIMAL} \
      -D TLRENDER_JPEG=${TLRENDER_JPEG} \
      -D TLRENDER_NET=${TLRENDER_NET} \
      -D TLRENDER_QT6=${TLRENDER_QT6} \
      -D TLRENDER_QT5=${TLRENDER_QT5} \
      -D TLRENDER_RAW=${TLRENDER_RAW} \
      -D TLRENDER_SGI=${TLRENDER_SGI} \
      -D TLRENDER_STB=${TLRENDER_STB} \
      -D TLRENDER_TIFF=${TLRENDER_TIFF} \
      -D TLRENDER_USD=${TLRENDER_USD} \
      -D TLRENDER_VPX=${TLRENDER_VPX} \
      -D TLRENDER_WAYLAND=${TLRENDER_WAYLAND} \
      -D TLRENDER_X11=${TLRENDER_X11} \
      -D TLRENDER_YASM=${TLRENDER_YASM} \

cmake --build . -j 17 --config Debug 2>&1 | tee ../compile.log

cd -

