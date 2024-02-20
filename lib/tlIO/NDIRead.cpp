// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Gonzalo Garramuño
// All rights reserved.

#include <fstream>

#include <tlIO/NDIReadPrivate.h>

#include <tlCore/Assert.h>
#include <tlCore/LogSystem.h>
#include <tlCore/StringFormat.h>


namespace tl
{
    namespace ndi
    {
        std::string Read::Private::sourceName;
        
        void Read::_init(
            const file::Path& path,
            const std::vector<file::MemoryRead>& memory,
            const io::Options& options,
            const std::shared_ptr<io::Cache>& cache,
            const std::weak_ptr<log::System>& logSystem)
        {
            IRead::_init(path, memory, options, cache, logSystem);

            TLRENDER_P();

            auto i = options.find("FFmpeg/YUVToRGBConversion");
            if (i != options.end())
            {
                std::stringstream ss(i->second);
                ss >> p.options.yuvToRGBConversion;
            }

            std::ifstream s(path.get());

            if (s.is_open())
            {
                nlohmann::json j;
                s >> j;
                p.options = j;
                s.close();
            }
                    
            const NDIlib_source_t* sources = nullptr;
            
            p.NDI_find = NDIlib_find_create_v2();
            if (!p.NDI_find)
                throw std::runtime_error("Could not create NDI find");
            
            using namespace std::chrono;
            for (const auto start = high_resolution_clock::now();
                 high_resolution_clock::now() - start < seconds(3);)
            {
                // Wait up till 1 second to check for new sources to be added or removed
                if (!NDIlib_find_wait_for_sources(p.NDI_find,
                                                  1000 /* milliseconds */)) {
                    break;
                }
            }
            
            uint32_t no_sources = 0;
            
            // Get the updated list of sources
            while (!no_sources)
            {
                sources = NDIlib_find_get_current_sources(p.NDI_find,
                                                          &no_sources);
            }
            
            int ndiSource = -1;
            for (int i = 0; i < no_sources; ++i)
            {
                if (sources[i].p_ndi_name == p.options.sourceName)
                {
                    ndiSource = i;
                    break;
                }
            }

            if (ndiSource < 0)
            {
                throw std::runtime_error("Could not find a valid source");
            }
            

            //
            const auto& NDIsource = sources[ndiSource];

            
            // We now have at least one source,
            // so we create a receiver to look at it.
            NDIlib_recv_create_v3_t recv_desc;
            recv_desc.color_format = NDIlib_recv_color_format_fastest;
            recv_desc.bandwidth = NDIlib_recv_bandwidth_highest;
            recv_desc.allow_video_fields = false;
            recv_desc.source_to_connect_to = NDIsource;
            
            p.NDI_recv = NDIlib_recv_create(&recv_desc);
            if (!p.NDI_recv)
                throw std::runtime_error("Could not create NDI receiver");
            
            // Get the name of the source for debugging purposes
            NDIlib_tally_t tally_state;
            tally_state.on_program = true;
            tally_state.on_preview = false;
            
            /* Set tally */
            NDIlib_recv_set_tally(p.NDI_recv, &tally_state);
            
            
            
            // The descriptors
            p.decodeThread.running = true;
            p.decodeThread.thread = std::thread(
                [this, NDIsource]
                    {
                        TLRENDER_P();
                        
                        double fps = 24.0;
                        NDIlib_video_frame_t v;
                        NDIlib_audio_frame_t a;
                        NDIlib_frame_type_e type_e = NDIlib_frame_type_none;

                        p.audioThread.currentTime =
                            otime::RationalTime(0.0, 48000.0);
                        p.videoThread.currentTime =
                            otime::RationalTime(0.0, fps);

                        while (type_e != NDIlib_frame_type_error &&
                               p.decodeThread.running)
                        {
                            if (p.readAudio)
                            {
                                type_e = NDIlib_recv_capture(
                                    p.NDI_recv, &v, nullptr, nullptr, 50);
                            }
                            else
                            {
                                type_e = NDIlib_recv_capture(
                                    p.NDI_recv, &v, &a, nullptr, 50);
                            }
                            if (type_e == NDIlib_frame_type_video)
                            {
                                if (!p.readVideo)
                                {
                                    p.videoThread.running = true;
                                    p.readVideo = std::make_shared<ReadVideo>(
                                        p.options.sourceName, v, p.options);
                                    const auto& videoInfo = p.readVideo->getInfo();
                                    if (videoInfo.isValid())
                                    {
                                        p.info.video.push_back(videoInfo);
                                        p.info.videoTime = p.readVideo->getTimeRange();
                                    }
                                    p.readVideo->start();
                                    p.videoThread.currentTime =
                                        p.info.videoTime.start_time();
                                    p.videoThread.logTimer = std::chrono::steady_clock::now();
                                    fps = p.info.videoTime.duration().rate();
                                }
                                else
                                {
                                    const auto audio = p.audioThread.currentTime;
                                    auto video = p.videoThread.currentTime;
                                    if (p.readAudio)
                                        video = video.rescaled_to(audio.rate());
                                    if ((p.readAudio && (video <= audio)) ||
                                        p.options.noAudio)
                                    {
                                        // We should not process the first
                                        // video frame until p.readAudio has
                                        // been created or p.info.audio will
                                        // not be set.
                                        _videoThread(v);
                                    }
                                }
                                
                                // Release this video frame
                                NDIlib_recv_free_video(p.NDI_recv, &v);
                            }
                            else if (type_e == NDIlib_frame_type_audio)
                            {
                                if (!p.options.noAudio)
                                {
                                    if (!p.readAudio)
                                    {
                                        // p.readAudio will release the audio
                                        // frame for us.
                                        p.readAudio =
                                            std::make_shared<ReadAudio>(
                                                p.options.sourceName, NDIsource,
                                                a, p.options);
                                        p.info.audio = p.readAudio->getInfo();
                                        p.info.audioTime = p.readAudio->getTimeRange();
                                        p.readAudio->start();
                                        p.audioThread.currentTime =
                                            p.info.audioTime.start_time();
                                        p.audioThread.logTimer = std::chrono::steady_clock::now();

                                        p.audioThread.running = true;
                                        p.audioThread.thread =
                                            std::thread(
                                                [this]
                                                    {
                                                        _audioThread();
                                                    });
                                    
                                    }
                                }
                                else
                                {
                                    p.audioThread.currentTime =
                                        p.videoThread.currentTime;
                                }
                            }
                        }
            
                        {
                            std::unique_lock<std::mutex> lock(p.videoMutex.mutex);
                            p.videoMutex.stopped = true;
                        }
                        _cancelVideoRequests();
                        {
                            std::unique_lock<std::mutex> lock(p.audioMutex.mutex);
                            p.audioMutex.stopped = true;
                        }
                        _cancelAudioRequests();
                    });
        }

