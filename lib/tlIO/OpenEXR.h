// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#pragma once

#include <ImfCompression.h>

#include <tlIO/SequenceIO.h>

namespace tl
{
    //! OpenEXR image I/O.
    namespace exr
    {
        //! Channel grouping.
        enum class ChannelGrouping
        {
            None,
            Known,
            All,

            Count,
            First = None
        };
        TLRENDER_ENUM(ChannelGrouping);
        TLRENDER_ENUM_SERIALIZE(ChannelGrouping);

        //! OpenEXR reader.
        class Read : public io::ISequenceRead
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

        protected:
            io::Info _getInfo(
                const std::string& fileName,
                const file::MemoryRead*) override;
            io::VideoData _readVideo(
                const std::string& fileName,
                const file::MemoryRead*,
                const otime::RationalTime&,
                const io::Options&) override;

        private:
            ChannelGrouping _channelGrouping = ChannelGrouping::Known;
            bool            _ignoreDisplayWindow = false;
            bool            _autoNormalize = false;
            int             _xLevel = -1;
            int             _yLevel = -1;
        };

        //! OpenEXR writer.
        class Write : public io::ISequenceWrite
        {
        protected:
            void _init(
                const file::Path&,
                const io::Info&,
                const io::Options&,
                const std::weak_ptr<log::System>&);

            Write();

        public:
            virtual ~Write();

            //! Create a new writer.
            static std::shared_ptr<Write> create(
                const file::Path&,
                const io::Info&,
                const io::Options&,
                const std::weak_ptr<log::System>&);

        protected:
            void _writeVideo(
                const std::string& fileName,
                const otime::RationalTime&,
                const std::shared_ptr<image::Image>&,
                const io::Options&) override;

            void _writeLayer(
                const std::shared_ptr<image::Image>& image,
                int layerId = 0);

        private:
            TLRENDER_PRIVATE();
            
            Imf::Compression _compression = Imf::ZIP_COMPRESSION;
            float _dwaCompressionLevel = 45.F;
            int   _zipCompressionLevel = 4;
            double _speed = io::sequenceDefaultSpeed;
        };

        //! OpenEXR plugin.
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
                const io::Options& = io::Options()) override;
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
