// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlIO/NDI.h>

#include <tlCore/LogSystem.h>
#include <tlCore/String.h>

#include <array>

namespace tl
{
    namespace ndi
    {
        void Plugin::_init(
            const std::shared_ptr<io::Cache>& cache,
            const std::weak_ptr<log::System>& logSystem)
        {
            IPlugin::_init(
                "ndi",
                {
                    "ndi"
                },
                cache,
                logSystem);

            _logSystemWeak = logSystem;

            // if (auto logSystem = _logSystemWeak.lock())
            // {
            //     logSystem->print("tl::io::ndi::Plugin", "Codecs: " + string::join(codecNames, ", "));
            // }
        }

        Plugin::Plugin()
        {}

        std::shared_ptr<Plugin> Plugin::create(
            const std::shared_ptr<io::Cache>& cache,
            const std::weak_ptr<log::System>& logSystem)
        {
            auto out = std::shared_ptr<Plugin>(new Plugin);
            out->_init(cache, logSystem);
            return out;
        }

        std::shared_ptr<io::IRead> Plugin::read(
            const file::Path& path,
            const io::Options& options)
        {
            return Read::create(path, options, _cache, _logSystem);
        }

        std::shared_ptr<io::IRead> Plugin::read(
            const file::Path& path,
            const std::vector<file::MemoryRead>& memory,
            const io::Options& options)
        {
            return Read::create(path, memory, options, _cache, _logSystem);
        }
    }
}
