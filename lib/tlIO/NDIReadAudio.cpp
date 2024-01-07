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
            NDIlib_recv_instance_t recv,
            const std::vector<file::MemoryRead>& memory,
            double videoRate,
            const Options& options) :
            pNDI_recv(recv),
            _fileName(fileName),
            _options(options)
        {
            DBG("");

            
            NDIlib_audio_frame_v2_t audio_frame;
            NDIlib_frame_type_e type_e = NDIlib_frame_type_none;
            
            while(type_e != NDIlib_frame_type_audio)
            {
                type_e = NDIlib_recv_capture_v2(
                    pNDI_recv, nullptr, &audio_frame, nullptr, 5000);
            }

            DBG("");
            _info.channelCount = audio_frame.no_channels;
            _info.sampleRate   = audio_frame.sample_rate;
            _info.dataType     = audio::DataType::F32;

            double fps = videoRate;
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
            int64_t timestamp = audio_frame.timecode - audio_frame.timestamp;
            
            DBG2("AUDIO timecode " << timestamp << " timestamp "
                 << audio_frame.timestamp
                 << " currentTime=" << currentTime);
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
                
            if (1)
            {
                DBG2( "audio time: " << time << " currentTime=" << currentTime );
                DBG2( "nb_samples: " << audio_frame.no_samples );
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
