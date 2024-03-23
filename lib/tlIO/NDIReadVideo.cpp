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

namespace
{
    const char* kModule = "ndi";
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
            const NDIlib_source_t& NDIsource,
            const NDIlib_recv_create_v3_t& recv_desc,
            const NDIlib_video_frame_t& v,
            const std::weak_ptr<log::System>& logSystem,
            const Options& options) :
            _fileName(fileName),
            _logSystem(logSystem),
            _options(options)
        {   
            double fps = v.frame_rate_N /
                         static_cast<double>(v.frame_rate_D);
            double startTime = 0.0;
            double lastTime = kNDI_MOVIE_DURATION * fps; 
            _timeRange = otime::TimeRange(otime::RationalTime(startTime, fps),
                                          otime::RationalTime(lastTime, fps));
            
            NDI_recv = NDIlib_recv_create(&recv_desc);
            if (!NDI_recv)
                throw std::runtime_error("Could not create NDI audio receiver");
            
            _from_ndi(v);
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

            stop();
        }

        const bool ReadVideo::isValid() const
        {
            return NDI_recv;
        }

        const image::Info& ReadVideo::getInfo() const
        {
            return _info;
        }

        const otime::TimeRange& ReadVideo::getTimeRange() const
        {
            return _timeRange;
        }

        
        void ReadVideo::_from_ndi(const NDIlib_video_frame_t& v)
        {
            
            bool init = false;

            float pixelAspectRatio = 1.F;
            if (v.picture_aspect_ratio == 0.F)
                pixelAspectRatio = 1.F / v.xres * v.yres;
                
            if (_info.size.w != v.xres || _info.size.h != v.yres ||
                _info.size.pixelAspectRatio != pixelAspectRatio)
            {
                init = true;
            }

            _info.size.w = v.xres;
            _info.size.h = v.yres;
            _info.size.pixelAspectRatio = pixelAspectRatio;
            _info.layout.mirror.y = true;
            _info.videoLevels = image::VideoLevels::FullRange;
                
            if (_ndiFourCC != v.FourCC ||
                _ndiStride != v.line_stride_in_bytes)
            {
                init = true;
                _ndiFourCC = v.FourCC;
                _ndiStride = v.line_stride_in_bytes;
            
                switch(v.FourCC)
                {
                case NDIlib_FourCC_type_UYVY:
                    // YCbCr color space packed, not planar using 4:2:2. (works)
                    _avInputPixelFormat = AV_PIX_FMT_UYVY422;
                    _avOutputPixelFormat = AV_PIX_FMT_RGB24;
                    _info.pixelType = image::PixelType::RGB_U8;
                    break;
                case NDIlib_FourCC_type_UYVA:
                    // @todo: This is 4:2:2:4 YUV with an alpha plane following.
                    LOG_ERROR("UVYVA pixel format will not have an alpha channel.");
                    // YCbCr color space packed, not planar using 4:2:2:4.
                    _avInputPixelFormat = AV_PIX_FMT_UYVY422;
                    _avOutputPixelFormat = AV_PIX_FMT_RGBA;
                    _info.pixelType = image::PixelType::RGBA_U8;
                    break;
                case NDIlib_FourCC_type_P216:
                    LOG_ERROR("P216 pixel format is buggy");
                    // This is a 16bpp version of NV12 (semi-planar 4:2:2).
                    _avInputPixelFormat = AV_PIX_FMT_P016LE;
                    _avOutputPixelFormat = AV_PIX_FMT_YUV422P16LE;
                    _info.pixelType = image::PixelType::YUV_422P_U16;
                    // _avOutputPixelFormat = AV_PIX_FMT_RGB48;
                    // _info.pixelType = image::PixelType::RGB_U16;
                    break;
                case NDIlib_FourCC_type_PA16:
                    LOG_ERROR("PA16 pixel format is buggy and not supported yet");
                    // This is 4:2:2:4 in 16bpp.
                    _avInputPixelFormat = AV_PIX_FMT_P016LE;
                    _avOutputPixelFormat = AV_PIX_FMT_RGBA64;
                    _info.pixelType = image::PixelType::RGBA_U16;
                    break;
                case NDIlib_FourCC_type_YV12:
                    LOG_STATUS("NDI Stream is YV12");
                    // Planar 8bit 4:2:0 YUV video format.
                    _avInputPixelFormat = AV_PIX_FMT_YUV420P;
                    _info.pixelType = image::PixelType::YUV_420P_U8;
                    _avOutputPixelFormat = _avInputPixelFormat;
                    break;
                case NDIlib_FourCC_type_RGBA:
                    LOG_STATUS("NDI Stream is RGBA");
                    _avInputPixelFormat = AV_PIX_FMT_RGBA;
                    _info.pixelType = image::PixelType::RGBA_U8;
                    _avOutputPixelFormat = _avInputPixelFormat;
                    break;
                case NDIlib_FourCC_type_BGRA:
                    LOG_STATUS("NDI Stream is BGRA");
                    _avInputPixelFormat = AV_PIX_FMT_BGRA;
                    _avOutputPixelFormat = AV_PIX_FMT_RGBA;
                    _info.pixelType = image::PixelType::RGBA_U8;
                    break;
                case NDIlib_FourCC_type_RGBX:
                    LOG_STATUS("NDI Stream is RGBX");
                    _avInputPixelFormat = AV_PIX_FMT_RGB24;
                    _info.pixelType = image::PixelType::RGB_U8;
                    _avOutputPixelFormat = _avInputPixelFormat;
                    break;
                case NDIlib_FourCC_type_BGRX:
                    LOG_STATUS("NDI Stream is BGRX");
                    _avInputPixelFormat = AV_PIX_FMT_BGR24;
                    _info.pixelType = image::PixelType::RGB_U8;
                    _avOutputPixelFormat = AV_PIX_FMT_RGB24;
                    break;
                case NDIlib_FourCC_type_I420:
                    // @todo: Not tested yet, this is 4:2:0 YUV with UV reversed
                    LOG_WARNING("I420 pixel format not tested");
                    _avInputPixelFormat = AV_PIX_FMT_YUV420P;
                    _avOutputPixelFormat = _avInputPixelFormat;
                    _info.pixelType = image::PixelType::YUV_420P_U8;
                    break;
                default:
                    throw std::runtime_error("Unsupported pixel type");
                }
            }
            
            if (init)
            {
                start();
            }
            
            av_image_fill_arrays(
                _avFrame->data,
                _avFrame->linesize,
                v.p_data,
                _avInputPixelFormat,
                _info.size.w,
                _info.size.h,
                1);

            auto image = image::Image::create(_info);
            _copy(image);
            _buffer.push_back(image);
        }
        

        bool ReadVideo::process(const otime::RationalTime& currentTime)
        {
            bool out = true;
            
            if (_buffer.size() < _options.videoBufferSize)
            {
                int decoding = _decode(currentTime);
                if (decoding < 0)
                    LOG_ERROR("Error decoding video stream"); 
                out = false;
            }
            
            return out;
        }
        
        int ReadVideo::_decode(const otime::RationalTime& time)
        {
            int out = 0;
            NDIlib_video_frame_t v;
            NDIlib_frame_type_e type;

            while (out == 0 && NDI_recv)
            {
                type = NDIlib_recv_capture(NDI_recv, &v, nullptr, nullptr, 50);
                if (type == NDIlib_frame_type_error)
                {
                    out = -1;
                }
                else if (type == NDIlib_frame_type_video)
                {
                    _from_ndi(v);
                    NDIlib_recv_free_video(NDI_recv, &v);
                    out = 1;
                }
                else if (type == NDIlib_frame_type_status_change)
                {
                    std::cerr << "Video status change" << std::endl;
                }
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
        
        void ReadVideo::stop()
        {
            if (NDI_recv)
                NDIlib_recv_destroy(NDI_recv);
            NDI_recv = nullptr;
        }
        
        void ReadVideo::start()
        {
            if (_avFrame)
            {
                av_frame_free(&_avFrame);
            }
            if (!_avFrame)
            {
                _avFrame = av_frame_alloc();
                if (!_avFrame)
                {
                    throw std::runtime_error(string::Format("{0}: Cannot allocate frame").arg(_fileName));
                }
            }
            
            if (!canCopy(_avInputPixelFormat, _avOutputPixelFormat))
            {
                if (_avFrame2)
                {
                    av_frame_free(&_avFrame2);
                }

                _avFrame2 = av_frame_alloc();
                if (!_avFrame2)
                {
                    throw std::runtime_error(string::Format("{0}: Cannot allocate frame").arg(_fileName));
                }
                
                if (_swsContext)
                {
                    sws_freeContext(_swsContext);
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

                if (_info.size.w > 1920 || _info.size.h > 1080)
                {
                    inv_table = sws_getCoefficients(SWS_CS_BT2020);
                }
                else if (_info.size.w > 720 || _info.size.h > 576)
                {
                    inv_table = sws_getCoefficients(SWS_CS_ITU709);
                }
                else
                {
                    inv_table = sws_getCoefficients(SWS_CS_ITU601);
                }
                
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
                default:
                    break;
                }
            }
            else
            {
                // Fill destination avFrame
                av_image_fill_arrays(
                    _avFrame2->data, _avFrame2->linesize, data,
                    _avOutputPixelFormat, w, h, 1);

                // Do some NDI conversion that FFmpeg does not support.
                if (_ndiFourCC == NDIlib_FourCC_type_I420)
                {
                    size_t tmp = _avFrame->linesize[1];
                    _avFrame->linesize[1] = _avFrame->linesize[2];
                    _avFrame->linesize[2] = tmp;
                }

                // Do the conversion with FFmpeg
                sws_scale(
                    _swsContext, (uint8_t const* const*)_avFrame->data,
                    _avFrame->linesize, 0, h, _avFrame2->data,
                    _avFrame2->linesize);
            }
        }
    }
}
