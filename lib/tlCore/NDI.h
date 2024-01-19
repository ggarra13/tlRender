// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2024 Gonzalo Garramuño
// All rights reserved.

#pragma once

#include <nlohmann/json.hpp>

#include <string>

namespace tl
{
    namespace ndi
    {
        struct Options
        {
            std::string sourceName;
            bool        noAudio     = false;
            bool yuvToRGBConversion = false;
            size_t requestTimeout = 5;
            size_t videoBufferSize = 4;
        };
        
        //! \name Serialize
        ///@{

        void to_json(nlohmann::json&, const Options&);

        void from_json(const nlohmann::json&, Options&);

        ///@}
    }
}
