// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlIO/RAW.h>

#include <tlCore/Error.h>
#include <tlCore/String.h>
#include <tlCore/StringFormat.h>

#include <array>
#include <sstream>

namespace tl
{
    namespace raw
    {
        Plugin::Plugin()
        {}

        std::shared_ptr<Plugin> Plugin::create(
            const std::shared_ptr<io::Cache>& cache,
            const std::weak_ptr<log::System>& logSystem)
        {
            auto out = std::shared_ptr<Plugin>(new Plugin);
            out->_init(
                "RAW",
                {
                    { ".arw", io::FileType::Sequence },
                    { ".crw", io::FileType::Sequence },
                    { ".cr2", io::FileType::Sequence },
                    { ".cr3", io::FileType::Sequence },
                    { ".dcr", io::FileType::Sequence },
                    { ".dng", io::FileType::Sequence },
                    { ".kdc", io::FileType::Sequence },
                    { ".mos", io::FileType::Sequence },
                    { ".nef", io::FileType::Sequence },
                    { ".raf", io::FileType::Sequence },
                    { ".raw", io::FileType::Sequence },
                    { ".rw2", io::FileType::Sequence },
                    { ".red", io::FileType::Sequence },
                },
                cache,
                logSystem);
            return out;
        }

        std::shared_ptr<io::IRead> Plugin::read(
            const file::Path& path,
            const io::Options& options)
        {
            return Read::create(
                path,
                io::merge(options, _options),
                _cache,
                _logSystem);
        }

        std::shared_ptr<io::IRead> Plugin::read(
            const file::Path& path,
            const std::vector<file::MemoryRead>& memory,
            const io::Options& options)
        {
            return Read::create(
                path,
                memory,
                io::merge(options, _options),
                _cache,
                _logSystem);
        }
    }
}
