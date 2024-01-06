// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Gonzalo Garramuño
// All rights reserved.

#include <tlIO/NDIReadPrivate.h>

#include <tlCore/StringFormat.h>

#if 1
#  define DBG(x) \
    std::cerr << x << " " << __FUNCTION__ << " " << __LINE__ << std::endl;
#else
#  define DBG(x)
#endif

namespace tl
{
    namespace ndi
    {
        AVSampleFormat fromAudioType(audio::DataType value)
        {
            AVSampleFormat out = AV_SAMPLE_FMT_NONE;
            switch (value)
            {
            case audio::DataType::S16: out = AV_SAMPLE_FMT_S16; break;
            case audio::DataType::S32: out = AV_SAMPLE_FMT_S32; break;
            case audio::DataType::F32: out = AV_SAMPLE_FMT_FLT; break;
            case audio::DataType::F64: out = AV_SAMPLE_FMT_DBL; break;
            default: break;
            }
            return out;
        }

        
        ReadAudio::ReadAudio(
            const std::string& fileName,
            const std::vector<file::MemoryRead>& memory,
            double videoRate,
            const Options& options) :
            _fileName(fileName),
            _options(options)
        {
            DBG("");
            pNDI_find = NDIlib_find_create_v2();
            if (!pNDI_find)
                throw std::runtime_error("Could not create NDI find");
            DBG("");
            uint32_t no_sources = 0;
            const NDIlib_source_t* p_sources = NULL;
            while (!no_sources)
            {
                // Wait until the sources on the network have changed
                NDIlib_find_wait_for_sources(pNDI_find, 1000/* One second */);
                p_sources = NDIlib_find_get_current_sources(pNDI_find,
                                                            &no_sources);
            }

            DBG("");
            pNDI_recv = NDIlib_recv_create_v3();
            if (!pNDI_recv)
                throw std::runtime_error("Could not create NDI receiver");
    
            DBG("");
            // Connect to our sources
            NDIlib_recv_connect(pNDI_recv, p_sources + 0);
            
            // Destroy the NDI finder.
            // We needed to have access to the pointers to p_sources[0]
            NDIlib_find_destroy(pNDI_find);
            pNDI_find = nullptr;

            DBG("");
            NDIlib_video_frame_v2_t video_frame;
            NDIlib_audio_frame_v2_t audio_frame;
            NDIlib_frame_type_e type_e = NDIlib_frame_type_none;


            int frame_rate_N = 3000, frame_rate_D = 1001;
            
            while(type_e != NDIlib_frame_type_audio)
            {
                type_e = NDIlib_recv_capture_v2(
                    pNDI_recv, &video_frame, &audio_frame, nullptr, 5000);
                if (type_e == NDIlib_frame_type_video)
                {
                    frame_rate_N = video_frame.frame_rate_N;
                    frame_rate_D = video_frame.frame_rate_D;
                }
                
            }

            DBG("");
            _info.channelCount = audio_frame.no_channels;
            _info.sampleRate   = audio_frame.sample_rate;
            _info.dataType     = audio::DataType::F32;

            double fps =
                frame_rate_N / static_cast<double>(frame_rate_D);
            double last = 3 * 60 * 60 * fps; // 3 hours time range
            _timeRange = otime::TimeRange(
                otime::RationalTime(0.0, fps).rescaled_to(_info.sampleRate),
                otime::RationalTime(last, fps).rescaled_to(_info.sampleRate));
            DBG("");

        }

        ReadAudio::~ReadAudio()
        {
            DBG("");
        }
        
        bool ReadAudio::isValid() const
        {
            return true;
        }

        const audio::Info& ReadAudio::getInfo() const
        {
            DBG("");
            return _info;
        }

        const otime::TimeRange& ReadAudio::getTimeRange() const
        {
            DBG("");
            return _timeRange;
        }

        void ReadAudio::start()
        {
        }

        void ReadAudio::seek(const otime::RationalTime& time)
        {
            _buffer.clear();
        }

        bool ReadAudio::process(
            const otime::RationalTime& currentTime,
            size_t sampleCount)
        {
            DBG("");
            bool out = false;
            const size_t bufferSampleCount = audio::getSampleCount(_buffer);
            if (bufferSampleCount < sampleCount)
            {
            DBG("");
                _decode(currentTime);
            DBG("");
            }
            else
            {
            DBG("");
                out = true;
            }
            DBG("out=" << out);
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
            NDIlib_audio_frame_v2_t audio_frame;
            NDIlib_frame_type_e type_e = NDIlib_frame_type_none;
            while(type_e != NDIlib_frame_type_audio)
            {
                type_e = NDIlib_recv_capture_v2(
                    pNDI_recv, nullptr, &audio_frame, nullptr, 5000);
            }
            
            DBG("");
            const int64_t timestamp = audio_frame.timecode;
            
            DBG("timestamp " << timestamp);
            AVRational r;
            r.num = 1;
            r.den = _info.sampleRate;
            AVRational time_base;
            r.num = _info.sampleRate;
            r.den = 1;
            const auto time = otime::RationalTime(
                _timeRange.start_time().value() +
                av_rescale_q(
                    timestamp,
                    time_base,
                    r),
                _info.sampleRate);
                
            if (1) //time >= currentTime)
            {
                DBG( "audio time: " << time << " currentTime=" << currentTime );
                DBG( "nb_samples: " << audio_frame.no_samples );
                // Allocate enough space for 16bpp interleaved buffer
				NDIlib_audio_frame_interleaved_16s_t audio_frame_16bpp_interleaved;
				audio_frame_16bpp_interleaved.reference_level = 20;	// We are going to have 20dB of headroom
                audio_frame_16bpp_interleaved.p_data =
                    new short[audio_frame.no_samples * audio_frame.no_channels];

                // Convert it
				NDIlib_util_audio_to_interleaved_16s_v2(&audio_frame, &audio_frame_16bpp_interleaved);

				// Feel free to do something with the interleaved audio data here
                auto tmp = audio::Audio::create(_info, audio_frame.no_samples);
                memcpy(tmp->getData(), audio_frame_16bpp_interleaved.p_data,
                       tmp->getByteCount());

				// Free the original buffer
				NDIlib_recv_free_audio_v2(pNDI_recv, &audio_frame);

				// Free the interleaved audio data
				delete[] audio_frame_16bpp_interleaved.p_data;
                
                _buffer.push_back(tmp);
                out = 1;
            }
            return out;
        }
    }
}
