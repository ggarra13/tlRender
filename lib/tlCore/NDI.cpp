
#include <tlCore/NDI.h>

namespace tl
{
    namespace ndi
    {
        void to_json(nlohmann::json& json, const Options& value)
        {
            json = nlohmann::json
            {
                { "sourceName", value.sourceName },
                { "noAudio", value.noAudio },
                { "yuvToRGBConversion", value.yuvToRGBConversion },
                { "requestTimeout", value.requestTimeout },
                { "videoBufferSize", value.videoBufferSize },
            };
        }

        void from_json(const nlohmann::json& json, Options& value)
        {
            json.at("sourceName").get_to(value.sourceName);
            json.at("noAudio").get_to(value.noAudio);
            json.at("yuvToRGBConversion").get_to(value.yuvToRGBConversion);
            json.at("requestTimeout").get_to(value.requestTimeout );
            json.at("videoBufferSize").get_to(value.videoBufferSize );
        }

    }
}
