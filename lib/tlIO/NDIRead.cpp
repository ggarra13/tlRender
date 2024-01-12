// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Gonzalo Garramuño
// All rights reserved.

#include <fstream>

#include <tlIO/NDIReadPrivate.h>

#include <tlCore/Assert.h>
#include <tlCore/LogSystem.h>
#include <tlCore/StringFormat.h>

#if 1
#define DBG(x)
#define DBG2(x) \
    std::cerr << x << " " << __FUNCTION__ << " " << __LINE__ << std::endl;
#else
#define DBG(x)                                                          \
    std::cerr << x << " " << __FUNCTION__ << " " << __LINE__ << std::endl;
#endif

namespace tl
{
    namespace ndi
    {
        int Options::ndiSource = -1;
        
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

            std::string sourceName;
            i = options.find("NDI/SourceName");
            if (i != options.end())
            {
                // @bug: SourceName is not being sent through options
                std::stringstream ss(i->second);
                ss >> sourceName;
            }
            else
            {
                std::ifstream s(path.get());
                std::getline(s, sourceName);
                s.close();
            }

            p.NDI_find = NDIlib_find_create_v2();
            if (!p.NDI_find)
                throw std::runtime_error("Could not create NDI find");

                
            uint32_t no_sources = 0;
            if (p.options.ndiSource < 0 || !p.sources)
            {
                // Get the updated list of sources
                while (!no_sources)
                {
                    p.sources = NDIlib_find_get_current_sources(p.NDI_find, &no_sources);
                }
            }
            
            for (int i = 0; i < no_sources; ++i)
            {
                if (p.sources[i].p_ndi_name == sourceName)
                {
                    p.options.ndiSource = i;
                    break;
                }
            }

            // We now have at least one source,
            // so we create a receiver to look at it.
            NDIlib_recv_create_v3_t recv_desc;
            recv_desc.color_format = NDIlib_recv_color_format_fastest;
            
            p.NDI_recv = NDIlib_recv_create(&recv_desc);
            if (!p.NDI_recv)
                throw std::runtime_error("Could not create NDI receiver");
            
            // Connect to our sources
            NDIlib_recv_connect(p.NDI_recv,
                                p.sources + p.options.ndiSource);
            

                
            // Get the name of the source for debugging purposes
            const std::string source = sourceName;
            NDIlib_tally_t tally_state;
            tally_state.on_program = true;
            tally_state.on_preview = false;
            
