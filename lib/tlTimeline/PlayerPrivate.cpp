// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#include <tlTimeline/PlayerPrivate.h>

#include <tlTimeline/Util.h>

#include <tlCore/StringFormat.h>

namespace tl
{
    namespace timeline
    {
        namespace
        {
            inline float fadeValue(double sample, double in, double out)
            {
                return (sample - in) / (out - in);
            }
        }
        
        otime::RationalTime Player::Private::loopPlayback(const otime::RationalTime& time)
        {
            otime::RationalTime out = time;

            const auto& range = inOutRange->get();
            switch (loop->get())
            {
            case Loop::Loop:
            {
                bool looped = false;
                out = timeline::loop(out, range, &looped);
                if (looped)
                {
                    {
                        std::unique_lock<std::mutex> lock(mutex.mutex);
                        mutex.playbackStartTime = out;
                        mutex.playbackStartTimer = std::chrono::steady_clock::now();
                    }
                    resetAudioTime();
                }
                break;
            }
            case Loop::Once:
            {
                const auto playbackValue = playback->get();
                if (out < range.start_time() && Playback::Reverse == playbackValue)
                {
                    out = range.start_time();
                    playback->setIfChanged(Playback::Stop);
                    {
                        std::unique_lock<std::mutex> lock(mutex.mutex);
                        mutex.playback = Playback::Stop;
                        mutex.clearRequests = true;
                    }
                }
                else if (out > range.end_time_inclusive() && Playback::Forward == playbackValue)
                {
                    out = range.end_time_inclusive();
                    playback->setIfChanged(Playback::Stop);
                    {
                        std::unique_lock<std::mutex> lock(mutex.mutex);
                        mutex.playback = Playback::Stop;
                        mutex.clearRequests = true;
                    }
                }
                break;
            }
            case Loop::PingPong:
            {
                const auto playbackValue = playback->get();
                if (out < range.start_time() && Playback::Reverse == playbackValue)
                {
                    out = range.start_time();
                    playback->setIfChanged(Playback::Forward);
                    {
                        std::unique_lock<std::mutex> lock(mutex.mutex);
                        mutex.playback = Playback::Forward;
                        mutex.playbackStartTime = out;
                        mutex.playbackStartTimer = std::chrono::steady_clock::now();
                        mutex.currentTime = currentTime->get();
                        mutex.clearRequests = true;
                        mutex.cacheDirection = CacheDirection::Forward;
                    }
                    resetAudioTime();
                }
                else if (out > range.end_time_inclusive() && Playback::Forward == playbackValue)
                {
                    out = range.end_time_inclusive();
                    playback->setIfChanged(Playback::Reverse);
                    {
                        std::unique_lock<std::mutex> lock(mutex.mutex);
                        mutex.playback = Playback::Reverse;
                        mutex.playbackStartTime = out;
                        mutex.playbackStartTimer = std::chrono::steady_clock::now();
                        mutex.currentTime = currentTime->get();
                        mutex.clearRequests = true;
                        mutex.cacheDirection = CacheDirection::Reverse;
                    }
                    resetAudioTime();
                }
                break;
            }
            default: break;
            }

            return out;
        }

        void Player::Private::clearRequests()
        {
            std::vector<std::vector<uint64_t> > ids(1 + thread.compare.size());
            for (const auto& i : thread.videoDataRequests)
            {
                for (size_t j = 0; j < i.second.size() && j < ids.size(); ++j)
                {
                    ids[j].push_back(i.second[j].id);
                }
            }
            for (const auto& i : thread.audioDataRequests)
            {
                ids[0].push_back(i.second.id);
            }
            timeline->cancelRequests(ids[0]);
            for (size_t i = 0; i < thread.compare.size(); ++i)
            {
                thread.compare[i]->cancelRequests(ids[i + 1]);
            }
            thread.videoDataRequests.clear();
            thread.audioDataRequests.clear();
        }

        void Player::Private::clearCache()
        {
            thread.videoDataCache.clear();
            {
                std::unique_lock<std::mutex> lock(mutex.mutex);
                mutex.cacheInfo = PlayerCacheInfo();
            }
            {
                std::unique_lock<std::mutex> lock(audioMutex.mutex);
                audioMutex.audioDataCache.clear();
            }
        }
        
