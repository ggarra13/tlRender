// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#include <tlCore/HDR.h>

#include <tlCore/Error.h>
#include <tlCore/String.h>

namespace tl
{
    namespace image
    {
        TLRENDER_ENUM_IMPL(
            HDRPrimaries,
            "Red",
            "Green",
            "Blue",
            "White");
        TLRENDER_ENUM_SERIALIZE_IMPL(HDRPrimaries);

        void to_json(nlohmann::json& json, const HDRBezier& value)
        {
            json = nlohmann::json
            {
                { "targetLuma", value.targetLuma },
                { "kneeX", value.kneeX },
                { "kneeY", value.kneeY },
                { "numAnchors", value.numAnchors },
                { "anchors0", value.anchors[0] },
                { "anchors1", value.anchors[1] },
                { "anchors2", value.anchors[2] },
                { "anchors3", value.anchors[3] },
                { "anchors4", value.anchors[4] },
                { "anchors5", value.anchors[5] },
                { "anchors6", value.anchors[6] },
                { "anchors7", value.anchors[7] },
                { "anchors8", value.anchors[8] },
                { "anchors9", value.anchors[9] },
                { "anchors10", value.anchors[10] },
                { "anchors11", value.anchors[11] },
                { "anchors12", value.anchors[12] },
                { "anchors13", value.anchors[13] },
                { "anchors14", value.anchors[14] },
            };
        }

        void from_json(const nlohmann::json& json, HDRBezier& value)
        {
            json.at("targetLuma").get_to(value.targetLuma);
            json.at("kneeX").get_to(value.kneeX);
            json.at("kneeY").get_to(value.kneeY);
            json.at("numAnchors").get_to(value.numAnchors);
            json.at("anchors0").get_to(value.anchors[0]);
            json.at("anchors1").get_to(value.anchors[1]);
            json.at("anchors2").get_to(value.anchors[2]);
            json.at("anchors3").get_to(value.anchors[3]);
            json.at("anchors4").get_to(value.anchors[4]);
            json.at("anchors5").get_to(value.anchors[5]);
            json.at("anchors6").get_to(value.anchors[6]);
            json.at("anchors7").get_to(value.anchors[7]);
            json.at("anchors8").get_to(value.anchors[8]);
            json.at("anchors9").get_to(value.anchors[9]);
            json.at("anchors10").get_to(value.anchors[10]);
            json.at("anchors11").get_to(value.anchors[11]);
            json.at("anchors12").get_to(value.anchors[12]);
            json.at("anchors13").get_to(value.anchors[13]);
            json.at("anchors14").get_to(value.anchors[14]);
        }
        
        void to_json(nlohmann::json& json, const HDRData& value)
        {
            json = nlohmann::json
            {
                { "eotf", value.eotf },
                { "primaries", value.primaries },
                { "displayMasteringLuminance", value.displayMasteringLuminance },
                { "maxCLL", value.maxCLL },
                { "maxFALL", value.maxFALL },
                { "sceneMax0", value.sceneMax[0] },
                { "sceneMax1", value.sceneMax[1] },
                { "sceneMax2", value.sceneMax[2] },
                { "sceneAvg", value.sceneAvg },
                { "ootf", value.ootf },
                { "maxPQY", value.maxPQY },
                { "avgPQY", value.avgPQY },
            };
        }

        void from_json(const nlohmann::json& json, HDRData& value)
        {
            json.at("eotf").get_to(value.eotf);
            json.at("primaries").get_to(value.primaries);
            json.at("displayMasteringLuminance").get_to(value.displayMasteringLuminance);
            json.at("maxCLL").get_to(value.maxCLL);
            json.at("maxFALL").get_to(value.maxFALL);
            json.at("sceneMax0").get_to(value.sceneMax[0]);
            json.at("sceneMax1").get_to(value.sceneMax[1]);
            json.at("sceneMax2").get_to(value.sceneMax[2]);
            json.at("sceneAvg").get_to(value.sceneAvg);
            json.at("ootf").get_to(value.ootf);
            json.at("maxPQY").get_to(value.maxPQY);
            json.at("avgPQY").get_to(value.avgPQY);
        }
    }
}
