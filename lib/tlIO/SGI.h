// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2022 Darby Johnston
// All rights reserved.

#pragma once

#include <tlIO/SequenceIO.h>

#include <tlCore/FileIO.h>

namespace tl
{
    //! Silicon Graphics image I/O.
    //!
    //! References:
    //! - Paul Haeberli, "The SGI Image File Format, Version 1.00"
    //!   http://paulbourke.net/dataformats/sgirgb/sgiversion.html
    namespace sgi
    {
        //! SGI header.
        struct Header
        {
            uint16_t magic = 474;
            uint8_t  storage = 0;
            uint8_t  bytes = 0;
            uint16_t dimension = 0;
            uint16_t width = 0;
            uint16_t height = 0;
            uint16_t channels = 0;
            uint32_t pixelMin = 0;
            uint32_t pixelMax = 0;
        };

        //! SGI reader.
        class Read : public io::ISequenceRead
        {
        protected:
            void _init(
                const file::Path&,
                const io::Options&,
                const std::weak_ptr<log::System>&);
            Read();

        public:
            ~Read() override;

            //! Create a new reader.
            static std::shared_ptr<Read> create(
                const file::Path&,
                const io::Options&,
                const std::weak_ptr<log::System>&);

        protected:
            io::Info _getInfo(const std::string& fileName) override;
            io::VideoData _readVideo(
                const std::string& fileName,
                const otime::RationalTime&,
                uint16_t layer) override;
        };

        //! SGI writer.
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
            ~Write() override;

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
                const std::shared_ptr<imaging::Image>&) override;
        };

        //! SGI plugin.
        class Plugin : public io::IPlugin
        {
        protected:
            Plugin();

        public:
            //! Create a new plugin.
            static std::shared_ptr<Plugin> create(const std::weak_ptr<log::System>&);

            std::shared_ptr<io::IRead> read(
                const file::Path&,
                const io::Options& = io::Options()) override;
            imaging::Info getWriteInfo(
                const imaging::Info&,
                const io::Options& = io::Options()) const override;
            std::shared_ptr<io::IWrite> write(
                const file::Path&,
                const io::Info&,
                const io::Options& = io::Options()) override;
        };
    }
}
