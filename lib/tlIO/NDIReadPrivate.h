// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Gonzalo Garramuño
// All rights reserved.

#pragma once

#include <tlIO/NDI.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>

struct AVStream;    
} // extern "C"


#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace tl
{
    namespace ndi
    {
        struct AVIOBufferData
        {
            AVIOBufferData();
            AVIOBufferData(const uint8_t* p, size_t size);

            const uint8_t* p = nullptr;
            size_t size = 0;
            size_t offset = 0;
        };

        int avIOBufferRead(void* opaque, uint8_t* buf, int bufSize);
        int64_t avIOBufferSeek(void* opaque, int64_t offset, int whence);

        const size_t avIOContextBufferSize = 4096;
        
        struct Options
        {
            otime::RationalTime startTime = time::invalidTime;
            bool yuvToRGBConversion = false;
            audio::Info audioConvertInfo;
            size_t threadCount = 2;
            size_t requestTimeout = 5;
            size_t videoBufferSize = 4;
            otime::RationalTime audioBufferSize = otime::RationalTime(2.0, 1.0);
        };
        
        class ReadVideo
        {
        public:
            ReadVideo(
                const std::string& fileName,
                const std::vector<file::MemoryRead>& memory,
                const Options& options);

            ~ReadVideo();

            bool isValid() const;
            const image::Info& getInfo() const;
            const otime::TimeRange& getTimeRange() const;

            void start();
            // void seek(const otime::RationalTime&);
            bool process(const otime::RationalTime& currentTime);

            bool isBufferEmpty() const;
            std::shared_ptr<image::Image> popBuffer();

        private:
            int _decode(const otime::RationalTime& currentTime);
            void _copy(std::shared_ptr<image::Image>&);
            
            std::string _fileName;
            Options _options;
            image::Info _info;
            otime::TimeRange _timeRange = time::invalidTimeRange;
            std::list<std::shared_ptr<image::Image> > _buffer;

            // NDI structs
            NDIlib_find_instance_t pNDI_find;
            NDIlib_recv_instance_t pNDI_recv;
            uint32_t no_sources = 0;
            const NDIlib_source_t* p_sources = nullptr;
            int frame_rate_N = 30000, frame_rate_D = 1001;

            // FFmpeg conversion variables
            AVFrame* _avFrame = nullptr;
            AVFrame* _avFrame2 = nullptr;
            AVPixelFormat _avInputPixelFormat = AV_PIX_FMT_NONE;
            AVPixelFormat _avOutputPixelFormat = AV_PIX_FMT_NONE;
            SwsContext* _swsContext = nullptr;
        };

        class ReadAudio
        {
        public:
            ReadAudio(
                const std::string& fileName,
                const std::vector<file::MemoryRead>&,
                double videoRate,
                const Options&);

            ~ReadAudio();

            bool isValid() const;
            const audio::Info& getInfo() const;
            const otime::TimeRange& getTimeRange() const;
            const image::Tags& getTags() const;

            void start();
            // void seek(const otime::RationalTime&);
            bool process(
                const otime::RationalTime& currentTime,
                size_t sampleCount);

            size_t getBufferSize() const;
            void bufferCopy(uint8_t*, size_t sampleCount);

        private:
            int _decode(const otime::RationalTime& currentTime);

            std::string _fileName;
            Options _options;
            audio::Info _info;
            otime::TimeRange _timeRange = time::invalidTimeRange;
            std::list<std::shared_ptr<audio::Audio> > _buffer;
        };

        struct Read::Private
        {
            Options options;

            std::shared_ptr<ReadVideo> readVideo;
            std::shared_ptr<ReadAudio> readAudio;

            io::Info info;
            struct InfoRequest
            {
                std::promise<io::Info> promise;
            };

            struct VideoRequest
            {
                otime::RationalTime time = time::invalidTime;
                io::Options options;
                std::promise<io::VideoData> promise;
            };
            struct VideoMutex
            {
                std::list<std::shared_ptr<InfoRequest> > infoRequests;
                std::list<std::shared_ptr<VideoRequest> > videoRequests;
                //std::shared_ptr<VideoRequest> videoRequest;
                bool stopped = false;
                std::mutex mutex;
            };
            VideoMutex videoMutex;
            struct VideoThread
            {
                otime::RationalTime currentTime = time::invalidTime;
                std::chrono::steady_clock::time_point logTimer;
                std::condition_variable cv;
                std::thread thread;
                std::atomic<bool> running;
            };
            VideoThread videoThread;

            struct AudioRequest
            {
                otime::TimeRange timeRange = time::invalidTimeRange;
                io::Options options;
                std::promise<io::AudioData> promise;
            };
            struct AudioMutex
            {
                std::list<std::shared_ptr<AudioRequest> > requests;
                //std::shared_ptr<AudioRequest> currentRequest;
                bool stopped = false;
                std::mutex mutex;
            };
            AudioMutex audioMutex;
            struct AudioThread
            {
                otime::RationalTime currentTime = time::invalidTime;
                std::chrono::steady_clock::time_point logTimer;
                std::condition_variable cv;
                std::thread thread;
                std::atomic<bool> running;
            };
            AudioThread audioThread;
        };
    }
}
