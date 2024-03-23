
#include <tlCore/NDIOptions.h>

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
            };
        }

        void from_json(const nlohmann::json& json, Options& value)
        {
            json.at("sourceName").get_to(value.sourceName);
            json.at("noAudio").get_to(value.noAudio);
        }

    }
}
