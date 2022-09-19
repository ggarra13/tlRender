// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2022 Darby Johnston
// All rights reserved.

#pragma once

#include <tlIO/SequenceIO.h>

#include <tlCore/FileIO.h>

namespace tl
{
    //! NetPBM I/O.
    //!
    //! References:
    //! - Netpbm, "PPM Format Specification"
    //!   http://netpbm.sourceforge.net/doc/ppm.html
    namespace ppm
    {
        //! PPM data type.
        enum class Data
        {
            ASCII,
            Binary,

            Count,
            First = ASCII
        };
        TLRENDER_ENUM(Data);
        TLRENDER_ENUM_SERIALIZE(Data);

        //! Get the number of bytes in a file scanline.
        size_t getFileScanlineByteCount(
            int    width,
            size_t channelCount,
            size_t bitDepth);

        //! Read PPM file ASCII data.
        void readASCII(
            const std::shared_ptr<file::FileIO>& io,
            uint8_t* out,
            size_t size,
            size_t componentSize);

        //! Save PPM file ASCII data.
        size_t writeASCII(
            const uint8_t* in,
            char* out,
            size_t size,
            size_t componentSize);

        //! PPM reader.
        class Read : public io::ISequenceRead
        {
        protected:
            void _init(
                const file::Path&,
                const std::vector<io::MemoryFileRead>&,
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

            //! Create a new reader.
            static std::shared_ptr<Read> create(
                const file::Path&,
                const std::vector<io::MemoryFileRead>&,
                const io::Options&,
                const std::weak_ptr<log::System>&);

        protected:
            io::Info _getInfo(
                const std::string& fileName,
                const io::MemoryFileRead*) override;
            io::VideoData _readVideo(
                const std::string& fileName,
                const io::MemoryFileRead*,
                const otime::RationalTime&,
                uint16_t layer) override;
        };

        //! PPM writer.
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

        private:
            Data _data = Data::Binary;
        };

        //! PPM plugin.
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
            std::shared_ptr<io::IRead> read(
                const file::Path&,
                const std::vector<io::MemoryFileRead>&,
                const io::Options & = io::Options()) override;
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
