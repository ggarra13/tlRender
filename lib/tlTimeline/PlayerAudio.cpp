// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#include <tlTimeline/PlayerPrivate.h>

#include <tlTimeline/Util.h>

#include <tlCore/StringFormat.h>

namespace
{
    inline float fadeValue(double sample, double in, double out)
    {
        return (sample - in) / (out - in);
    }
}

namespace tl
{
    namespace timeline
    {
        const std::vector<int>& Player::getChannelMute() const
        {
            return _p->channelMute->get();
        }

        std::shared_ptr<observer::IList<int> > Player::observeChannelMute() const
        {
            return _p->channelMute;
        }

        void Player::setChannelMute(const std::vector<int>& value)
        {
            TLRENDER_P();
            if (p.channelMute->setIfChanged(value))
            {
                std::unique_lock<std::mutex> lock(p.audioMutex.mutex);
                p.audioMutex.channelMute = value;
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
            std::vector<int> channelMute;
            std::chrono::steady_clock::time_point muteTimeout;
            bool reset = false;
            {
                std::unique_lock<std::mutex> lock(p->audioMutex.mutex);
                speed = p->audioMutex.speed;
                defaultSpeed = p->timeline->getTimeRange().duration().rate();
                speedMultiplier = defaultSpeed / speed;
                volume = p->audioMutex.volume;
                mute = p->audioMutex.mute;
                channelMute = p->audioMutex.channelMute;
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
                        int audioIndex = 0;
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

                                if (audioIndex < channelMute.size() &&
                                    channelMute[audioIndex])
                                {
                                    volumeMultiplier = 0.F;
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
                                ++audioIndex;
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
                    // audio::move(
                    //     p->audioThread.buffer,
                    //     reinterpret_cast<uint8_t*>(outputBuffer),
                    //     nFrames);
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
        
    }
}
