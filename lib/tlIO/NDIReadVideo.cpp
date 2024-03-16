// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024 Gonzalo Garramuño
// All rights reserved.

#include <tlIO/FFmpegMacros.h>
#include <tlIO/NDIReadPrivate.h>

#include <tlCore/StringFormat.h>

extern "C"
{
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}


namespace tl
{
    namespace ndi
    {
        namespace
        {
            bool canCopy(AVPixelFormat in, AVPixelFormat out)
            {
                return (in == out &&
                        (AV_PIX_FMT_RGB24   == in ||
                         AV_PIX_FMT_RGBA    == in ||
                         AV_PIX_FMT_YUV420P == in));
            }
        }

        ReadVideo::ReadVideo(
            const std::string& fileName,
            const NDIlib_video_frame_t& v,
            const std::weak_ptr<log::System>& logSystem,
            const Options& options) :
            _fileName(fileName),
            _logSystem(logSystem),
            _options(options)
        {   
            double fps = v.frame_rate_N /
                         static_cast<double>(v.frame_rate_D);
            double start = 0.0;
            double last = kNDI_MOVIE_DURATION * fps; 
            _timeRange = otime::TimeRange(otime::RationalTime(start, fps),
                                          otime::RationalTime(last, fps));
            
            _info.size.w = v.xres;
            _info.size.h = v.yres;
            if (v.picture_aspect_ratio == 0.F)
                _info.size.pixelAspectRatio = 1.0 / _info.size.w * _info.size.h;
            else
                _info.size.pixelAspectRatio = 1.0;
            _info.layout.mirror.y = true;

            switch(v.FourCC)
            {
            case NDIlib_FourCC_type_UYVY:
                // YCbCr color space packed, not planar using 4:2:2.
                _avInputPixelFormat = AV_PIX_FMT_UYVY422;
                _avOutputPixelFormat = AV_PIX_FMT_RGB24;
                _info.pixelType = image::PixelType::RGB_U8;
                break;
            case NDIlib_FourCC_type_P216:
                // this is a 16bpp version of NV12 (semi-planar 4:2:2).
                _avInputPixelFormat = AV_PIX_FMT_P016LE;
                _avOutputPixelFormat = AV_PIX_FMT_YUV422P16LE;
                _info.pixelType = image::PixelType::YUV_422P_U16;
                break;
            case NDIlib_FourCC_type_PA16:
                // YCbCr color space using 4:2:2 in 16bpp.
                _avInputPixelFormat = AV_PIX_FMT_YUVA422P16LE;
                _avOutputPixelFormat = AV_PIX_FMT_RGBA64;
                _info.pixelType = image::PixelType::RGBA_U16;
                break;
            case NDIlib_FourCC_type_YV12:
                // Planar 8bit 4:2:0 video format.
                _avInputPixelFormat = AV_PIX_FMT_YUV420P;
                _info.pixelType = image::PixelType::YUV_420P_U8;
                _avOutputPixelFormat = _avInputPixelFormat;
                break;
            case NDIlib_FourCC_type_RGBA:
                _avInputPixelFormat = AV_PIX_FMT_RGBA;
                _info.pixelType = image::PixelType::RGBA_U8;
                _avOutputPixelFormat = _avInputPixelFormat;
                break;
            case NDIlib_FourCC_type_RGBX:
                _avInputPixelFormat = AV_PIX_FMT_RGB24;
                _info.pixelType = image::PixelType::RGB_U8;
                _avOutputPixelFormat = _avInputPixelFormat;
                break;
            case NDIlib_FourCC_type_BGRX:
                _avInputPixelFormat = AV_PIX_FMT_BGR24;
                _info.pixelType = image::PixelType::RGB_U8;
                _avOutputPixelFormat = AV_PIX_FMT_RGB24;
                break;
            case NDIlib_FourCC_type_I420:
                // @todo: Not supported yet, this is YUV with UV reversed
                LOG_ERROR("NDI I420 pixel format not supported yet", "ndi");
                _avInputPixelFormat = AV_PIX_FMT_YUV420P;
                _avOutputPixelFormat = _avInputPixelFormat;
                _info.pixelType = image::PixelType::YUV_420P_U8;
                break;
            default:
                throw std::runtime_error("Unsupported pixel type");
            }
            
            _info.videoLevels = image::VideoLevels::FullRange;
        }