        Read::Read() :
            _p(new Private)
        {}

        Read::~Read()
        {
            TLRENDER_P();

            p.audioThread.running = false;
            if (p.audioThread.thread.joinable())
            {
                p.audioThread.thread.join();
            }
            p.videoThread.running = false;
            p.decodeThread.running = false;
            if (p.decodeThread.thread.joinable())
            {
                p.decodeThread.thread.join();
            }
            
            // We destroy receiver (video)
            NDIlib_recv_destroy(p.NDI_recv);

            // We destroy the finder
            NDIlib_find_destroy(p.NDI_find);
        }

        std::shared_ptr<Read> Read::create(
            const file::Path& path,
            const io::Options& options,
            const std::shared_ptr<io::Cache>& cache,
            const std::weak_ptr<log::System>& logSystem)
        {
            auto out = std::shared_ptr<Read>(new Read);
            out->_init(path, {}, options, cache, logSystem);
            return out;
        }

        std::shared_ptr<Read> Read::create(
            const file::Path& path,
            const std::vector<file::MemoryRead>& memory,
            const io::Options& options,
            const std::shared_ptr<io::Cache>& cache,
            const std::weak_ptr<log::System>& logSystem)
        {
            auto out = std::shared_ptr<Read>(new Read);
            out->_init(path, memory, options, cache, logSystem);
            return out;
        }

        std::future<io::Info> Read::getInfo()
        {
            TLRENDER_P();
            auto request = std::make_shared<Private::InfoRequest>();
            auto future = request->promise.get_future();
            bool valid = false;
            {
                std::unique_lock<std::mutex> lock(p.videoMutex.mutex);
                if (!p.videoMutex.stopped)
                {
                    valid = true;
                    p.videoMutex.infoRequests.push_back(request);
                }
            }
            if (valid)
            {
                p.videoThread.cv.notify_one();
            }
            else
            {
                request->promise.set_value(io::Info());
            }
            return future;
        }

        std::future<io::VideoData> Read::readVideo(
            const otime::RationalTime& time,
            const io::Options& options)
        {
            TLRENDER_P();
            auto request = std::make_shared<Private::VideoRequest>();
            request->time = time;
            request->options = io::merge(options, _options);
            auto future = request->promise.get_future();
            bool valid = false;
            {
                std::unique_lock<std::mutex> lock(p.videoMutex.mutex);
                if (!p.videoMutex.stopped)
                {
                    valid = true;
                    p.videoMutex.videoRequests.push_back(request);
                }
            }
            if (valid)
            {
                p.videoThread.cv.notify_one();
            }
            else
            {
                request->promise.set_value(io::VideoData());
            }
            return future;
        }

