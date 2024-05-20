#!/usr/bin/env bash
# SPDX-License-Identifier: BSD-3-Clause
# mrv2 
# Copyright Contributors to the mrv2 Project. All rights reserved.


#
# Turn on exit on error
#
set -o pipefail -e

export BUILD_DIR=_build

rm -f $BUILD_DIR/tlRender/src/tlRender-build/bin/tlplay-gl/tlplay-gl

cd $BUILD_DIR/tlRender/src/tlRender-build

cmake --build . -j 17 --config Release -t install

cd -

