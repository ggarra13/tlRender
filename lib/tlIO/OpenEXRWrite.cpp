// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlIO/OpenEXR.h>

#include <tlCore/StringFormat.h>

#include <ImfHeader.h>
#include <ImfChannelList.h>
#include <ImfPartType.h>
#include <ImfFrameBuffer.h>
#include <ImfStandardAttributes.h>
#include <ImfMultiPartOutputFile.h>
#include <ImfOutputPart.h>

namespace tl
{
    namespace exr
    {
        namespace
        {
            void flipImageY(
                char* dst, const char* src, const size_t height,
                const size_t bytes)
            {
                for (size_t row = 0; row < height; ++row)
                {
                    const char* srcRow = src + row * bytes;
                    char* destRow = dst + (height - row - 1) * bytes;

                    // Copy the row from the source image to the destination
                    // image
                    std::memcpy(destRow, srcRow, bytes);
                }
            }
        }
        
        void Write::_init(
            const file::Path& path,
            const io::Info& info,
            const io::Options& options,
            const std::weak_ptr<log::System>& logSystem)
        {
            ISequenceWrite::_init(path, info, options, logSystem);

            auto i = options.find("OpenEXR/Compression");
            if (i != options.end())
            {
                std::stringstream ss(i->second);
                ss >> _compression;
            }
            i = options.find("OpenEXR/PixelType");
            if (i != options.end())
            {
                std::stringstream ss(i->second);
                ss >> _pixelType;
            }
            i = options.find("OpenEXR/ZipCompressionLevel");
            if (i != options.end())
            {
                std::stringstream ss(i->second);
                ss >> _zipCompressionLevel;
            }
            i = options.find("OpenEXR/DWACompressionLevel");
            if (i != options.end())
            {
                std::stringstream ss(i->second);
                ss >> _dwaCompressionLevel;
            }
        }

        Write::Write()
        {}

        Write::~Write()
        {}

        std::shared_ptr<Write> Write::create(
            const file::Path& path,
            const io::Info& info,
            const io::Options& options,
            const std::weak_ptr<log::System>& logSystem)
        {
            auto out = std::shared_ptr<Write>(new Write);
            out->_init(path, info, options, logSystem);
            return out;
        }

        void Write::_writeVideo(
            const std::string& fileName,
            const otime::RationalTime&,
            const std::shared_ptr<image::Image>& image)
        {
            const auto& info = image->getInfo();
            Imf::Header header(
                info.size.w,
                info.size.h,
                1.F,
                Imath::V2f(0.F, 0.F),
                1.F,
                Imf::INCREASING_Y,
                toImf(_compression));
            header.zipCompressionLevel() = _zipCompressionLevel;
            header.dwaCompressionLevel() = _dwaCompressionLevel;
            writeTags(image->getTags(), io::sequenceDefaultSpeed, header);
            
            std::vector<Imf::FrameBuffer> fbs;
            Imf::FrameBuffer fb;
            const uint8_t bitDepth = getBitDepth(_pixelType) / 8;
            const uint8_t channelCount = getChannelCount(_pixelType);

            switch (channelCount)
            {
            case 2:
                header.channels().insert("A", Imf::Channel(toImf(_pixelType)));
                // no break here
            case 1:
                header.channels().insert("L", Imf::Channel(toImf(_pixelType)));
                break;
            case 4:
                header.channels().insert("A", Imf::Channel(toImf(_pixelType)));
                // no break here
            case 3:
                header.channels().insert("B", Imf::Channel(toImf(_pixelType)));
                header.channels().insert("G", Imf::Channel(toImf(_pixelType)));
                header.channels().insert("R", Imf::Channel(toImf(_pixelType)));
                break;
            }

            
            header.setName(fileName);
            header.setType(Imf::SCANLINEIMAGE);
            header.setVersion(1);
            
            std::vector<char*> slices;

            auto ci = header.channels().begin();
            auto ce = header.channels().end();
            uint8_t* base = image->getData();
            const size_t xStride = bitDepth * channelCount;
            const size_t yStride = bitDepth * channelCount * info.size.w;
            int k = 3;
            for ( ; ci != ce; ++ci)
            {
                const std::string& name = ci.name();
                char* buf = reinterpret_cast<char*>(base + k-- * bitDepth);
                char* flip = new char[info.size.h * yStride];
                flipImageY(flip, buf, info.size.h, yStride);
                fb.insert(
                    name,
                    Imf::Slice(toImf(_pixelType), flip, xStride, yStride));
                slices.push_back(flip);
            }
            
            fbs.push_back(fb);

            std::vector<Imf::Header> headers;
            headers.push_back(header);
            
            const int numParts = static_cast<int>(headers.size());
            Imf::MultiPartOutputFile multi(
                fileName.c_str(), &headers[0], numParts);
            for (int part = 0; part < numParts; ++part)
            {
                Imf::OutputPart out(multi, part);
                const Imf::Header& header = multi.header(part);
                const Imath::Box2i& daw = header.dataWindow();
                out.setFrameBuffer(fbs[part]);
                out.writePixels(daw.max.y - daw.min.y + 1);
            }

            for (auto& slice : slices)
                delete [] slice;
        }
    }
}
