// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlTimeline/Audio.h>
#include <tlTimeline/ReadCache.h>
#include <tlTimeline/Video.h>

#include <tlCore/Context.h>
#include <tlCore/ValueObserver.h>

#include <opentimelineio/timeline.h>

namespace tl
{
    //! Timelines.
    namespace timeline
    {
        //! Get the timeline file extensions.
        std::vector<std::string> getExtensions(
            int types,
            const std::shared_ptr<system::Context>&);

        //! File sequence.
        enum class FileSequenceAudio
        {
            None,      //!< No audio
            BaseName,  //!< Search for an audio file with the same base name as the file sequence
            FileName,  //!< Use the given audio file name
            Directory, //!< Use the first audio file in the given directory

            Count,
            First = None
        };
        TLRENDER_ENUM(FileSequenceAudio);
        TLRENDER_ENUM_SERIALIZE(FileSequenceAudio);

        //! Timeline options.
        struct Options
        {
            FileSequenceAudio fileSequenceAudio = FileSequenceAudio::BaseName;
            std::string fileSequenceAudioFileName;
            std::string fileSequenceAudioDirectory;

            size_t videoRequestCount = 16;
            size_t audioRequestCount = 16;
            std::chrono::milliseconds requestTimeout = std::chrono::milliseconds(5);

            io::Options ioOptions;

            file::PathOptions pathOptions;

            bool operator == (const Options&) const;
            bool operator != (const Options&) const;
        };

        //! Create a new timeline from a file name. The file name can point
        //! to an .otio file, movie file, or image sequence.
        otio::SerializableObject::Retainer<otio::Timeline> create(
            const std::string& fileName,
            const std::shared_ptr<system::Context>&,
            const Options& = Options(),
            const std::shared_ptr<ReadCache>& = nullptr);

        //! Create a new timeline from a file name and audio file name.
        //! The file name can point to an .otio file, movie file, or
        //! image sequence.
        otio::SerializableObject::Retainer<otio::Timeline> create(
            const std::string& fileName,
            const std::string& audioFileName,
            const std::shared_ptr<system::Context>&,
            const Options& = Options(),
            const std::shared_ptr<ReadCache>& = nullptr);

        //! Timeline.
        class Timeline : public std::enable_shared_from_this<Timeline>
        {
            TLRENDER_NON_COPYABLE(Timeline);

        protected:
            void _init(
                const otio::SerializableObject::Retainer<otio::Timeline>&,
                const std::shared_ptr<system::Context>&,
                const Options&,
                const std::shared_ptr<ReadCache>&);

            Timeline();

        public:
            ~Timeline();

            //! Create a new timeline.
            static std::shared_ptr<Timeline> create(
                const otio::SerializableObject::Retainer<otio::Timeline>&,
                const std::shared_ptr<system::Context>&,
                const Options& = Options(),
                const std::shared_ptr<ReadCache>& = nullptr);

            //! Create a new timeline from a file name. The file name can point
            //! to an .otio file, movie file, or image sequence.
            static std::shared_ptr<Timeline> create(
                const std::string& fileName,
                const std::shared_ptr<system::Context>&,
                const Options& = Options());

            //! Create a new timeline from a file name and audio file name.
            //! The file name can point to an .otio file, movie file, or
            //! image sequence.
            static std::shared_ptr<Timeline> create(
                const std::string& fileName,
                const std::string& audioFileName,
                const std::shared_ptr<system::Context>&,
                const Options& = Options());

            //! Get the context.
            const std::weak_ptr<system::Context>& getContext() const;

            //! Get the timeline.
            const otio::SerializableObject::Retainer<otio::Timeline>& getTimeline() const;

            //! Observe timeline changes.
            std::shared_ptr<observer::IValue<bool> > observeTimelineChanges() const;

            //! Set the timeline.
            void setTimeline(const otio::SerializableObject::Retainer<otio::Timeline>&);

            //! Get the file path.
            const file::Path& getPath() const;

            //! Get the audio file path.
            const file::Path& getAudioPath() const;

            //! Get the timeline options.
            const Options& getOptions() const;

            //! \name Information
            ///@{

            //! Get the time range.
            const otime::TimeRange& getTimeRange() const;

            //! Get the I/O information. This information is retreived from
            //! the first clip in the timeline.
            const io::Info& getIOInfo() const;

            ///@}

            //! \name Video and Audio Data
            ///@{

            //! Get video data.
            std::future<VideoData> getVideo(const otime::RationalTime&, uint16_t layer = 0);

            //! Get audio data.
            std::future<AudioData> getAudio(int64_t seconds);

            //! Cancel requests.
            void cancelRequests();

            ///@}

            //! Tick the timeline.
            void tick();

        private:
            TLRENDER_PRIVATE();
        };
    }
}