        void Player::Private::reverseRequests(const otime::RationalTime& start,
                                              const otime::RationalTime& end,
                                              const otime::RationalTime& inc)
        {
            const otime::TimeRange& timeRange = timeline->getTimeRange();
            for (auto time = start; time >= end; time -= inc)
            {
                const auto i = thread.videoDataCache.find(time);
                if (i == thread.videoDataCache.end())
                {
                    const auto j = thread.videoDataRequests.find(time);
                    if (j == thread.videoDataRequests.end())
                    {
                        // std::cerr << thread.cacheDirection
                        //           << "\t\tBACK video request: "
                        //           << time << std::endl;
                        auto& request = thread.videoDataRequests[time];
                        request.clear();
                        io::Options ioOptions2 = thread.ioOptions;
                        ioOptions2["Layer"] = string::Format("{0}").arg(thread.videoLayer);
                        request.push_back(timeline->getVideo(time, ioOptions2));
                        for (size_t i = 0; i < thread.compare.size(); ++i)
                        {
                            const otime::RationalTime time2 = timeline::getCompareTime(
                                time,
                                timeRange,
                                thread.compare[i]->getTimeRange(),
                                thread.compareTime);
                            ioOptions2["Layer"] = string::Format("{0}").
                                                  arg(i < thread.compareVideoLayers.size() ?
                                                      thread.compareVideoLayers[i] :
                                                      thread.videoLayer);
                            request.push_back(thread.compare[i]->getVideo(time2, ioOptions2));
                        }
                    }
                }
            }
        }

        void Player::Private::forwardRequests(const otime::RationalTime& start,
                                              const otime::RationalTime& end,
                                              const otime::RationalTime& inc,
                                              const bool clearFrame)
        {
            const otime::TimeRange& timeRange = timeline->getTimeRange();
            for (otime::RationalTime time = start; time <= end; time += inc)
            {
                const auto i = thread.videoDataCache.find(time);
                if (i == thread.videoDataCache.end())
                {
                    const auto j = thread.videoDataRequests.find(time);
                    if (j == thread.videoDataRequests.end())
                    {
                        auto& request = thread.videoDataRequests[time];
                        request.clear();
                        io::Options ioOptions2 = thread.ioOptions;
                        ioOptions2["Layer"] = string::Format("{0}").arg(thread.videoLayer);
                        if (clearFrame)
                            ioOptions2["ClearFrame"] = "1";
                        request.push_back(timeline->getVideo(time, ioOptions2));
                        for (size_t i = 0; i < thread.compare.size(); ++i)
                        {
                            const otime::RationalTime time2 = timeline::getCompareTime(
                                time,
                                timeRange,
                                thread.compare[i]->getTimeRange(),
                                thread.compareTime);
                            ioOptions2["Layer"] = string::Format("{0}").
                                                  arg(i < thread.compareVideoLayers.size() ?
                                                      thread.compareVideoLayers[i] :
                                                      thread.videoLayer);
                            request.push_back(thread.compare[i]->getVideo(time2, ioOptions2));
                        }
                    }
                }
            }
        }

        void Player::Private::finishedVideoRequests()
        {
            // Check for finished video.
            auto videoDataRequestsIt = thread.videoDataRequests.begin();
            while (videoDataRequestsIt != thread.videoDataRequests.end())
            {
                bool ready = true;
                for (auto videoDataRequestIt = videoDataRequestsIt->second.begin();
                    videoDataRequestIt != videoDataRequestsIt->second.end();
                    ++videoDataRequestIt)
                {
                    ready &= videoDataRequestIt->future.valid() &&
                        videoDataRequestIt->future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                }
                if (ready)
                {
                    const otime::RationalTime time = videoDataRequestsIt->first;
                    auto& videoDataCache = thread.videoDataCache[time];
                    videoDataCache.clear();
                    for (auto videoDataRequestIt = videoDataRequestsIt->second.begin();
                        videoDataRequestIt != videoDataRequestsIt->second.end();
                        ++videoDataRequestIt)
                    {
                        auto videoData = videoDataRequestIt->future.get();
                        videoData.time = time;
                        videoDataCache.push_back(videoData);
                    }
                    videoDataRequestsIt = thread.videoDataRequests.erase(videoDataRequestsIt);
                }
                else
                {
                    ++videoDataRequestsIt;
                }
            }
        }
        