            /* Set tally */
            NDIlib_recv_set_tally(p.NDI_recv, &tally_state);

            
            // The descriptors
            p.decodeThread.running = true;
            p.decodeThread.thread = std::thread(
                [this, source]
                    {
                        TLRENDER_P();
                        
                        double fps = 24.0;
                        NDIlib_video_frame_t v;
                        NDIlib_audio_frame_t a;
                        NDIlib_frame_type_e type_e = NDIlib_frame_type_none;

                        bool once = true;
                        double ignore = 0.0;
                        double preroll = 1000.0;
                        while (type_e != NDIlib_frame_type_error &&
                               p.decodeThread.running)
                        {
                            type_e = NDIlib_recv_capture(p.NDI_recv, &v, &a, nullptr, 50);
                            if (type_e == NDIlib_frame_type_video)
                            {
                                if (!p.readVideo)
                                {
                                    p.videoThread.running = true;
                                    p.readVideo = std::make_shared<ReadVideo>(
                                        source, v, p.options);
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
                                    if (p.videoThread.currentTime <=
                                        p.audioThread.currentTime)
                                    {
                                        // We should not process the first frame or else
                                        // p.info.audio will not be set.
                                        _videoThread(v);
                                    }
                                }
                                
                                // Release this video frame
                                NDIlib_recv_free_video(p.NDI_recv, &v);
                            }
                            if (type_e == NDIlib_frame_type_audio)
                            {
                                if (!p.readAudio)
                                {
                                    p.readAudio = std::make_shared<ReadAudio>(
                                        source, a, fps, p.options);
                                    p.info.audio = p.readAudio->getInfo();
                                    p.info.audioTime = p.readAudio->getTimeRange();
                                    p.readAudio->start();
                                    p.audioThread.currentTime =
                                        p.info.audioTime.start_time();
                                    p.audioThread.logTimer = std::chrono::steady_clock::now();
                                }
                                
                                _audioThread(a);

                                
                                // Release this audio frame
                                NDIlib_recv_free_audio(p.NDI_recv, &a);
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

            p.videoThread.running = false;
            p.decodeThread.running = false;
            if (p.decodeThread.thread.joinable())
            {
                p.decodeThread.thread.join();
            }
            
            
            // Destroy the NDI receiver
            NDIlib_recv_destroy(p.NDI_recv);
            p.NDI_recv = nullptr;
            
            // Destroy the NDI finder.
            // We needed to have access to the pointers to p.sources[0]
            if(p.NDI_find)
                NDIlib_find_destroy(p.NDI_find);
            p.NDI_find = nullptr;
            
            // Not required, but nice
            NDIlib_destroy();
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
            while (p.videoThread.running)
            {
                // Check requests.
                std::list<std::shared_ptr<Private::InfoRequest> > infoRequests;
                std::shared_ptr<Private::VideoRequest> videoRequest;
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
                    const std::string cacheKey = io::Cache::getVideoKey(
                        _path.get(),
                        videoRequest->time,
                        videoRequest->options);
                    if (_cache->getVideo(cacheKey, videoData))
                    {
                        videoRequest->promise.set_value(videoData);
                        videoRequest.reset();
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
                        const std::string cacheKey = io::Cache::getVideoKey(
                            _path.get(),
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

        void Read::_audioThread(const NDIlib_audio_frame_t& a)
        {
            TLRENDER_P();
            // Check requests.
            size_t requestSampleCount = 0;
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
                    if (!p.audioMutex.requests.empty() &&
                        !p.audioMutex.currentRequest)
                    {
                        p.audioMutex.currentRequest = p.audioMutex.requests.front();
                        p.audioMutex.requests.pop_front();
                        requestSampleCount = p.audioMutex.currentRequest->timeRange.duration().rescaled_to(p.info.audio.sampleRate).value();
                    }
                }
            }

            // Check the cache.
            io::AudioData audioData;
            if (p.audioMutex.currentRequest && _cache)
            {
                const std::string cacheKey = io::Cache::getAudioKey(
                    _path.get(),
                    p.audioMutex.currentRequest->timeRange,
                    p.audioMutex.currentRequest->options);
                if (_cache->getAudio(cacheKey, audioData))
                {
                    p.audioMutex.currentRequest->promise.set_value(audioData);
                    p.audioMutex.currentRequest.reset();
                }
            }

            // No Seek.

            // Process.
            bool intersects = false;
            if (p.audioMutex.currentRequest)
            {
                intersects = p.audioMutex.currentRequest->timeRange.intersects(p.info.audioTime);
                if (intersects && p.readAudio->isValid())
                    p.readAudio->process(p.audioThread.currentTime, a);
                
                // Handle request.
                if (p.readAudio->getBufferSize() >=
                    p.audioMutex.currentRequest->timeRange.duration()
                    .rescaled_to(p.info.audio.sampleRate)
                    .value())
                {
                    io::AudioData audioData;
                    audioData.time = p.audioMutex.currentRequest->timeRange.start_time();
                    audioData.audio = audio::Audio::create(
                        p.info.audio, p.audioMutex.currentRequest->timeRange.duration().value());
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
                    p.audioMutex.currentRequest->promise.set_value(audioData);

                    p.audioThread.currentTime += p.audioMutex.currentRequest->timeRange.duration();
                    if (_cache)
                    {
                        const std::string cacheKey = io::Cache::getAudioKey(
                            _path.get(),
                            p.audioMutex.currentRequest->timeRange,
                            p.audioMutex.currentRequest->options);
                        _cache->addAudio(cacheKey, audioData);
                    }
                
                    p.audioMutex.currentRequest.reset();
                }
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
