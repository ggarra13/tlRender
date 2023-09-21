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
                    int ret;
                    _memory = memory;
                    if (memory)
                    {
                        ret = iProcessor.open_buffer(memory->p, memory->size);
                        if (ret)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1} - Error {2}")
                                .arg(fileName)
                                .arg("open_buffer failed")
                                .arg(libraw_strerror(ret)));
                        }
                    }
                    else
                    {
                        // Open the file and read the metadata
#ifdef _WIN32
                        ret = iProcessor.open_file(fileName.c_str());
#else
                        ret = iProcessor.open_file(fileName.c_str());
#endif
                        if (ret)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1} - Error {2}")
                                .arg(fileName)
                                .arg("open_file failed")
                                .arg(libraw_strerror(ret)));
                        }
                    }
                    
                    _info.video.resize(1);
                    auto& info = _info.video[0];
                    iProcessor.imgdata.params.output_bps = 16;
                    iProcessor.imgdata.params.no_auto_bright = 1;
                    iProcessor.imgdata.params.adjust_maximum_thr = 0.0f;
                    iProcessor.imgdata.params.user_sat = 0;
                    iProcessor.imgdata.params.use_camera_wb = 0;
                    iProcessor.imgdata.params.use_camera_matrix = 1;


                    const libraw_iparams_t& idata(iProcessor.imgdata.idata);
                    const libraw_colordata_t& color(iProcessor.imgdata.color);
                    const libraw_image_sizes_t& sizes(iProcessor.imgdata.sizes);
                    const libraw_imgother_t& other(iProcessor.imgdata.other);
                    switch(sizes.flip)
                    {
                    case 5:
                        info.layout.mirror.y = true;
                    case 6:
                        info.size.h = sizes.width;
                        info.size.w = sizes.height;
                        break;
                    case 0:
                        info.layout.mirror.y = true;
                    case 3: // no rotation
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
                    if (idata.make[0])
                        tags["Camera Manufacturer"] = idata.make;
                    if (idata.model[0])
                        tags["Camera Model"] = idata.model;
                    tags["Normalized Make"] = idata.normalized_make;
                    tags["Normaliized Model"] = idata.normalized_model;
                    if (idata.software[0])
                        tags["Software"] = idata.software;
                    else if(color.model2[0])
                        tags["Software"] = color.model2;
                    {
                        std::stringstream ss;
                        ss << other.iso_speed;
                        tags["Exif:ISOSpeedRatings"] = ss.str();
                    }
                    {
                        std::stringstream ss;
                        ss << other.shutter;
                        tags["ExposureTime"] = ss.str();
                    }
                    {
                        std::stringstream ss;
                        ss << -std::log2(other.shutter);
                        tags["Exif:ShutterSpeedValue"] = ss.str();
                    }
                    {
                        std::stringstream ss;
                        ss << other.aperture;
                        tags["FNumber"] = ss.str();
                    }
                    {
                        std::stringstream ss;
                        ss << 2.0f * std::log2(other.aperture);
                        tags["Exif:ApertureValue"] = ss.str();
                    }
                    {
                        std::stringstream ss;
                        ss << other.focal_len;
                        tags["Exif:FocalLength"] = ss.str();
                    }
    
                    info.pixelType = image::PixelType::RGB_U16;
                    info.layout.endian = memory::Endian::LSB;
                }
            

                const io::Info& getInfo() const
                {
                    return _info;
                }

                io::VideoData read(
                    const std::string& fileName,
                    const otime::RationalTime& time)
                    {
                        int ret;
                        io::VideoData out;
                        out.time = time;
                        const auto& info = _info.video[0];
                        out.image = image::Image::create(info);

                        if (_memory)
                        {
                            ret = iProcessor.open_buffer(_memory->p, _memory->size);
                            if (ret)
                            {
                                throw std::runtime_error(
                                    string::Format("{0}: {1} - Error {2}")
                                    .arg(fileName)
                                    .arg("open_buffer failed")
                                    .arg(libraw_strerror(ret)));
                            }
                        }
                        else
                        {
                            // Open the file and read the metadata
#ifdef _WIN32
                            ret = iProcessor.open_file(fileName.c_str());
#else
                            ret = iProcessor.open_file(fileName.c_str());
#endif
                            if (ret)
                            {
                                throw std::runtime_error(
                                    string::Format("{0}: {1} - Error {2}")
                                    .arg(fileName)
                                    .arg("open_file failed")
                                    .arg(libraw_strerror(ret)));
                            }
                        }
                        
                        if (ret)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1} - Error {2}")
                                .arg(fileName)
                                .arg("open_file failed")
                                .arg(libraw_strerror(ret)));
                        }
                    
                        const libraw_image_sizes_t& sizes(iProcessor.imgdata.sizes);
                        
                        // Let us unpack the image
                        ret = iProcessor.unpack();
                        if (ret)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1} - Error {2}")
                                .arg(fileName)
                                .arg("unpack failed")
                                .arg(libraw_strerror(ret)));
                        }
                    
                        ret = iProcessor.dcraw_process();
                        if (ret)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1} - Error {2}")
                                .arg(fileName)
                                .arg("dcraw_process failed")
                                .arg(libraw_strerror(ret)));
                        }
                    
                        const int channels = image::getChannelCount(info.pixelType);
                        const size_t bytes = image::getBitDepth(info.pixelType) / 8;

                        image = iProcessor.dcraw_make_mem_image(&ret);
                        if (ret || !image)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1} - Error {2}")
                                .arg(fileName)
                                .arg("dcraw_make_mem_image")
                                .arg(libraw_strerror(ret)));
                        }

                        if (image->type != LIBRAW_IMAGE_BITMAP)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1}")
                                .arg(fileName)
                                .arg("Not a bitmap image"));
                        }
                        if (image->colors != 3 && image->colors != 1)
                        {
                            throw std::runtime_error(
                                string::Format("{0}: {1}")
                                .arg(fileName)
                                .arg("Not supported color depth"));
                        }

                        if (image->colors == 3)
                        {
                            memcpy(out.image->getData(), image->data, image->data_size);
                        }
                        else // image->colors == 1
                        {
                            uint16_t* data = reinterpret_cast<uint16_t*>(out.image->getData());
                            for (size_t i = 0; i < image->data_size; ++i)
                            {
                                const size_t j = i * 3;
                                data[j] = image->data[i];
                                data[j + 1] = image->data[i];
                                data[j + 2] = image->data[i];
                            }
                        }
                    
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