        ReadVideo::~ReadVideo()
        {
            if (_swsContext)
            {
                sws_freeContext(_swsContext);
            }
            if (_avFrame2)
            {
                av_frame_free(&_avFrame2);
            }
            if (_avFrame)
            {
                av_frame_free(&_avFrame);
            }
        }

        bool ReadVideo::isValid() const
        {
            return true;
        }

        const image::Info& ReadVideo::getInfo() const
        {
            return _info;
        }

        const otime::TimeRange& ReadVideo::getTimeRange() const
        {
            return _timeRange;
        }

        bool ReadVideo::process(const otime::RationalTime& currentTime,
                                const NDIlib_video_frame_t& video_frame)
        {
            bool out = true;

            AVRational r;
            r.num = 1;
            r.den = _timeRange.duration().rate();

            AVRational time_base;
            time_base.num = 1;
            time_base.den = NDI_TIME_BASE;

            const int64_t dts =
                av_rescale_q(video_frame.timecode, time_base, r);

            const otime::RationalTime time(
                _timeRange.start_time().value() + dts,
                _timeRange.duration().rate());
                
                
            if (1) //time >= currentTime)
            {
                // Fill source avFrame
                av_image_fill_arrays(
                    _avFrame->data,
                    _avFrame->linesize,
                    video_frame.p_data,
                    _avInputPixelFormat,
                    _info.size.w,
                    _info.size.h,
                    1);
                
                auto image = image::Image::create(_info);
                _copy(image);
                _buffer.push_back(image);
                out = false;
            }
            
            return out;
        }
        
        bool ReadVideo::isBufferEmpty() const
        {
            return _buffer.empty();
        }

        std::shared_ptr<image::Image> ReadVideo::popBuffer()
        {
            std::shared_ptr<image::Image> out;
            if (!_buffer.empty())
            {
                out = _buffer.front();
                _buffer.pop_front();
            }
            return out;
        }
        