        std::future<io::AudioData> Read::readAudio(
            const otime::TimeRange& timeRange,
            const io::Options& options)
        {
            TLRENDER_P();
            auto request = std::make_shared<Private::AudioRequest>();
            request->timeRange = timeRange;
            request->options = io::merge(options, _options);
            auto future = request->promise.get_future();
            bool valid = false;
            {
                std::unique_lock<std::mutex> lock(p.audioMutex.mutex);
                if (!p.audioMutex.stopped)
                {
                    valid = true;
                    p.audioMutex.requests.push_back(request);
                }
            }
            if (valid)
            {
                p.audioThread.cv.notify_one();
            }
            else
            {
                request->promise.set_value(io::AudioData());
            }
            return future;
        }

        void Read::cancelRequests()
        {
            _cancelVideoRequests();
            _cancelAudioRequests();
        }

        void Read::_videoThread(const NDIlib_video_frame_t& v)
        {
            TLRENDER_P();

            while(p.videoThread.running)
            {
                // Check requests.
                std::shared_ptr<Private::VideoRequest> videoRequest;
                std::list<std::shared_ptr<Private::InfoRequest> > infoRequests;
                {
                    std::unique_lock<std::mutex> lock(p.videoMutex.mutex);
                    if (p.videoThread.cv.wait_for(
                            lock,
                            std::chrono::milliseconds(p.options.requestTimeout),
                            [this]
                                {
                                    return
                                        !_p->videoMutex.infoRequests.empty() ||
                                        !_p->videoMutex.videoRequests.empty();
                                }))
                    {
                        infoRequests = std::move(p.videoMutex.infoRequests);
                        if (!p.videoMutex.videoRequests.empty())
                        {
                            videoRequest = p.videoMutex.videoRequests.front();
                            p.videoMutex.videoRequests.pop_front();
                        }
                    }
                }

                // Information requests.
                for (auto& request : infoRequests)
                {
                    request->promise.set_value(p.info);
                }

                // Check the cache.
                io::VideoData videoData;
                if (videoRequest && _cache)
                {
                    const std::string cacheKey = io::getCacheKey(
                        _path,
                        videoRequest->time,
                        videoRequest->options);
                    if (_cache->getVideo(cacheKey, videoData))
                    {
                        p.videoThread.currentTime = videoRequest->time;
                        videoRequest->promise.set_value(videoData);
                        videoRequest.reset();
                        return;
                    }
                }

                // No seeking allowed
                if (videoRequest &&
                    !time::compareExact(videoRequest->time, p.videoThread.currentTime))
                {
                    p.videoThread.currentTime = videoRequest->time;
                }

                // Process.
                while (videoRequest && p.readVideo->isBufferEmpty() &&
                       p.readVideo->isValid() &&
                       p.readVideo->process(p.videoThread.currentTime, v))
                    ;

                // Video request.
                if (videoRequest)
                {
                    io::VideoData data;
                    data.time = videoRequest->time;
                    if (!p.readVideo->isBufferEmpty())
                    {
                        data.image = p.readVideo->popBuffer();
                    }
                    videoRequest->promise.set_value(data);
                    
                    if (_cache)
                    {
                        const std::string cacheKey = io::getCacheKey(
                            _path,
                            videoRequest->time,
                            videoRequest->options);
                        _cache->addVideo(cacheKey, data);
                    }

                    p.videoThread.currentTime += otime::RationalTime(1.0, p.info.videoTime.duration().rate());
                    return;
                }

                // Logging.
                {
                    const auto now = std::chrono::steady_clock::now();
                    const std::chrono::duration<float> diff = now - p.videoThread.logTimer;
                    if (diff.count() > 10.F)
                    {
                        p.videoThread.logTimer = now;
                        if (auto logSystem = _logSystem.lock())
                        {
                            const std::string id = string::Format("tl::io::ndi::Read {0}").arg(this);
                            size_t requestsSize = 0;
                            {
                                std::unique_lock<std::mutex> lock(p.videoMutex.mutex);
                                requestsSize = p.videoMutex.videoRequests.size();
                            }
                            logSystem->print(id, string::Format(
                                                 "\n"
                                                 "    Path: {0}\n"
                                                 "    Video requests: {1}").
                                             arg(_path.get()).
                                             arg(requestsSize));
                        }
                    }
                }
            }
        }

