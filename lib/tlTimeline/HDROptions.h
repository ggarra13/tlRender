// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Gonzalo Garramuño
// All rights reserved.

#include <tlCore/HDR.h>

namespace tl
{
    namespace timeline
    {
        //! Tonemap options.
        struct HDROptions
        {
            bool               tonemap = false;
            image::HDRData     hdrData;
            
            bool operator == (const HDROptions&) const;
            bool operator != (const HDROptions&) const;
        };
    }
}

#include <tlTimeline/HDROptionsInline.h>
