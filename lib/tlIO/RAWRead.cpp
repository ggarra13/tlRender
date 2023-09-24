// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

//
// Small portions of this code have been borrowed from OpenImageIO.
// Copyright Contributors to the OpenImageIO project.
// SPDX-License-Identifier: Apache-2.0
// https://github.com/OpenImageIO/oiio
//

#include <tlIO/RAW.h>

#include <tlCore/String.h>
#include <tlCore/StringFormat.h>

#include <libraw/libraw.h>

#define LIBRAW_ERROR(function, ret)                 \
    if (ret)                                        \
    {                                               \
        throw std::runtime_error(                   \
            string::Format("{0} - {1}")             \
            .arg(#function)                         \
            .arg(libraw_strerror(ret)));            \
    }



namespace {
    const char*
    libraw_filter_to_str(unsigned int filters)
    {
        // Convert the libraw filter pattern description
        // into a slightly more human readable string
        // LibRaw/internal/defines.h:166
        switch (filters) {
            // CYGM
        case 0xe1e4e1e4: return "GMYC";
        case 0x1b4e4b1e: return "CYGM";
        case 0x1e4b4e1b: return "YCGM";
        case 0xb4b4b4b4: return "GMCY";
        case 0x1e4e1e4e: return "CYMG";

            // RGB
        case 0x16161616: return "BGRG";
        case 0x61616161: return "GRGB";
        case 0x49494949: return "GBGR";
        case 0x94949494: return "RGBG";
        default: break;
        }
        return "";
    }
}  // namespace


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
                    {
                        std::lock_guard<std::mutex> lock(mutex);
                        iProcessor.reset(new LibRaw());
                    }
                    _memory = memory;

                    _openFile(fileName);
                    
                    _info.video.resize(1);
                    auto& info = _info.video[0];

                    const libraw_iparams_t& idata(iProcessor->imgdata.idata);
                    const libraw_colordata_t& color(iProcessor->imgdata.color);
                    const libraw_image_sizes_t& sizes(iProcessor->imgdata.sizes);
                    const libraw_imgother_t& other(iProcessor->imgdata.other);
                    switch(sizes.flip)
                    {
                    case 5:  // 90 deg counter clockwise
                    case 6:  // 90 deg clockwise
                        info.layout.mirror.y = true;
                        info.size.h = sizes.width;
                        info.size.w = sizes.height;
                        break;
                    case 0: // no rotation
                        info.layout.mirror.y = true;
                    case 3: // 180 degree rotation
                    default:
                        info.size.w = sizes.width;
                        info.size.h = sizes.height;
                        break;
                    }
                    info.size.pixelAspectRatio = sizes.pixel_aspect;

                    // Save Tags
                    auto& tags = _info.tags;

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
                    
                    _getTag("Orientation", _getOrientation(sizes.flip));
                    _getTag("ISO Speed Ratings", other.iso_speed);
                    _getTag("Exposure Time", other.shutter);
                    _getTag("Shutter Speed Value",
                            -std::log2(other.shutter));
                    _getTag("FNumber", other.aperture);
                    _getTag("Aperture Value",
                            2.0f * std::log2(other.aperture));
                    _getTag("Focal Length", other.focal_len);
    
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

                        auto& params(iProcessor->imgdata.params);
    
                        // Output 16-bit images
                        params.output_bps = 16;

                        // Some default parameters
                        params.no_auto_bright = 1;
                        params.adjust_maximum_thr = 0.0f;
                        params.user_sat = 0;
                        params.use_camera_wb = 1;

                        // Handle white balance
                        auto& color  = iProcessor->imgdata.color;
                        auto& idata  = iProcessor->imgdata.idata;
                        
                        auto is_rgbg_or_bgrg = [&](unsigned int filters) {
                            std::string filter(libraw_filter_to_str(filters));
                            return filter == "RGBG" || filter == "BGRG";
                        };
                        float norm[4] = { color.cam_mul[0], color.cam_mul[1], color.cam_mul[2],
                          color.cam_mul[3] };

                        if (is_rgbg_or_bgrg(idata.filters)) {
                            // normalize white balance around green
                            norm[0] /= norm[1];
                            norm[1] /= norm[1];
                            norm[2] /= norm[3] > 0 ? norm[3] : norm[1];
                            norm[3] /= norm[3] > 0 ? norm[3] : norm[1];
                        }
                        params.user_mul[0] = norm[0];
                        params.user_mul[1] = norm[1];
                        params.user_mul[2] = norm[2];
                        params.user_mul[3] = norm[3];

                        // Handle camera matrix
                        params.use_camera_matrix = 1;

                        // Handle LCMS color correction with sRGB as output
                        params.output_color = 1;
                        params.gamm[0] = 1.0 / 2.4;
                        params.gamm[1] = 12.92;

                        _openFile(fileName);
                    
                        const libraw_image_sizes_t& sizes(iProcessor->imgdata.sizes);
                        
                        // Let us unpack the image
                        {
                            std::lock_guard<std::mutex> lock(mutex);
                            ret = iProcessor->unpack();
                            LIBRAW_ERROR(unpack, ret);
                        }
                        
                        ret = iProcessor->raw2image_ex(/*substract_black=*/true);
                        LIBRAW_ERROR(raw2image_ex, ret);
                        
                        ret = iProcessor->adjust_maximum();
                        LIBRAW_ERROR(adjust_maximum, ret);
                        
                        iProcessor->imgdata.params.adjust_maximum_thr = 1.0;
                        
                        ret = iProcessor->adjust_maximum();
                        LIBRAW_ERROR(adjust_maximum, ret);
                        
                        ret = iProcessor->dcraw_process();
                        LIBRAW_ERROR(dcraw_process, ret);
                    
                        image = iProcessor->dcraw_make_mem_image(&ret);
                        LIBRAW_ERROR(dcraw_make_mem_image, ret);
                        if (!image)
                        {
                            throw std::runtime_error(
                                "dcraw_make_mem_image returned null");
                        }

                        if (image->type != LIBRAW_IMAGE_BITMAP)
                        {
                            throw std::runtime_error("Not a bitmap image");
                        }
                        if (image->colors != 3 && image->colors != 1)
                        {
                            throw std::runtime_error(
                                "Not supported color depth");
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
                    
                        iProcessor->dcraw_clear_mem(image);
                        iProcessor->recycle();

                        return out;
                    }

            protected:
                void _openFile(const std::string& fileName) const
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    
                    int ret;
                    if (_memory)
                    {
                        ret = iProcessor->open_buffer(_memory->p,
                                                      _memory->size);
                        LIBRAW_ERROR(open_buffer, ret);
                    }
                    else
                    {
                        // Open the file and read the metadata
#ifdef _WIN32
                        const std::wstring wideFileName =
                            string::toWide(fileName);
                        ret = iProcessor->open_file(wideFileName.c_str());
#else
                        ret = iProcessor->open_file(fileName.c_str());
#endif
                        LIBRAW_ERROR(open_file, ret);
                    }
                }
                
                const char* _getOrientation(int flip)
                    {
                        switch(flip)
                        {
                        case 5:  // 90 deg counter clockwise
                            return "90 Degrees Counter Clockwise";
                        case 6:  // 90 deg clockwise
                            return "90 Degrees Clockwise";
                        case 0: // no rotation
                            return "No Rotation";
                        case 3: // 180 degree rotation
                            return "180 degree rotation";
                        default:
                            return "Unknown";
                        }
                    }
                
                template<typename T>
                void _getTag(const char* tag, const T& value)
                    {
                        std::stringstream ss;
                        ss << value;
                        _info.tags[tag] = ss.str();
                    }
                
            private:
                // LibRaw is not thread safe.  We use a static mutex
                // so only one threads constructs a LibRaw at a time.
                static std::mutex mutex;
                std::unique_ptr<LibRaw> iProcessor;
                libraw_processed_image_t* image = nullptr; 
                io::Info _info;
                const file::MemoryRead* _memory;
            };
        }

        std::mutex File::mutex;

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