        void Read::_audioThread()
        {
            TLRENDER_P();
            while (p.audioThread.running)
            {
                std::shared_ptr<Private::AudioRequest> request;
                size_t requestSampleCount = 0;
                // Check requests.
                {
                    std::unique_lock<std::mutex> lock(p.audioMutex.mutex);
                    if (p.audioThread.cv.wait_for(
                            lock,
                            std::chrono::milliseconds(p.options.requestTimeout),
                            [this]
                                {
                                    return !_p->audioMutex.requests.empty();
                                }))
                    {
                        if (!p.audioMutex.requests.empty())
                        {
                            request = p.audioMutex.requests.front();
                            p.audioMutex.requests.pop_front();
                        }
                    }
                }

                // Check the cache.
                io::AudioData audioData;
                if (request && _cache)
                {
                    const std::string cacheKey = io::getCacheKey(
                        _path,
                        request->timeRange,
                        request->options);
                    if (_cache->getAudio(cacheKey, audioData))
                    {
                        request->promise.set_value(audioData);
                        request.reset();
                    }
                }

                // No Seek.

                // Process.
                bool intersects = false;
                if (request)
                {
                    intersects = request->timeRange.intersects(p.info.audioTime);
                }
                while (
                    request &&
                    intersects &&
                    p.readAudio->getBufferSize() < request->timeRange.duration().rescaled_to(p.info.audio.sampleRate).value() &&
                    p.readAudio->isValid() &&
                    p.readAudio->process(
                        p.audioThread.currentTime,
                        requestSampleCount ?
                        requestSampleCount :
                        p.options.audioBufferSize.rescaled_to(p.info.audio.sampleRate).value()))
                    ;
                
                // Handle request.
                if (request)
                {
                    io::AudioData audioData;
                    audioData.time = request->timeRange.start_time();
                    audioData.audio = audio::Audio::create(
                        p.info.audio, request->timeRange.duration().value());
                    audioData.audio->zero();
                    if (intersects)
                    {
                        size_t offset = 0;
                        if (audioData.time < p.info.audioTime.start_time())
                        {
                            offset = (p.info.audioTime.start_time() - audioData.time).value();
                        }
                        p.readAudio->bufferCopy(
                            audioData.audio->getData() + offset * p.info.audio.getByteCount(),
                            audioData.audio->getSampleCount() - offset);
                    }
                    request->promise.set_value(audioData);

                    if (_cache)
                    {
                        const std::string cacheKey = io::getCacheKey(
                            _path,
                            request->timeRange,
                            request->options);
                        _cache->addAudio(cacheKey, audioData);
                    }
                    
                    p.audioThread.currentTime += request->timeRange.duration();
                }


                    
                // Logging.
                {
                    const auto now = std::chrono::steady_clock::now();
                    const std::chrono::duration<float> diff = now - p.audioThread.logTimer;
                    if (diff.count() > 10.F)
                    {
                        p.audioThread.logTimer = now;
                        if (auto logSystem = _logSystem.lock())
                        {
                            const std::string id = string::Format("tl::io::ndi::Read {0}").arg(this);
                            size_t requestsSize = 0;
                            {
                                std::unique_lock<std::mutex> lock(p.audioMutex.mutex);
                                requestsSize = p.audioMutex.requests.size();
                            }
                            logSystem->print(id, string::Format(
                                                 "\n"
                                                 "    Path: {0}\n"
                                                 "    Audio requests: {1}").
                                             arg(_path.get()).
                                             arg(requestsSize));
                        }
                    }
                }
            }
        }

        void Read::_cancelVideoRequests()
        {
            TLRENDER_P();
            std::list<std::shared_ptr<Private::InfoRequest> > infoRequests;
            std::list<std::shared_ptr<Private::VideoRequest> > videoRequests;
            {
                std::unique_lock<std::mutex> lock(p.videoMutex.mutex);
                infoRequests = std::move(p.videoMutex.infoRequests);
                videoRequests = std::move(p.videoMutex.videoRequests);
            }
            for (auto& request : infoRequests)
            {
                request->promise.set_value(io::Info());
            }
            for (auto& request : videoRequests)
            {
                request->promise.set_value(io::VideoData());
            }
        }

        void Read::_cancelAudioRequests()
        {
            TLRENDER_P();
            std::list<std::shared_ptr<Private::AudioRequest> > requests;
            {
                std::unique_lock<std::mutex> lock(p.audioMutex.mutex);
                requests = std::move(p.audioMutex.requests);
            }
            for (auto& request : requests)
            {
                request->promise.set_value(io::AudioData());
            }
        }
    }
}
