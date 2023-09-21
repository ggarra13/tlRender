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
                    _memory = memory;
                        
                    if (memory)
                    {
                        throw std::runtime_error(
                            string::Format("{0}: {1}")
                            .arg(fileName)
                            .arg("Unsupported memory read"));
                    }
                    else
                    {
                        _info.video.resize(1);
                        auto& info = _info.video[0];
                        iProcessor.imgdata.params.output_bps = 16;

                        // Open the file and read the metadata
#ifdef _WIN32
                        int ret = iProcessor.open_file(fileName.c_str());
#else
                        int ret = iProcessor.open_file(fileName.c_str());
#endif
                        if (ret != 0)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1} - Error {2}")
                                .arg(fileName)
                                .arg("open_file failed")
                                .arg(ret));
                        }

                        const libraw_iparams_t& idata(iProcessor.imgdata.idata);
                        const libraw_image_sizes_t& sizes(iProcessor.imgdata.sizes);
                        switch(sizes.flip)
                        {
                        case 5:
                            info.layout.mirror.y = true;
                        case 6:
                            info.size.h = sizes.width;
                            info.size.w = sizes.height;
                            break;
                        case 0: // no rotation
                            info.layout.mirror.y = true;
                        case 3:
                        default:
                            info.size.w = sizes.width;
                            info.size.h = sizes.height;
                            break;
                        }

                        // Save Tags
                        auto& tags = _info.tags;
                        {
                            std::stringstream ss;
                            ss << sizes.flip;
                            tags["flip"] = ss.str();
                        }
                        tags["Camera Manufacturer"] = idata.make;
                        tags["Camera Model"] = idata.model;
                        tags["Software"] = idata.software;
                        info.pixelType = image::PixelType::RGB_U16;
                        info.layout.endian = memory::Endian::LSB;
                    }
                }

                const io::Info& getInfo() const
                {
                    return _info;
                }

                io::VideoData read(
                    const std::string& fileName,
                    const otime::RationalTime& time)
                {
                    io::VideoData out;
                    out.time = time;
                    const auto& info = _info.video[0];
                    out.image = image::Image::create(info);

#ifdef _WIN32
                    int ret = iProcessor.open_file(fileName.c_str());
#else
                    int ret = iProcessor.open_file(fileName.c_str());
#endif
                    if (ret != 0)
                    {
                        throw std::runtime_error(
                            string::Format("{0}: {1} - Error {2}")
                            .arg(fileName)
                            .arg("open_file failed")
                            .arg(ret));
                    }
                    
                    const libraw_image_sizes_t& sizes(iProcessor.imgdata.sizes);
                        
                    // Let us unpack the image
                    ret = iProcessor.unpack();
                    if (ret != 0)
                    {
                        throw std::runtime_error(
                            string::Format("{0}: {1} - Error {2}")
                            .arg(fileName)
                            .arg("unpack failed")
                            .arg(ret));
                    }
                    
                    ret = iProcessor.dcraw_process();
                    if (ret != 0)
                    {
                        throw std::runtime_error(
                            string::Format("{0}: {1} - Error {2}")
                            .arg(fileName)
                            .arg("dcraw_process failed")
                            .arg(ret));
                    }
                    
                    const int channels = image::getChannelCount(info.pixelType);
                    const size_t bytes = image::getBitDepth(info.pixelType) / 8;

                    image = iProcessor.dcraw_make_mem_image(&ret);
                    if (ret != 0 || !image)
                    {
                        throw std::runtime_error(
                            string::Format("{0}: {1} - Error {2}")
                            .arg(fileName)
                            .arg("dcraw_make_mem_image")
                            .arg(ret));
                    }

                    if (image->type != LIBRAW_IMAGE_BITMAP)
                    {
                        throw std::runtime_error(
                            string::Format("{0}: {1}")
                            .arg(fileName)
                            .arg("Not a bitmap image"));
                    }

                    memcpy(out.image->getData(), image->data, image->data_size);

                    iProcessor.dcraw_clear_mem(image);
                    iProcessor.recycle();

                    return out;
                }

            private:
                LibRaw iProcessor;
                libraw_processed_image_t* image = nullptr; 
                io::Info _info;
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
            io::Info out = File(fileName, memory).getInfo();
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
