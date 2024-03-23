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
            const NDIlib_source_t& NDIsource,
            const NDIlib_audio_frame_t& audio_frame,
            const std::weak_ptr<log::System>& logSystem,
            const Options& options) :
            _fileName(fileName),
            _logSystem(logSystem),
            _options(options)
        {
            _info.channelCount = audio_frame.no_channels;
            _info.sampleRate   = audio_frame.sample_rate;
            _info.dataType     = audio::DataType::F32;

            double start = 0.0;
            double last = kNDI_MOVIE_DURATION;
            _timeRange = otime::TimeRange(
                otime::RationalTime(start, 1.0).rescaled_to(_info.sampleRate),
                otime::RationalTime(last, 1.0).rescaled_to(_info.sampleRate));

            // We now have at least one source,
            // so we create a receiver to look at it.
            NDIlib_recv_create_v3_t recv_desc;
            recv_desc.color_format = NDIlib_recv_color_format_fastest;
            recv_desc.bandwidth = NDIlib_recv_bandwidth_highest;
            recv_desc.allow_video_fields = false;
            recv_desc.source_to_connect_to = NDIsource;

            NDI_recv = NDIlib_recv_create(&recv_desc);
            if (!NDI_recv)
                throw std::runtime_error("Could not create NDI audio receiver");
            
            _from_ndi(audio_frame);
        }

        ReadAudio::~ReadAudio()
        {
            if (NDI_recv)
                NDIlib_recv_destroy(NDI_recv);
        }

        const audio::Info& ReadAudio::getInfo() const
        {
            return _info;
        }

        const otime::TimeRange& ReadAudio::getTimeRange() const
        {
            return _timeRange;
        }

        void ReadAudio::seek(const otime::RationalTime& time)
        {
            _buffer.clear();
        }
        
        bool ReadAudio::process(
            const otime::RationalTime& currentTime,
            size_t sampleCount)
        {
            bool out = true;
            const size_t bufferSampleCount = getBufferSize();
            if (bufferSampleCount < sampleCount)
            {
                int decoding = _decode(currentTime);
                if (decoding < 0)
                {
                    //! \todo error out.
                    out = false;
                    NDI_recv = nullptr;
                }
            }
            return out;
        }

        void ReadAudio::_from_ndi(const NDIlib_audio_frame_t& a)
        {
            auto tmp = audio::Audio::create(_info, a.no_samples);
            memcpy(tmp->getData(), a.p_data, tmp->getByteCount());
            tmp = audio::planarInterleave(tmp);
            _buffer.push_back(tmp);
        }
        
        int ReadAudio::_decode(const otime::RationalTime& time)
        {
            int out = 0;
            NDIlib_audio_frame_t a;
            NDIlib_frame_type_e type_e;

            while (out == 0)
            {
                type_e = NDIlib_recv_capture(NDI_recv, nullptr, &a, nullptr, 50);
                if (type_e == NDIlib_frame_type_error)
                {
                    out = -1;
                }
                else if (type_e == NDIlib_frame_type_audio)
                {
                    _from_ndi(a);
                    NDIlib_recv_free_audio(NDI_recv, &a);
                    out = 1;
                }
            }
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
