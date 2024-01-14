// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Gonzalo Garramuño
// All rights reserved.

#include <tlIO/NDIReadPrivate.h>

#include <tlCore/StringFormat.h>

namespace tl
{
    namespace ndi
    {
        
        ReadAudio::ReadAudio(
            const std::string& fileName,
            const NDIlib_audio_frame_t& audio_frame,
            const Options& options) :
            _fileName(fileName),
            _options(options)
        {
            _info.channelCount = audio_frame.no_channels;
            _info.sampleRate   = audio_frame.sample_rate;
            _info.dataType     = audio::DataType::F32;

            double start = 0.0;
            double last = kNDI_MOVIE_DURATION; // 1 minute
            _timeRange = otime::TimeRange(
                otime::RationalTime(start, 1.0).rescaled_to(_info.sampleRate),
                otime::RationalTime(last, 1.0).rescaled_to(_info.sampleRate));
        }

        ReadAudio::~ReadAudio()
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
        

        bool ReadAudio::process(
            const otime::RationalTime& currentTime,
            const NDIlib_audio_frame_t& audio_frame)
        {
            bool out = false;
            auto tmp = audio::Audio::create(_info, audio_frame.no_samples);
            memcpy(tmp->getData(), audio_frame.p_data,
                   tmp->getByteCount());
            tmp = audio::planarInterleave(tmp);
            _buffer.push_back(tmp);
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
    }
}
