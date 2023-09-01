// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlIO/OpenEXRPrivate.h>

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

        void Write::_writeLayer(
            const std::shared_ptr<image::Image>& image,
            int layerId)
        {
            const uint8_t channelCount = getChannelCount(_pixelType);
            const uint8_t bitDepth = getBitDepth(_pixelType) / 8;
            Imf::OutputPart out(*_outputFile, layerId);
            const Imf::Header& header = _outputFile->header(layerId);
            const Imath::Box2i& daw = header.dataWindow();

            const size_t width   = daw.max.x - daw.min.x;
            const size_t height  = daw.max.y - daw.min.y;
            const size_t xStride = bitDepth * channelCount;
            const size_t yStride = bitDepth * channelCount * width;
                
            char* base = const_cast<char*>(reinterpret_cast<const char*>(image->getData()));
            char* flip = new char[height * yStride];
            flipImageY(flip, base, height, yStride);
                
            Imf::FrameBuffer fb;
            auto ci = header.channels().begin();
            auto ce = header.channels().end();
            for (int k = 3; ci != ce; ++ci)
            {
                const std::string& name = ci.name();
                char* buf = flip + k-- * bitDepth;
                fb.insert(
                    name,
                    Imf::Slice(toImf(_pixelType), buf, xStride, yStride));
            }

            out.setFrameBuffer(fb);
            out.writePixels(height + 1);
            delete [] flip;
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
                info.size.pixelAspectRatio,
                Imath::V2f(0.F, 0.F),
                1.F,
                Imf::INCREASING_Y,
                toImf(_compression));
            header.zipCompressionLevel() = _zipCompressionLevel;
            header.dwaCompressionLevel() = _dwaCompressionLevel;
            const auto tags = image->getTags();
            writeTags(tags, io::sequenceDefaultSpeed, header);

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
            default:
                throw std::runtime_error("Invalid channel count");
                break;
            }

            const std::string layerName = fileName;
            header.setName(layerName);
            header.setType(Imf::SCANLINEIMAGE);
            header.setVersion(1);

            auto i = tags.find("Display Window");
            if ( i != tags.end())
            {
                std::stringstream s(i->second);
                math::Box2i box;
                s >> box;
                header.displayWindow() = Imath::Box2i(
                    Imath::V2i(box.min.x, box.min.y),
                    Imath::V2i(box.max.x, box.max.y));
            }
            i = tags.find("Data Window");
            if ( i != tags.end())
            {
                std::stringstream s(i->second);
                math::Box2i box;
                s >> box;
                header.dataWindow() = Imath::Box2i(
                    Imath::V2i(box.min.x, box.min.y),
                    Imath::V2i(box.max.x, box.max.y));
            }
            
            std::vector<Imf::Header> headers;
            headers.push_back(header);
            
            const int numParts = static_cast<int>(headers.size());
            _outputFile = new Imf::MultiPartOutputFile(
                fileName.c_str(), &headers[0], numParts);

            for (int part = 0; part < numParts; ++part)
            {
                _writeLayer(image, part);
            }

            delete _outputFile;
        }
    }
}
