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
            const double videoRate,
            const Options& options) :
            _fileName(fileName),
            _options(options)
        {
            _info.channelCount = audio_frame.no_channels;
            _info.sampleRate   = audio_frame.sample_rate;
            _info.dataType     = audio::DataType::F32;

            double fps = videoRate;
            double start = 0.0 * fps;
            //double last = 3 * 60 * 60 * fps; // 3 hours time range
            double last = 60 * 1 * fps; // 1 minute
            _timeRange = otime::TimeRange(
                otime::RationalTime(start, fps).rescaled_to(_info.sampleRate),
                otime::RationalTime(last, fps).rescaled_to(_info.sampleRate));
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
        
        void ReadAudio::_calculateCurrentTime(
            const NDIlib_audio_frame_t& audio_frame)
        {
            AVRational r;
            r.num = 1;
            r.den = _info.sampleRate;
            
            const int64_t pts = audio_frame.timecode;
            
            _currentTime = otime::RationalTime(
                _timeRange.start_time().value() +
                av_rescale_q(pts, NDI_TIME_BASE_Q, r),
                _info.sampleRate);
                
            _duration    = otime::RationalTime(
                av_rescale_q(audio_frame.no_samples, NDI_TIME_BASE_Q, r),
                _info.sampleRate);
        }
    }
}
