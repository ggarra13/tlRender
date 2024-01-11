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
            NDIlib_recv_instance_t recv,
            const double videoRate,
            const Options& options) :
            pNDI_recv(recv),
            _fileName(fileName),
            _options(options)
        {
            
            NDIlib_audio_frame_t audio_frame;
            NDIlib_frame_type_e type_e = NDIlib_frame_type_none;
            
            while(type_e != NDIlib_frame_type_audio)
            {
                type_e = NDIlib_recv_capture(
                    pNDI_recv, nullptr, &audio_frame, nullptr, 5000);
            }

            _info.channelCount = audio_frame.no_channels;
            _info.sampleRate   = audio_frame.sample_rate;
            _info.dataType     = audio::DataType::F32;

            double fps = videoRate;
            //double last = 3 * 60 * 60 * fps; // 3 hours time range
            double start = 0 * fps;
            double last = 60 * 1 * fps; // 1 minute
            _timeRange = otime::TimeRange(
                otime::RationalTime(start, fps).rescaled_to(_info.sampleRate),
                otime::RationalTime(last, fps).rescaled_to(_info.sampleRate));

            _calculateCurrentTime(audio_frame);
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
        
        otime::RationalTime ReadAudio::getTime() const
        {
            return _currentTime;
        }

        otime::RationalTime ReadAudio::getDuration() const
        {
            return _duration;
        }

        void ReadAudio::seek(const otime::RationalTime& time)
        {
            _buffer.clear();
        }

        bool ReadAudio::process(
            const otime::RationalTime& currentTime,
            size_t sampleCount)
        {
            bool out = false;
            const size_t bufferSampleCount = audio::getSampleCount(_buffer);
            if (bufferSampleCount < sampleCount)
            {
                int decoding = 0;
                while (0 == decoding)
                {
                    decoding = _decode(currentTime);
                }
            }
            out = true;
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
        
        int ReadAudio::_decode(const otime::RationalTime& currentTime)
        {
            int out = 0;

            while(out == 0 && running)
            {
                NDIlib_audio_frame_t audio_frame;
                NDIlib_frame_type_e type_e = NDIlib_frame_type_none;
                while(type_e != NDIlib_frame_type_audio)
                {
                    type_e = NDIlib_recv_capture(
                        pNDI_recv, nullptr, &audio_frame, nullptr, 5000);
                }
                
                AVRational r;
                r.num = 1;
                r.den = _info.sampleRate;

                _calculateCurrentTime(audio_frame);

                if (_currentTime >= currentTime)
                {
                    auto tmp =
                        audio::Audio::create(_info, audio_frame.no_samples);
                    memcpy(tmp->getData(), audio_frame.p_data,
                           tmp->getByteCount());
                    tmp = audio::planarInterleave(tmp);
                    
                    // Free the original buffer
                    NDIlib_recv_free_audio(pNDI_recv, &audio_frame);

                    _buffer.push_back(tmp);
                    out = 1;
                    break;
                }
                
                // Free the original buffer
                NDIlib_recv_free_audio(pNDI_recv, &audio_frame);
            }
            return out;
        }
    }
}
