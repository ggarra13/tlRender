// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlIO/SequenceIO.h>

#include <tlCore/Box.h>

#include <ImathBox.h>
#include <ImfHeader.h>
#include <ImfInputFile.h>
#include <ImfMultiPartInputFile.h>
#include <ImfPixelType.h>

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

        //! Image channel.
        struct Channel
        {
            Channel();
            Channel(
                const std::string&    name,
                Imf::PixelType        pixelType,
                const math::Vector2i& sampling  = math::Vector2i(1, 1));

            std::string    name;
            Imf::PixelType pixelType = Imf::PixelType::HALF;
            math::Vector2i sampling  = math::Vector2i(1, 1);
        };

        //! Compression types.
        enum class Compression
        {
            None,
            RLE,
            ZIPS,
            ZIP,
            PIZ,
            PXR24,
            B44,
            B44A,
            DWAA,
            DWAB,

            Count,
            First = None
        };
        TLRENDER_ENUM(Compression);
        TLRENDER_ENUM_SERIALIZE(Compression);

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
                uint16_t layer) override;

        private:
            ChannelGrouping _channelGrouping = ChannelGrouping::Known;
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
                const std::shared_ptr<image::Image>&) override;

        private:
            Compression _compression = Compression::ZIP;
            float _dwaCompressionLevel = 45.F;
            int   _zipCompressionLevel = 4;
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
