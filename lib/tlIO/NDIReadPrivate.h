// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlIO/NDI.h>

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
            size_t threadCount = ffmpeg::threadCount;
            size_t requestTimeout = 5;
            size_t videoBufferSize = 4;
            otime::RationalTime audioBufferSize = otime::RationalTime(2.0, 1.0);
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
