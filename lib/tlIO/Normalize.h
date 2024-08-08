// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-Present Gonzalo Garramuño.
// All rights reserved.

#pragma once

#include <tlCore/Image.h>

namespace tl
{
    namespace io
    {
        void normalizeImage(std::shared_ptr<image::Image> inout,
                            const image::Info& info, const int minX,
                            const int maxX, const int minY,
                            const int maxY);
        
        void invalidValues(std::shared_ptr<image::Image> inout,
                           const image::Info& info, const int minX,
                           const int maxX, const int minY,
                           const int maxY, const float invalid = 0.8F,
                           const float minValue = 0.F,
                           const float maxValue = 1.F);
    }
}
