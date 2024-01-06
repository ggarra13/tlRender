// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlIO/NDIReadPrivate.h>

#include <tlCore/StringFormat.h>

namespace tl
{
    namespace ndi
    {
        ReadAudio::ReadAudio(
            const std::string& fileName,
            const std::vector<file::MemoryRead>& memory,
            double videoRate,
            const Options& options) :
            _fileName(fileName),
            _options(options)
        {
        }

        bool ReadAudio::isValid() const
        {
            return true;
        }

        const audio::Info& ReadAudio::getInfo() const
        {
            return _info;
        }

        const otime::TimeRange& ReadAudio::getTimeRange() const
        {
            return _timeRange;
        }

        void ReadAudio::start()
        {
        }

        // void ReadAudio::seek(const otime::RationalTime& time)
        // {
        //     //std::cout << "audio seek: " << time << std::endl;

        // }

        bool ReadAudio::process(
            const otime::RationalTime& currentTime,
            size_t sampleCount)
        {
            bool out = false;
            return out;
        }

        size_t ReadAudio::getBufferSize() const
        {
            return audio::getSampleCount(_buffer);
        }

        void ReadAudio::bufferCopy(uint8_t* out, size_t sampleCount)
        {
            audio::move(_buffer, out, sampleCount);
        }

        int ReadAudio::_decode(const otime::RationalTime& currentTime)
        {
            int out = 0;
            while (0 == out)
            {
                out = 1;
            }
            return out;
        }
    }
}
