// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlIO/NDIReadPrivate.h>

#include <tlCore/StringFormat.h>

namespace tl
{
    namespace ndi
    {
        namespace
        {
            bool canCopy(AVPixelFormat in, AVPixelFormat out)
            {
                return in == out &&
                    (AV_PIX_FMT_RGB24   == in ||
                     AV_PIX_FMT_GRAY8   == in ||
                     AV_PIX_FMT_RGBA    == in ||
                     AV_PIX_FMT_YUV420P == in);
            }
        }

        ReadVideo::ReadVideo(
            const std::string& fileName,
            const std::vector<file::MemoryRead>& memory,
            const Options& options) :
            _fileName(fileName),
            _options(options)
        {
            if (!memory.empty())
            {
            
            }
            pNDI_find = NDIlib_find_create_v2();
            if (!pNDI_find)
                throw std::runtime_error("Could not create NDI find");
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
            
            if (pNDI_find)
                NDIlib_find_destroy(pNDI_find);
    
            // Destroy the receiver
            if (pNDI_recv)
                NDIlib_recv_destroy(pNDI_recv);
            
            // Not required, but nice
            NDIlib_destroy();
        }

        bool ReadVideo::isValid() const
        {
            return true;
        }

        const image::Info& ReadVideo::getInfo() const
        {
            uint32_t no_sources = 0;
            const NDIlib_source_t* p_sources = NULL;
            while (!no_sources)
            {
                // Wait until the sources on the network have changed
                std::cerr << "Looking for sources ..." << std::endl;
                NDIlib_find_wait_for_sources(pNDI_find, 1000/* One second */);
                p_sources = NDIlib_find_get_current_sources(pNDI_find,
                                                            &no_sources);
            }

    
            // We now have at least one source,
            // so we create a receiver to look at it.
            pNDI_recv = NDIlib_recv_create_v3();
            if (!pNDI_recv)
                throw std::runtime_error("Could not create NDI receiver");
    
            // Connect to our sources
            NDIlib_recv_connect(pNDI_recv, p_sources + 0);
    
            // Destroy the NDI finder.
            // We needed to have access to the pointers to p_sources[0]
            NDIlib_find_destroy(pNDI_find);
            pNDI_find = nullptr;

            NDIlib_video_frame_v2_t video_frame;
            NDIlib_audio_frame_v2_t audio_frame;

            NDIlib_recv_capture_v2(pNDI_recv, &video_frame, nullptr,
                                   nullptr, 5000);

            _info.size.w = video_frame.xres;
            _info.size.h = video_frame.yres;
            _info.size.pixelAspectRatio = video_frame.picture_aspect_ratio;
            _info.layout.mirror.y = true;

            switch(video_frame.FourCC)
            {
            case NDIlib_FourCC_type_UYVY:
                // YCbCr color space using 4:2:2.
                _avInputPixelFormat = AV_PIX_FMT_UYVY;
                _info.pixelType = image::PixelType::YUV_422P_U8;
                _avOut
                    putPixelFormat = _avInputPixelFormat;
                break;
            case NDIlib_FourCC_type_P216:
                // YCbCr color space using 4:2:2 in 16bpp.
                _avInputPixelFormat = AV_PIX_FMT_YUV422P16LE;
                if (options.yuvToRGBConversion)
                {
                    _avOutputPixelFormat = AV_PIX_FMT_RGB48;
                    _info.pixelType = image::PixelType::RGB_U16;
                }
                else
                {
                    //! \todo Use the _info.layout.endian field instead of
                    //! converting endianness.
                    _avOutputPixelFormat = AV_PIX_FMT_YUV422P16LE;
                    _info.pixelType = image::PixelType::YUV_422P_U16;
                }
                break;
            case NDIlib_FourCC_type_PA16:
                // YCbCr color space using 4:2:2 in 16bpp.
                _avInputPixelFormat = AV_PIX_FMT_YUVA422P16LE;
                //! \todo Support these formats natively.
                _avOutputPixelFormat = AV_PIX_FMT_RGBA64;
                _info.pixelType = image::PixelType::RGBA_U16;
                break;
            case NDIlib_FourCC_type_YV12:
                // Planar 8bit 4:2:0 video format.
                _avInputPixelFormat = AV_PIX_FMT_YUV420P;
                //! \todo Support these formats natively.
                _avOutputPixelFormat = AV_PIX_FMT_RGBA;
                _info.pixelType = image::PixelType::YUV_420P_U8;
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
                _avOutputPixelFormat = AV_PIXEL_FMT_RGB24;
                break;
            case NDIlib_FourCC_type_I420:
                _avInputPixelFormat = AV_PIX_FMT_YUV420P;
                if (options.yuvToRGBConversion)
                {
                    _avOutputPixelFormat = AV_PIX_FMT_RGB24;
                    _info.pixelType = image::PixelType::RGB_U8;
                }
                else
                {
                    _avOutputPixelFormat = _avInputPixelFormat;
                    _info.pixelType = image::PixelType::YUV_420P_U8;
                }
                break;
            case AV_PIX_FMT_YUVA420P:
            case AV_PIX_FMT_YUVA422P:
            case AV_PIX_FMT_YUVA444P:
                //! \todo Support these formats natively.
                _avOutputPixelFormat = AV_PIX_FMT_RGBA;
                _info.pixelType = image::PixelType::RGBA_U8;
                break;
            default:
                throw std::runtime_error("Unsupported pixel type");
            }
            
    
            return _info;
        }

