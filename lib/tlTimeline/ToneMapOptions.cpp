// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#include <tlTimeline/ToneMapOptions.h>

namespace tl
{
    namespace timeline
    {
        void to_json(nlohmann::json& json, const ToneMapOptions& value)
        {
            json = nlohmann::json
            {
                { "enabled", value.enabled },
                { "hdrData", value.hdrData },
            };
        }

        void from_json(const nlohmann::json& json, ToneMapOptions& value)
        {
            json.at("enabled").get_to(value.enabled);
            json.at("hdrData").get_to(value.hdrData);
        }
    }
}