        void ReadVideo::start()
        {
            _avFrame = av_frame_alloc();
            if (!_avFrame)
            {
                throw std::runtime_error(string::Format("{0}: Cannot allocate frame").arg(_fileName));
            }

            if (!canCopy(_avInputPixelFormat, _avOutputPixelFormat))
            {
                _avFrame2 = av_frame_alloc();
                if (!_avFrame2)
                {
                    throw std::runtime_error(string::Format("{0}: Cannot allocate frame").arg(_fileName));
                }
                _swsContext = sws_alloc_context();
                if (!_swsContext)
                {
                    throw std::runtime_error(string::Format("{0}: Cannot allocate context").arg(_fileName));
                }
                av_opt_set_defaults(_swsContext);
                int r = av_opt_set_int(
                    _swsContext, "srcw", _info.size.w, AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(
                    _swsContext, "srch", _info.size.h, AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(
                    _swsContext, "src_format", _avInputPixelFormat,
                    AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(
                    _swsContext, "dstw", _info.size.w, AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(
                    _swsContext, "dsth", _info.size.h, AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(
                    _swsContext, "dst_format", _avOutputPixelFormat,
                    AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(
                    _swsContext, "sws_flags", swsScaleFlags,
                    AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(
                    _swsContext, "threads", 0, AV_OPT_SEARCH_CHILDREN);
                r = sws_init_context(_swsContext, nullptr, nullptr);
                if (r < 0)
                {
                    throw std::runtime_error(string::Format("{0}: Cannot initialize sws context").arg(_fileName));
                }

                // Handle matrices and color space details
                int in_full, out_full, brightness, contrast, saturation;
                const int *inv_table, *table;

                sws_getColorspaceDetails(
                    _swsContext, (int**)&inv_table, &in_full,
                    (int**)&table, &out_full, &brightness, &contrast,
                    &saturation);
                
                inv_table = sws_getCoefficients(SWS_CS_ITU709);
                table     = sws_getCoefficients(SWS_CS_ITU709);
                        
                in_full = 0;
                out_full = 1;

                sws_setColorspaceDetails(
                    _swsContext, inv_table, in_full, table, out_full,
                    brightness, contrast, saturation);
            }
        }        

        void ReadVideo::_copy(std::shared_ptr<image::Image>& image)
        {
            const auto& info = image->getInfo();
            const std::size_t w = info.size.w;
            const std::size_t h = info.size.h;
            uint8_t* const data = image->getData();
            if (canCopy(_avInputPixelFormat, _avOutputPixelFormat))
            {
                const uint8_t* const data0 = _avFrame->data[0];
                const int linesize0 = _avFrame->linesize[0];
                switch (_avInputPixelFormat)
                {
                case AV_PIX_FMT_RGB24:
                    for (std::size_t i = 0; i < h; ++i)
                    {
                        std::memcpy(
                            data + w * 3 * i,
                            data0 + linesize0 * 3 * i,
                            w * 3);
                    }
                    break;
                case AV_PIX_FMT_RGBA:
                    for (std::size_t i = 0; i < h; ++i)
                    {
                        std::memcpy(
                            data + w * 4 * i,
                            data0 + linesize0 * 4 * i,
                            w * 4);
                    }
                    break;
                case AV_PIX_FMT_YUV420P:
                {   
                    const std::size_t w2 = w / 2;
                    const std::size_t h2 = h / 2;
                    const uint8_t* const data1 = _avFrame->data[1];
                    const uint8_t* const data2 = _avFrame->data[2];
                    const int linesize1 = _avFrame->linesize[1];
                    const int linesize2 = _avFrame->linesize[2];
                    for (std::size_t i = 0; i < h; ++i)
                    {
                        std::memcpy(
                            data + w * i,
                            data0 + linesize0 * i,
                            w);
                    }
                    for (std::size_t i = 0; i < h2; ++i)
                    {
                        std::memcpy(
                            data + (w * h) + w2 * i,
                            data1 + linesize1 * i,
                            w2);
                        std::memcpy(
                            data + (w * h) + (w2 * h2) + w2 * i,
                            data2 + linesize2 * i,
                            w2);
                    }
                    break;
                }
                case AV_PIX_FMT_UYVY422:
                {
                    size_t numPixels = linesize0 * h;
                    size_t index = 0;
                    
                    for (size_t i = 0; i < numPixels; i += 4, index += 6)
                    {
                        uint8_t Y0 = data0[i];
                        uint8_t U = data0[i + 1];
                        uint8_t Y1 = data0[i + 2];
                        uint8_t V = data0[i + 3];

                        int R0 = std::max(
                            0, std::min(255, int(Y0 + 1.402 * (V - 128))));
                        int G0 = std::max(
                            0, std::min(
                                   255, int(Y0 - 0.344136 * (U - 128) -
                                            0.714136 * (V - 128))));
                        int B0 = std::max(
                            0, std::min(255, int(Y0 + 1.772 * (U - 128))));

                        int R1 = std::max(
                            0, std::min(255, int(Y1 + 1.402 * (V - 128))));
                        int G1 = std::max(
                            0, std::min(
                                   255, int(Y1 - 0.344136 * (U - 128) -
                                            0.714136 * (V - 128))));
                        int B1 = std::max(
                            0, std::min(255, int(Y1 + 1.772 * (U - 128))));

                        data[index]     = R0;
                        data[index + 1] = G0;
                        data[index + 2] = B0;
                        
                        data[index + 3] = R1;
                        data[index + 4] = G1;
                        data[index + 5] = B1;
                    }
                    
                    break;
                }
                default: break;
                }
            }
            else
            {
                // Fill destination avFrame
                av_image_fill_arrays(
                    _avFrame2->data,
                    _avFrame2->linesize,
                    data,
                    _avOutputPixelFormat,
                    w,
                    h,
                    1);

                // Do the conversion with FFmpeg
                sws_scale(
                    _swsContext,
                    (uint8_t const* const*)_avFrame->data,
                    _avFrame->linesize,
                    0,
                    _info.size.h,
                    _avFrame2->data,
                    _avFrame2->linesize);
            }
        }

        void ReadVideo::_printTable(const std::string& name,
                                    const int32_t* table)
        {
            char msg[256];
            if (!table)
            {
                snprintf(msg, 256, "%s = nullptr", name.c_str());
                
                LOG_STATUS(msg, "ndi");
                return;
            }
            
            snprintf(msg, 256, "%s = %d %d %d %d",
                     name.c_str(), table[0], table[1], table[2], table[3]);
            
            LOG_STATUS(msg, "ndi");
        }
    }
}
