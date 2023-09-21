// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <libraw/libraw.h>

#include <tlIO/RAW.h>

#include <tlCore/StringFormat.h>

namespace tl
{
    namespace raw
    {
        namespace
        {
            
            class File
            {
            public:
                File(const std::string& fileName, const file::MemoryRead* memory)
                {
                    int res = 0, w = 0, h = 0, n = 0, bits = 8;

                    _memory = memory;
                        
                    if (memory)
                    {
                        res = rawi_info_from_memory(memory->p, memory->size, &w,
                                                    &h, &n);
                        if (res == 0)
                            throw std::runtime_error(
                                string::Format("{0}: {1}")
                                    .arg(fileName)
                                    .arg("Corrupted image type"));
                            
                        _info.size.w = w;
                        _info.size.h = h;
                            
                        res = rawi_is_16_bit_from_memory(memory->p, memory->size);
                        if (res) bits = 16;
                            
                        _info.pixelType = image::getIntType(n, bits);
                            
                        if (image::PixelType::None == _info.pixelType)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1}")
                                    .arg(fileName)
                                    .arg("Unsupported image type"));
                        }
                        _info.layout.endian = memory::Endian::MSB;
                    }
                    else
                    {

                        // Open the file and read the metadata
                        int ret = iProcessor.open_file(fileName.c_str());
                        
                        _info.size.w = iProcessor.imgdata.sizes.width;
                        _info.size.h = iProcessor.imgdata.sizes.height;
                    
                        _info.pixelType = image::PixelType::RGBA_U8;
                        if (image::PixelType::None == _info.pixelType)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1}")
                                    .arg(fileName)
                                    .arg("Unsupported image type"));
                        }
                        _info.layout.endian = memory::Endian::MSB;
                    }
                }

                const image::Info& getInfo() const
                {
                    return _info;
                }

                io::VideoData read(
                    const std::string& fileName,
                    const otime::RationalTime& time)
                {
                    io::VideoData out;
                    out.time = time;
                    out.image = image::Image::create(_info);

                    // Let us unpack the image
                    int ret = iProcessor.unpack();
                    if (ret != 0)
                    {
                        throw std::runtime_error(
                            string::Format("{0}: {1} - Error {2}")
                            .arg(fileName)
                            .arg("unpack failed")
                            .arg(ret));
                    }
                    
                    ret = iProcessor.raw2image();
                    if (ret != 0)
                    {
                        throw std::runtime_error(
                            string::Format("{0}: {1} - Error {2}")
                            .arg(fileName)
                            .arg("raw2image failed")
                            .arg(ret));
                    }
                    
                    const int channels = image::getChannelCount(_info.pixelType);
                    const size_t bytes = image::getBitDepth(_info.pixelType) / 8;
                                                       
                    memcpy(
                        out.image->getData(), data,
                        iProcessor.imgdata.sizes.iwidth *
                        iProcessor.imgdata.sizes.iheight * channels * bytes);

                    iProcessor.recycle();


                    return out;
                }

            private:
                LibRaw iProcessor;
                image::Info _info;
                const file::MemoryRead* _memory;
            };
        }

        void Read::_init(
            const file::Path& path,
            const std::vector<file::MemoryRead>& memory,
            const io::Options& options,
            const std::shared_ptr<io::Cache>& cache,
            const std::weak_ptr<log::System>& logSystem)
        {
            ISequenceRead::_init(path, memory, options, cache, logSystem);
        }

        Read::Read()
        {}

        Read::~Read()
        {
            _finish();
        }

        std::shared_ptr<Read> Read::create(
            const file::Path& path,
            const io::Options& options,
            const std::shared_ptr<io::Cache>& cache,
            const std::weak_ptr<log::System>& logSystem)
        {
            auto out = std::shared_ptr<Read>(new Read);
            out->_init(path, {}, options, cache, logSystem);
            return out;
        }

        std::shared_ptr<Read> Read::create(
            const file::Path& path,
            const std::vector<file::MemoryRead>& memory,
            const io::Options& options,
            const std::shared_ptr<io::Cache>& cache,
            const std::weak_ptr<log::System>& logSystem)
        {
            auto out = std::shared_ptr<Read>(new Read);
            out->_init(path, memory, options, cache, logSystem);
            return out;
        }

        io::Info Read::_getInfo(
            const std::string& fileName,
            const file::MemoryRead* memory)
        {
            io::Info out;
            out.video.push_back(File(fileName, memory).getInfo());
            out.videoTime = otime::TimeRange::range_from_start_end_time_inclusive(
                otime::RationalTime(_startFrame, _defaultSpeed),
                otime::RationalTime(_endFrame, _defaultSpeed));
            return out;
        }

        io::VideoData Read::_readVideo(
            const std::string& fileName,
            const file::MemoryRead* memory,
            const otime::RationalTime& time,
            uint16_t layer)
        {
            return File(fileName, memory).read(fileName, time);
        }
    }
}