        void Player::Private::cacheUpdate()
        {
            // Get the video ranges to be cached.
            const otime::TimeRange& timeRange = timeline->getTimeRange();
            const otime::RationalTime readAheadDivided(
                thread.cacheOptions.readAhead.value() / static_cast<double>(1 + thread.compare.size()),
                thread.cacheOptions.readAhead.rate());
            const otime::RationalTime readAheadRescaled = readAheadDivided.
                rescaled_to(timeRange.duration().rate()).
                floor();
            const otime::RationalTime readBehindDivided(
                thread.cacheOptions.readBehind.value() / static_cast<double>(1 + thread.compare.size()),
                thread.cacheOptions.readBehind.rate());
            const otime::RationalTime readBehindRescaled = readBehindDivided.
                rescaled_to(timeRange.duration().rate()).
                floor();
            otime::TimeRange videoRange = time::invalidTimeRange;
            switch (thread.cacheDirection)
            {
            case CacheDirection::Forward:
                videoRange = otime::TimeRange::range_from_start_end_time_inclusive(
                    thread.currentTime - readBehindRescaled,
                    thread.currentTime + readAheadRescaled);
                break;
            case CacheDirection::Reverse:
                videoRange = otime::TimeRange::range_from_start_end_time_inclusive(
                    thread.currentTime - readAheadRescaled,
                    thread.currentTime + readBehindRescaled);
                break;
            default: break;
            }
            // std::cout << "------------- " << thread.currentTime << " "
            //           << thread.cacheDirection << std::endl;
            // std::cout << "in out range: " << thread.inOutRange << std::endl;
            // std::cout << "video range: " << videoRange << std::endl;
            auto videoRanges = timeline::loopCache(
                videoRange,
                thread.inOutRange,
                thread.cacheDirection);
            videoRanges.insert(
                videoRanges.begin(),
                otime::TimeRange(
                    thread.currentTime,
                    otime::RationalTime(1.0, thread.currentTime.rate())));

            //! If we are at the start either playing backwards or stopping,
            //! we need to loop the cache read behind to the end (for looping).
            if (mutex.playback != Playback::Forward &&
                loop->get() == Loop::Loop &&
                videoRanges[1].start_time() == thread.inOutRange.start_time())
            {
                const auto& end = thread.inOutRange.end_time_inclusive();
                const auto& start = end - readBehindRescaled;
                videoRange =
                    otime::TimeRange::range_from_start_end_time_inclusive(
                        start, end);
                videoRanges.push_back(videoRange);
            }
            
            // for (const auto& i : videoRanges)
            // {
            //    std::cout << "\tvideo ranges: " << i << std::endl;
            // }

            // Get the audio ranges to be cached.
            const otime::RationalTime audioOffsetTime = otime::RationalTime(thread.audioOffset, 1.0).
                rescaled_to(timeRange.duration().rate());
            //std::cout << "audio offset: " << audioOffsetTime << std::endl;
            const otime::RationalTime audioOffsetAhead = otime::RationalTime(
                audioOffsetTime.value() < 0.0 ?
                    -audioOffsetTime :
                    otime::RationalTime(0.0, timeRange.duration().rate())).
                round();
            const otime::RationalTime audioOffsetBehind = otime::RationalTime(
                audioOffsetTime.value() > 0.0 ?
                    audioOffsetTime :
                    otime::RationalTime(0.0, timeRange.duration().rate())).
                round();
            //std::cout << "audio offset ahead: " << audioOffsetAhead << std::endl;
            //std::cout << "audio offset behind: " << audioOffsetBehind << std::endl;
            otime::TimeRange audioRange = time::invalidTimeRange;
            switch (thread.cacheDirection)
            {
            case CacheDirection::Forward:
                audioRange = otime::TimeRange::range_from_start_end_time_inclusive(
                    thread.currentTime - readBehindRescaled - audioOffsetBehind,
                    thread.currentTime + readAheadRescaled + audioOffsetAhead);
                break;
            case CacheDirection::Reverse:
                audioRange = otime::TimeRange::range_from_start_end_time_inclusive(
                    thread.currentTime - readAheadRescaled - audioOffsetAhead,
                    thread.currentTime + readBehindRescaled + audioOffsetBehind);
                break;
            default: break;
            }
            //std::cout << "audio range: " << audioRange << std::endl;
            const otime::TimeRange inOutAudioRange = otime::TimeRange::range_from_start_end_time_inclusive(
                thread.inOutRange.start_time() - audioOffsetBehind,
                thread.inOutRange.end_time_inclusive() + audioOffsetAhead).
                    clamped(timeRange);
            //std::cout << "in out audio range: " << inOutAudioRange << std::endl;
            const auto audioRanges = timeline::loopCache(
                audioRange,
                inOutAudioRange,
                thread.cacheDirection);

            // Remove old video from the cache.
            auto videoCacheIt = thread.videoDataCache.begin();
            while (videoCacheIt != thread.videoDataCache.end())
            {
                const otime::RationalTime t = videoCacheIt->first;
                const auto j = std::find_if(
                    videoRanges.begin(),
                    videoRanges.end(),
                    [t](const otime::TimeRange& value)
                    {
                        return value.contains(t);
                    });
                if (j == videoRanges.end())
                {
                    videoCacheIt = thread.videoDataCache.erase(videoCacheIt);
                }
                else
                {
                    ++videoCacheIt;
                }
            }

            // Remove old audio from the cache.
            {
                std::unique_lock<std::mutex> lock(audioMutex.mutex);
                auto audioCacheIt = audioMutex.audioDataCache.begin();
                while (audioCacheIt != audioMutex.audioDataCache.end())
                {
                    const otime::TimeRange cacheRange(
                        otime::RationalTime(
                            timeRange.start_time().rescaled_to(1.0).value() +
                            audioCacheIt->first,
                            1.0),
                        otime::RationalTime(1.0, 1.0));
                    const auto j = std::find_if(
                        audioRanges.begin(),
                        audioRanges.end(),
                        [cacheRange](const otime::TimeRange& value)
                        {
                            return cacheRange.intersects(value);
                        });
                    if (j == audioRanges.end())
                    {
                        audioCacheIt = audioMutex.audioDataCache.erase(audioCacheIt);
                    }
                    else
                    {
                        ++audioCacheIt;
                    }
                }
            }

            // Get uncached video.
            if (!ioInfo.video.empty())
            {
                for (const auto& range : videoRanges)
                {
                    switch (thread.cacheDirection)
                    {
                    case CacheDirection::Forward:
                    {
                        // If we are stopped, and we are looping, we have to
                        // check the last video range we added at the end of
                        // the timeline to read it backwards
                        if (mutex.playback == Playback::Stop &&
                            loop->get() == Loop::Loop &&
                            range.start_time() != thread.inOutRange.start_time() &&
                            range.end_time_inclusive() == thread.inOutRange.end_time_inclusive())
                        {
                            const auto start = range.end_time_inclusive();
                            const auto end = range.start_time();
                            const auto inc = otime::RationalTime(1.0, range.duration().rate());
                            reverseRequests(start, end, inc);
                        }
                        else
                        {
                            const otime::RationalTime start = range.start_time();
                            const otime::RationalTime end = range.end_time_inclusive();
                            const otime::RationalTime inc = otime::RationalTime(1.0, range.duration().rate());
                            forwardRequests(start, end, inc);
                        }
                        break;
                    }
                    case CacheDirection::Reverse:
                    {
                        const auto start = range.end_time_inclusive();
                        const auto end = range.start_time();
                        const auto inc = otime::RationalTime(1.0, range.duration().rate());
                        reverseRequests(start, end, inc);
                        break;
                    }
                    default: break;
                    }
                }
            }

            // Get uncached audio.
            if (ioInfo.audio.isValid())
            {
                std::set<int64_t> seconds;
                for (const auto& range : audioRanges)
                {
                    const int64_t start = range.start_time().rescaled_to(1.0).value() -
                        timeRange.start_time().rescaled_to(1.0).value();
                    const int64_t end = start + range.duration().rescaled_to(1.0).value();
                    for (int64_t time = start; time <= end; ++time)
                    {
                        seconds.insert(time);
                    }
                }
                std::map<int64_t, double> requests;
                {
                    std::unique_lock<std::mutex> lock(audioMutex.mutex);
                    for (int64_t s : seconds)
                    {
                        const auto i = audioMutex.audioDataCache.find(s);
                        if (i == audioMutex.audioDataCache.end())
                        {
                            const auto j = thread.audioDataRequests.find(s);
                            if (j == thread.audioDataRequests.end())
                            {
                                requests[s] = timeRange.start_time().rescaled_to(1.0).value() + s;
                            }
                        }
                    }
                }
                switch (thread.cacheDirection)
                {
                case CacheDirection::Forward:
                    for (auto i = requests.begin(); i != requests.end(); ++i)
                    {
                        thread.audioDataRequests[i->first] = timeline->getAudio(i->second, thread.ioOptions);
                    }
                    break;
                case CacheDirection::Reverse:
                    for (auto i = requests.rbegin(); i != requests.rend(); ++i)
                    {
                        thread.audioDataRequests[i->first] = timeline->getAudio(i->second, thread.ioOptions);
                    }
                    break;
                default: break;
                }
                /*if (!requests.empty())
                {
                    std::cout << "audio range: " << audioRange << std::endl;
                    std::cout << "audio request:";
                    for (auto i : requests)
                    {
                        std::cout << " " << i;
                    }
                    std::cout << std::endl;
                }*/
            }

            finishedVideoRequests();

            // Check for finished audio.
            auto audioDataRequestsIt = thread.audioDataRequests.begin();
            while (audioDataRequestsIt != thread.audioDataRequests.end())
            {
                if (audioDataRequestsIt->second.future.valid() &&
                    audioDataRequestsIt->second.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                {
                    auto audioData = audioDataRequestsIt->second.future.get();
                    audioData.seconds = audioDataRequestsIt->first;
                    {
                        std::unique_lock<std::mutex> lock(audioMutex.mutex);
                        audioMutex.audioDataCache[audioDataRequestsIt->first] = audioData;
                    }
                    audioDataRequestsIt = thread.audioDataRequests.erase(audioDataRequestsIt);
                }
                else
                {
                    ++audioDataRequestsIt;
                }
            }

            // Update cached frames.
            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<float> diff = now - thread.cacheTimer;
            if (diff.count() > .5F)
            {
                thread.cacheTimer = now;
                std::vector<otime::RationalTime> cachedVideoFrames;
                for (const auto& i : thread.videoDataCache)
                {
                    cachedVideoFrames.push_back(i.first);
                }
                const float cachedVideoPercentage = cachedVideoFrames.size() /
                    static_cast<float>(readAheadDivided.rescaled_to(timeRange.duration().rate()).value() +
                        readBehindDivided.rescaled_to(timeRange.duration().rate()).value()) *
                    100.F;
                std::vector<otime::RationalTime> cachedAudioFrames;
                {
                    std::unique_lock<std::mutex> lock(audioMutex.mutex);
                    for (const auto& i : audioMutex.audioDataCache)
                    {
                        cachedAudioFrames.push_back(otime::RationalTime(
                            timeRange.start_time().rescaled_to(1.0).value() + i.first,
                            1.0));
                    }
                }
                auto cachedVideoRanges = toRanges(cachedVideoFrames);
                auto cachedAudioRanges = toRanges(cachedAudioFrames);
                for (auto& i : cachedAudioRanges)
                {
                    i = otime::TimeRange(
                        i.start_time().rescaled_to(timeRange.duration().rate()).floor(),
                        i.duration().rescaled_to(timeRange.duration().rate()).ceil());
                }
                float cachedAudioPercentage = 0.F;
                {
                    std::unique_lock<std::mutex> lock(mutex.mutex);
                    mutex.cacheInfo.videoPercentage = cachedVideoPercentage;
                    mutex.cacheInfo.videoFrames = cachedVideoRanges;
                    mutex.cacheInfo.audioFrames = cachedAudioRanges;
                }
            }
        }

        void Player::Private::resetAudioTime()
        {
            {
                std::unique_lock<std::mutex> lock(audioMutex.mutex);
                audioMutex.reset = true;
            }
#if defined(TLRENDER_AUDIO)
            if (thread.rtAudio &&
                thread.rtAudio->isStreamRunning())
            {
                try
                {
                    thread.rtAudio->setStreamTime(0.0);
                }
                catch (const std::exception&)
                {
                    //! \todo How should this be handled?
                }
            }
#endif // TLRENDER_AUDIO
        }

#if defined(TLRENDER_AUDIO)
        int Player::Private::rtAudioCallback(
            void* outputBuffer,
            void* inputBuffer,
            unsigned int nFrames,
            double streamTime,
            RtAudioStreamStatus status,
            void* userData)
        {
            auto p = reinterpret_cast<Player::Private*>(userData);
            
            // Get mutex protected values.
            Playback playback = Playback::Stop;
            otime::RationalTime playbackStartTime = time::invalidTime;
            double audioOffset = 0.0;
            {
                std::unique_lock<std::mutex> lock(p->mutex.mutex);
                playback = p->mutex.playback;
                playbackStartTime = p->mutex.playbackStartTime;
                audioOffset = p->mutex.audioOffset;
            }
            double speed = 0.0;
            double defaultSpeed = 0.0;
            double speedMultiplier = 1.0F;
            float volume = 1.F;
            bool mute = false;
            std::chrono::steady_clock::time_point muteTimeout;
            bool reset = false;
            {
                std::unique_lock<std::mutex> lock(p->audioMutex.mutex);
                speed = p->audioMutex.speed;
                defaultSpeed = p->timeline->getTimeRange().duration().rate();
                speedMultiplier = defaultSpeed / speed;
                volume = p->audioMutex.volume;
                mute = p->audioMutex.mute;
                muteTimeout = p->audioMutex.muteTimeout;
                reset = p->audioMutex.reset;
                p->audioMutex.reset = false;
            }
            //std::cout << "playback: " << playback << std::endl;
            //std::cout << "playbackStartTime: " << playbackStartTime << std::endl;

            // Zero output audio data.
            std::memset(outputBuffer, 0, nFrames * p->audioThread.info.getByteCount());

            switch (playback)
            {
            case Playback::Forward:
            case Playback::Reverse:
            {
                // Flush the audio resampler and buffer when the RtAudio
                // playback is reset.
                if (reset)
                {
                    if (p->audioThread.resample)
                    {
                        p->audioThread.resample->flush();
                    }
                    p->audioThread.silence.reset();
                    p->audioThread.buffer.clear();
                    p->audioThread.rtAudioCurrentFrame = 0;
                    p->audioThread.backwardsSize =
                        std::numeric_limits<size_t>::max();
                }

                const size_t infoSampleRate = p->ioInfo.audio.sampleRate;
                const size_t threadSampleRate = p->audioThread.info.sampleRate *
                                                speedMultiplier;
                    
                
                auto audioInfo = p->audioThread.info;
                audioInfo.sampleRate = threadSampleRate;
                // Create the audio resampler.
                if (!p->audioThread.resample ||
                    (p->audioThread.resample &&
                     p->audioThread.resample->getInputInfo() !=
                         p->ioInfo.audio) ||
                    (p->audioThread.resample &&
                     p->audioThread.resample->getOutputInfo() !=
                         audioInfo))
                {
                    p->audioThread.resample = audio::AudioResample::create(
                        p->ioInfo.audio,
                        audioInfo);
                }

                // Fill the audio buffer.
                if (p->ioInfo.audio.sampleRate > 0 &&
                    playbackStartTime != time::invalidTime)
                {
                    const bool backwards = playback == Playback::Reverse;
                    if (!p->audioThread.silence)
                    {
                        p->audioThread.silence = audio::Audio::create(audioInfo, infoSampleRate);
                        p->audioThread.silence->zero();
                    }
                    
                    const int64_t playbackStartFrame =
                        playbackStartTime.rescaled_to(infoSampleRate).value() -
                        p->timeline->getTimeRange().start_time().rescaled_to(infoSampleRate).value() -
                        otime::RationalTime(audioOffset, 1.0).rescaled_to(infoSampleRate).value();
                    int64_t frame = playbackStartFrame;
                    auto timeOffset = otime::RationalTime(
                        p->audioThread.rtAudioCurrentFrame +
                        audio::getSampleCount(p->audioThread.buffer),
                        threadSampleRate).rescaled_to(infoSampleRate);

                    const int64_t maxOffset = infoSampleRate;
                    const int64_t frameOffset = timeOffset.value();

                    if (backwards)
                    {
                        frame -= frameOffset;
                    }
                    else
                    {
                        frame += frameOffset;
                    }

                    
                    
                    int64_t seconds = infoSampleRate > 0 ? (frame / infoSampleRate) : 0;
                    int64_t offset = frame - seconds * infoSampleRate;

                    
                    
                    // std::cout << "frame:       " << frame   << std::endl;
                    // std::cout << "seconds:     " << seconds << std::endl;
                    // std::cout << "offset:      " << offset  << std::endl;
                    while (audio::getSampleCount(p->audioThread.buffer) < nFrames)
                    {
                        // std::cout << "\tseconds: " << seconds << std::endl;
                        // std::cout << "\toffset:  " << offset << std::endl;
                        AudioData audioData;
                        {
                            std::unique_lock<std::mutex> lock(p->audioMutex.mutex);
                            const auto j = p->audioMutex.audioDataCache.find(seconds);
                            if (j != p->audioMutex.audioDataCache.end())
                            {
                                audioData = j->second;
                            }
                        }

                        std::vector<float> volumeScale;
                        volumeScale.reserve(audioData.layers.size());
                        std::vector<std::shared_ptr<audio::Audio> > audios;
                        std::vector<const uint8_t*> audioDataP;
                        const size_t dataOffset = offset * p->ioInfo.audio.getByteCount();
                        const auto rate = timeOffset.rate();
                        const auto sample = seconds * rate + offset;
                        const auto currentTime = otime::RationalTime(sample, rate).rescaled_to(1.0);
                        for (const auto& layer : audioData.layers)
                        {
                            float volumeMultiplier = 1.F;
                            if (layer.audio && layer.audio->getInfo() == p->ioInfo.audio)
                            {
                                auto audio = layer.audio;
                                
                                if (layer.inTransition)
                                {
                                    const auto& clipTimeRange = layer.clipTimeRange;
                                    const auto& range = otime::TimeRange(clipTimeRange.start_time().rescaled_to(rate),
                                                                         clipTimeRange.duration().rescaled_to(rate));
                                    const auto  inOffset = layer.inTransition->in_offset().value();
                                    const auto outOffset = layer.inTransition->out_offset().value();
                                    const auto fadeInBegin = range.start_time().value() - inOffset;
                                    const auto fadeInEnd   = range.start_time().value() + outOffset;
                                    if (sample > fadeInBegin)
                                    {
                                        if (sample < fadeInEnd)
                                        {
                                            volumeMultiplier = fadeValue(sample,
                                                                         range.start_time().value() - inOffset - 1.0,
                                                                         range.start_time().value() + outOffset);
                                            volumeMultiplier = std::min(1.F, volumeMultiplier);
                                        }
                                    }
                                    else
                                    {
                                        volumeMultiplier = 0.F;
                                    }
                                }
                                
                                if (layer.outTransition)
                                {
                                    const auto& clipTimeRange = layer.clipTimeRange;
                                    const auto& range = otime::TimeRange(clipTimeRange.start_time().rescaled_to(rate),
                                                                         clipTimeRange.duration().rescaled_to(rate));
                            
                                    const auto  inOffset = layer.outTransition->in_offset().value();
                                    const auto outOffset = layer.outTransition->out_offset().value();
                                    const auto fadeOutBegin = range.end_time_inclusive().value() - inOffset;
                                    const auto fadeOutEnd   = range.end_time_inclusive().value() + outOffset;
                                    if (sample > fadeOutBegin)
                                    {
                                        if (sample < fadeOutEnd)
                                        {
                                            volumeMultiplier = 1.F - fadeValue(sample,
                                                                               range.end_time_inclusive().value() - inOffset,
                                                                               range.end_time_inclusive().value() + outOffset + 1.0);
                                        }
                                        else
                                        {
                                            volumeMultiplier = 0.F;
                                        }
                                    }
                                }
                                
                                
                                if (backwards)
                                {
                                    auto tmp = audio::Audio::create(p->ioInfo.audio, infoSampleRate);
                                    tmp->zero();

                                    std::memcpy(tmp->getData(), audio->getData(), audio->getByteCount());
                                    audio = tmp;
                                    audios.push_back(audio);
                                }
                                audioDataP.push_back(audio->getData() + dataOffset);
                                volumeScale.push_back(volumeMultiplier);
                            }
                        }
                        if (audioDataP.empty())
                        {
                            volumeScale.push_back(0.F);
                            audioDataP.push_back(p->audioThread.silence->getData());
                        }

                        size_t size = std::min(
                            p->playerOptions.audioBufferFrameCount,
                            static_cast<size_t>(maxOffset - offset));

                        if (backwards)
                        {
                            if ( p->audioThread.backwardsSize < size )
                            {
                                size = p->audioThread.backwardsSize;
                            }
                            
                            audio::reverse(
                                const_cast<uint8_t**>(audioDataP.data()),
                                audioDataP.size(),
                                size,
                                p->ioInfo.audio.channelCount,
                                p->ioInfo.audio.dataType);
                        }

                        auto tmp = audio::Audio::create(p->ioInfo.audio, size);
                        tmp->zero();
                        audio::mix(
                            audioDataP.data(),
                            audioDataP.size(),
                            tmp->getData(),
                            volume,
                            volumeScale,
                            size,
                            p->ioInfo.audio.channelCount,
                            p->ioInfo.audio.dataType);

                        if (p->audioThread.resample)
                        {
                            p->audioThread.buffer.push_back(p->audioThread.resample->process(tmp));
                        }

                        if (backwards)
                        {
                            offset -= size;
                            if (offset < 0)
                            {
                                seconds -= 1;
                                if (speedMultiplier < 1.0)
                                    offset += ( maxOffset * speedMultiplier );
                                else
                                    offset += maxOffset;
                                p->audioThread.backwardsSize = static_cast<size_t>(maxOffset - offset);
                            }
                            else
                            {
                                p->audioThread.backwardsSize = size;
                            }
                        }
                        else
                        {
                            offset += size;
                            if (offset >= maxOffset)
                            {
                                offset -= maxOffset;
                                seconds += 1;
                            }
                        }

                    }
                }

                // Send audio data to RtAudio.
                const auto now = std::chrono::steady_clock::now();
                if (defaultSpeed == p->timeline->getTimeRange().duration().rate() &&
                    !mute &&
                    now >= muteTimeout &&
                    nFrames <= getSampleCount(p->audioThread.buffer))
                {
                    audio::move(
                        p->audioThread.buffer,
                        reinterpret_cast<uint8_t*>(outputBuffer),
                        nFrames);
                }

                // Update the audio frame.
                p->audioThread.rtAudioCurrentFrame += nFrames;

                break;
            }
            default: break;
            }

            return 0;
        }

        void Player::Private::rtAudioErrorCallback(
            RtAudioError::Type type,
            const std::string& errorText)
        {
            //std::cout << "RtAudio ERROR: " << errorText << std::endl;
        }
#endif // TLRENDER_AUDIO

        void Player::Private::log(const std::shared_ptr<system::Context>& context)
        {
            const std::string id = string::Format("tl::timeline::Player {0}").arg(this);

            // Get mutex protected values.
            otime::RationalTime currentTime = time::invalidTime;
            otime::TimeRange inOutRange = time::invalidTimeRange;
            io::Options ioOptions;
            PlayerCacheInfo cacheInfo;
            {
                std::unique_lock<std::mutex> lock(mutex.mutex);
                currentTime = mutex.currentTime;
                inOutRange = mutex.inOutRange;
                ioOptions = mutex.ioOptions;
                cacheInfo = mutex.cacheInfo;
            }
            size_t audioDataCacheSize = 0;
            {
                std::unique_lock<std::mutex> lock(audioMutex.mutex);
                audioDataCacheSize = audioMutex.audioDataCache.size();
            }

            // Create an array of characters to draw the timeline.
            const auto& timeRange = timeline->getTimeRange();
            const size_t lineLength = 80;
            std::string currentTimeDisplay(lineLength, '.');
            double n = (currentTime - timeRange.start_time()).value() / timeRange.duration().value();
            size_t index = math::clamp(n, 0.0, 1.0) * (lineLength - 1);
            if (index < currentTimeDisplay.size())
            {
                currentTimeDisplay[index] = 'T';
            }

            // Create an array of characters to draw the cached video frames.
            std::string cachedVideoFramesDisplay(lineLength, '.');
            for (const auto& i : cacheInfo.videoFrames)
            {
                n = (i.start_time() - timeRange.start_time()).value() / timeRange.duration().value();
                const size_t t0 = math::clamp(n, 0.0, 1.0) * (lineLength - 1);
                n = (i.end_time_inclusive() - timeRange.start_time()).value() / timeRange.duration().value();
                const size_t t1 = math::clamp(n, 0.0, 1.0) * (lineLength - 1);
                for (size_t j = t0; j <= t1; ++j)
                {
                    if (j < cachedVideoFramesDisplay.size())
                    {
                        cachedVideoFramesDisplay[j] = 'V';
                    }
                }
            }

            // Create an array of characters to draw the cached audio frames.
            std::string cachedAudioFramesDisplay(lineLength, '.');
            for (const auto& i : cacheInfo.audioFrames)
            {
                double n = (i.start_time() - timeRange.start_time()).value() / timeRange.duration().value();
                const size_t t0 = math::clamp(n, 0.0, 1.0) * (lineLength - 1);
                n = (i.end_time_inclusive() - timeRange.start_time()).value() / timeRange.duration().value();
                const size_t t1 = math::clamp(n, 0.0, 1.0) * (lineLength - 1);
                for (size_t j = t0; j <= t1; ++j)
                {
                    if (j < cachedAudioFramesDisplay.size())
                    {
                        cachedAudioFramesDisplay[j] = 'A';
                    }
                }
            }

            std::vector<std::string> ioOptionStrings;
            for (const auto& i : ioOptions)
            {
                ioOptionStrings.push_back(string::Format("{0}:{1}").arg(i.first).arg(i.second));
            }

            auto logSystem = context->getLogSystem();
            logSystem->print(id, string::Format(
                "\n"
                "    Path: {0}\n"
                "    Current time: {1}\n"
                "    In/out range: {2}\n"
                "    I/O options: {3}\n"
                "    Cache: {4} read ahead, {5} read behind\n"
                "    Video: {6} requests, {7} cached\n"
                "    Audio: {8} requests, {9} cached\n"
                "    {10}\n"
                "    {11}\n"
                "    {12}\n"
                "    (T=current time, V=cached video, A=cached audio)").
                arg(timeline->getPath().get()).
                arg(currentTime).
                arg(inOutRange).
                arg(string::join(ioOptionStrings, ", ")).
                arg(cacheOptions->get().readAhead).
                arg(cacheOptions->get().readBehind).
                arg(thread.videoDataRequests.size()).
                arg(thread.videoDataCache.size()).
                arg(thread.audioDataRequests.size()).
                arg(audioDataCacheSize).
                arg(currentTimeDisplay).
                arg(cachedVideoFramesDisplay).
                arg(cachedAudioFramesDisplay));
        }
    }
}
