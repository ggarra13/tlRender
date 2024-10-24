// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#pragma once

#include <nlohmann/json.hpp>

#include <tlCore/HDR.h>

namespace tl
{
    namespace timeline
    {
        //! OpenColorIO options.
        struct ToneMapOptions
        {
            bool               enabled = false;
            image::HDRData     hdrData;
            
            bool operator == (const ToneMapOptions&) const;
            bool operator != (const ToneMapOptions&) const;
        };

        void to_json(nlohmann::json&, const ToneMapOptions&);

        void from_json(const nlohmann::json&, ToneMapOptions&);
    }
}

#include <tlTimeline/ToneMapOptionsInline.h>