        const otime::TimeRange& ReadVideo::getTimeRange() const
        {
            // @todo: how to determine time range? It seems NDI is a constant
            //        stream...
            double fps = frame_rate_N / frame_rate_D;
            _timeRange = otime::TimeRange(otime::RationalTime(0, fps),
                                          otime::RationalTime(300000, fps));
            return _timeRange;
        }

        bool ReadVideo::process(const otime::RationalTime& currentTime)
        {
            bool out = false;
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
                int r = av_opt_set_int(_swsContext, "srcw", _info.size.w);
                r = av_opt_set_int(_swsContext, "srch", _info.size.h);
                r = av_opt_set_int(_swsContext, "src_format", _avInputPixelFormat, AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(_swsContext, "dstw", _info.size.w, AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(_swsContext, "dsth", _info.size.h, AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(_swsContext, "dst_format", _avOutputPixelFormat, AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(_swsContext, "sws_flags", swsScaleFlags, AV_OPT_SEARCH_CHILDREN);
                r = av_opt_set_int(_swsContext, "threads", 0, AV_OPT_SEARCH_CHILDREN);
                r = sws_init_context(_swsContext, nullptr, nullptr);
                if (r < 0)
                {
                    throw std::runtime_error(string::Format("{0}: Cannot initialize sws context").arg(_fileName));
                }
            }
        }
        
        int ReadVideo::_decode(const otime::RationalTime& currentTime)
        {
            int out = 0;
            NDIlib_frame_type_e type =
                NDIlib_recv_capture_v2(pNDI_recv, &video_frame, nullptr,
                                       nullptr, 5000);
            switch(type)
            {
            case NDIlib_frame_type_video:
                std::cerr << "got video" << std::endl;
				NDIlib_recv_free_video_v2(pNDI_recv, &video_frame);
                break;
            case NDIlib_frame_type_audio:
                std::cerr << "ignoring audio" << std::endl;
                break;
            default:
                break;
            }
            return out;
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
                case AV_PIX_FMT_GRAY8:
                    for (std::size_t i = 0; i < h; ++i)
                    {
                        std::memcpy(
                            data + w * i,
                            data0 + linesize0 * i,
                            w);
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
                default: break;
                }
            }
            else
            {
                av_image_fill_arrays(
                    _avFrame2->data,
                    _avFrame2->linesize,
                    data,
                    _avOutputPixelFormat,
                    w,
                    h,
                    1);
                sws_scale(
                    _swsContext,
                    (uint8_t const* const*)_avFrame->data,
                    _avFrame->linesize,
                    0,
                    _avCodecParameters[_avStream]->height,
                    _avFrame2->data,
                    _avFrame2->linesize);
            }
        }
    }
}
