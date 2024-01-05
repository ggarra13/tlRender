// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlIO/Plugin.h>

#include <tlCore/HDR.h>

namespace tl
{
    //! Ndi video and audio I/O
    namespace ndi
    {
        //! Ndi reader
        class Read : public io::IRead
        {
        protected:
            void _init(
                const file::Path&,
                const std::vector<file::MemoryRead>&,
                const io::Options&,
                const std::shared_ptr<io::Cache>&,
                const std::weak_ptr<log::System>&);

            Read();

        public:
            virtual ~Read();

            //! Create a new reader.
            static std::shared_ptr<Read> create(
                const file::Path&,
                const io::Options&,
                const std::shared_ptr<io::Cache>&,
                const std::weak_ptr<log::System>&);

            //! Create a new reader.
            static std::shared_ptr<Read> create(
                const file::Path&,
                const std::vector<file::MemoryRead>&,
                const io::Options&,
                const std::shared_ptr<io::Cache>&,
                const std::weak_ptr<log::System>&);

            std::future<io::Info> getInfo() override;
            std::future<io::VideoData> readVideo(
                const otime::RationalTime&,
                const io::Options& = io::Options()) override;
            std::future<io::AudioData> readAudio(
                const otime::TimeRange&,
                const io::Options& = io::Options()) override;
            void cancelRequests() override;

        private:
            void _videoThread();
            void _audioThread();
            void _cancelVideoRequests();
            void _cancelAudioRequests();

            TLRENDER_PRIVATE();
        };

        //! Ndi Plugin
        class Plugin : public io::IPlugin
        {
        protected:
            void _init(
                const std::shared_ptr<io::Cache>&,
                const std::weak_ptr<log::System>&);

            Plugin();

        public:
            //! Create a new plugin.
            static std::shared_ptr<Plugin> create(
                const std::shared_ptr<io::Cache>&,
                const std::weak_ptr<log::System>&);

            std::shared_ptr<io::IRead> read(
                const file::Path&,
                const io::Options& = io::Options()) override;
            std::shared_ptr<io::IRead> read(
                const file::Path&,
                const std::vector<file::MemoryRead>&,
                const io::Options & = io::Options()) override;
            image::Info getWriteInfo(
                const image::Info&,
                const io::Options& = io::Options()) const override;
            std::shared_ptr<io::IWrite> write(
                const file::Path&,
                const io::Info&,
                const io::Options& = io::Options()) override;
        };
    }
}
