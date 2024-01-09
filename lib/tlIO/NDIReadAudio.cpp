// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Gonzalo Garramuño
// All rights reserved.

#include <tlIO/NDIReadPrivate.h>

#include <tlCore/StringFormat.h>

#  define DBG(x)
#if 1
#  define DBG2(x) \
    std::cerr << x << " " << __FUNCTION__ << " " << __LINE__ << std::endl;
#else
#  define DBG2(x)
#endif

namespace tl
{
    namespace ndi
    {
        
        ReadAudio::ReadAudio(
            const std::string& fileName,
            NDIlib_recv_instance_t recv,
            const std::vector<file::MemoryRead>& memory,
            double videoRate,
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
            _info.dataType     = audio::DataType::S16;

            double fps = videoRate;
            double last = 3 * 60 * 60 * fps; // 3 hours time range
            _timeRange = otime::TimeRange(
                otime::RationalTime(0.0, fps).rescaled_to(_info.sampleRate),
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

        void ReadAudio::seek(const otime::RationalTime& time)
        {
            //std::cout << "audio seek: " << time << std::endl;
#ifndef USE_NDI
            if (_swrContext)
            {
                swr_init(_swrContext);
            }
#endif
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

        int ReadAudio::_decode(const otime::RationalTime& currentTime)
        {
            int out = 0;

            while(out == 0)
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
                
                const auto time = otime::RationalTime(
                    _timeRange.start_time().value() +
                    av_rescale_q(
                        audio_frame.timecode,
                        NDI_TIME_BASE_Q,
                        r),
                    _info.sampleRate);
                
                if (1) //time >= currentTime)
                {
                    DBG2( "audio time: " << time << " timecode="
                          << audio_frame.timecode
                          << " currentTime=" << currentTime );
                    DBG2( "nb_samples: " << audio_frame.no_samples );

                    auto tmp =
                        audio::Audio::create(_info, audio_frame.no_samples);
                    
                    // Allocate enough space for 16bpp interleaved buffer
                    NDIlib_audio_frame_interleaved_16s_t dst;
                    dst.reference_level = 0;
                    dst.p_data = (short*) tmp->getData();
                    
                    // Convert it
                    NDIlib_util_audio_to_interleaved_16s(&audio_frame, &dst);
                    

                    std::cerr << "---------------------" << std::endl;
                    for (int i = 0; i < 8; ++i)
                    {
                        std::cerr << dst.p_data[i] << std::endl;
                    }
                    
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
