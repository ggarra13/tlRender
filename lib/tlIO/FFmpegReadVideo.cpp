// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlIO/FFmpegReadPrivate.h>

#include <tlCore/StringFormat.h>

extern "C"
{
#include <libavutil/display.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>

} // extern "C"

namespace tl
{
    namespace ffmpeg
    {
        ReadVideo::ReadVideo(
            const std::string& fileName,
            const std::vector<file::MemoryRead>& memory,
            const Options& options) :
            _fileName(fileName),
            _options(options)
        {
            if (!memory.empty())
            {
                _avFormatContext = avformat_alloc_context();
                if (!_avFormatContext)
                {
                    throw std::runtime_error(string::Format("{0}: Cannot allocate format context").arg(fileName));
                }

                _avIOBufferData = AVIOBufferData(memory[0].p, memory[0].size);
                _avIOContextBuffer = static_cast<uint8_t*>(av_malloc(avIOContextBufferSize));
                _avIOContext = avio_alloc_context(
                    _avIOContextBuffer,
                    avIOContextBufferSize,
                    0,
                    &_avIOBufferData,
                    &avIOBufferRead,
                    nullptr,
                    &avIOBufferSeek);
                if (!_avIOContext)
                {
                    throw std::runtime_error(string::Format("{0}: Cannot allocate I/O context").arg(fileName));
                }

                _avFormatContext->pb = _avIOContext;
            }

            int r = avformat_open_input(
                &_avFormatContext,
                !_avFormatContext ? fileName.c_str() : nullptr,
                nullptr,
                nullptr);
            if (r < 0)
            {
                throw std::runtime_error(string::Format("{0}: {1}").arg(fileName).arg(getErrorLabel(r)));
            }

            r = avformat_find_stream_info(_avFormatContext, nullptr);
            if (r < 0)
            {
                throw std::runtime_error(string::Format("{0}: {1}").arg(fileName).arg(getErrorLabel(r)));
            }
            for (unsigned int i = 0; i < _avFormatContext->nb_streams; ++i)
            {
                //av_dump_format(_avFormatContext, 0, fileName.c_str(), 0);

                if (AVMEDIA_TYPE_VIDEO == _avFormatContext->streams[i]->codecpar->codec_type &&
                    AV_DISPOSITION_DEFAULT == _avFormatContext->streams[i]->disposition)
                {
                    _avStream = i;
                    break;
                }
            }
            if (-1 == _avStream)
            {
                for (unsigned int i = 0; i < _avFormatContext->nb_streams; ++i)
                {
                    if (AVMEDIA_TYPE_VIDEO == _avFormatContext->streams[i]->codecpar->codec_type)
                    {
                        _avStream = i;
                        break;
                    }
                }
            }
            std::string timecode = getTimecodeFromDataStream(_avFormatContext);
            if (_avStream != -1)
            {
                //av_dump_format(_avFormatContext, _avStream, fileName.c_str(), 0);

                auto avVideoStream = _avFormatContext->streams[_avStream];
                auto avVideoCodecParameters = avVideoStream->codecpar;
                auto avVideoCodec = avcodec_find_decoder(avVideoCodecParameters->codec_id);
                
                // If we are reading VPX, use libvpx-vp9 external lib if available so
                // we can read an alpha channel.
                if (avVideoCodecParameters->codec_id == AV_CODEC_ID_VP9)
                {
                    auto avLibVpxCodec = avcodec_find_decoder_by_name("libvpx-vp9");
                    if (avLibVpxCodec)
                    {
                        avVideoCodec = avLibVpxCodec;
                        avVideoCodecParameters->codec_id = avVideoCodec->id;
                    }
                }
                
                if (!avVideoCodec)
                {
                    throw std::runtime_error(string::Format("{0}: No video codec found").arg(fileName));
                }
                _avCodecParameters[_avStream] = avcodec_parameters_alloc();
                if (!_avCodecParameters[_avStream])
                {
                    throw std::runtime_error(string::Format("{0}: Cannot allocate parameters").arg(fileName));
                }
                r = avcodec_parameters_copy(_avCodecParameters[_avStream], avVideoCodecParameters);
                if (r < 0)
                {
                    throw std::runtime_error(string::Format("{0}: {1}").arg(fileName).arg(getErrorLabel(r)));
                }
                _avCodecContext[_avStream] = avcodec_alloc_context3(avVideoCodec);
                if (!_avCodecParameters[_avStream])
                {
                    throw std::runtime_error(string::Format("{0}: Cannot allocate context").arg(fileName));
                }
                r = avcodec_parameters_to_context(_avCodecContext[_avStream], _avCodecParameters[_avStream]);
                if (r < 0)
                {
                    throw std::runtime_error(string::Format("{0}: {1}").arg(fileName).arg(getErrorLabel(r)));
                }
                _avCodecContext[_avStream]->thread_count = options.threadCount;
                _avCodecContext[_avStream]->thread_type = FF_THREAD_FRAME;
                r = avcodec_open2(_avCodecContext[_avStream], avVideoCodec, 0);
                if (r < 0)
                {
                    throw std::runtime_error(string::Format("{0}: {1}").arg(fileName).arg(getErrorLabel(r)));
                }

                _info.size.w = _avCodecParameters[_avStream]->width;
                _info.size.h = _avCodecParameters[_avStream]->height;
                if (_avCodecParameters[_avStream]->sample_aspect_ratio.den > 0 &&
                    _avCodecParameters[_avStream]->sample_aspect_ratio.num > 0)
                {
                    _info.size.pixelAspectRatio = av_q2d(_avCodecParameters[_avStream]->sample_aspect_ratio);
                }
                _info.layout.mirror.y = true;

                _avInputPixelFormat = static_cast<AVPixelFormat>(_avCodecParameters[_avStream]->format);

                // LibVPX return AV_PIX_FMT_YUV420P with metadata "alpha_mode" set to 1.
                AVDictionaryEntry* tag = nullptr;
                while ((tag = av_dict_get(avVideoStream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
                {
                    const std::string key(tag->key);
                    const std::string value(tag->value);
                    if (string::compare(
                        key,
                        "alpha_mode",
                        string::Compare::CaseSensitive))
                    {
                        if (value == "1" && _avInputPixelFormat == AV_PIX_FMT_YUV420P)
                        {
                            _avInputPixelFormat = AV_PIX_FMT_YUVA420P;
                        }
                    }
                }

                switch (_avInputPixelFormat)
                {
                case AV_PIX_FMT_RGB24:
                    _avOutputPixelFormat = _avInputPixelFormat;
                    _info.pixelType = image::PixelType::RGB_U8;
                    break;
                case AV_PIX_FMT_GRAY8:
                    _avOutputPixelFormat = _avInputPixelFormat;
                    _info.pixelType = image::PixelType::L_U8;
                    break;
                case AV_PIX_FMT_RGBA:
                    _avOutputPixelFormat = _avInputPixelFormat;
                    _info.pixelType = image::PixelType::RGBA_U8;
                    break;
                case AV_PIX_FMT_YUV420P:
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
                case AV_PIX_FMT_YUV422P:
                    if (options.yuvToRGBConversion)
                    {
                        _avOutputPixelFormat = AV_PIX_FMT_RGB24;
                        _info.pixelType = image::PixelType::RGB_U8;
                    }
                    else
                    {
                        _avOutputPixelFormat = _avInputPixelFormat;
                        _info.pixelType = image::PixelType::YUV_422P_U8;
                    }
                    break;
                case AV_PIX_FMT_YUV444P:
                    if (options.yuvToRGBConversion)
                    {
                        _avOutputPixelFormat = AV_PIX_FMT_RGB24;
                        _info.pixelType = image::PixelType::RGB_U8;
                    }
                    else
                    {
                        _avOutputPixelFormat = _avInputPixelFormat;
                        _info.pixelType = image::PixelType::YUV_444P_U8;
                    }
                    break;
                case AV_PIX_FMT_YUV420P10BE:
                case AV_PIX_FMT_YUV420P10LE:
                case AV_PIX_FMT_YUV420P12BE:
                case AV_PIX_FMT_YUV420P12LE:
                case AV_PIX_FMT_YUV420P16BE:
                case AV_PIX_FMT_YUV420P16LE:
                    if (options.yuvToRGBConversion)
                    {
                        _avOutputPixelFormat = AV_PIX_FMT_RGB48;
                        _info.pixelType = image::PixelType::RGB_U16;
                    }
                    else
                    {
                        //! \todo Use the _info.layout.endian field instead of
                        //! converting endianness.
                        _avOutputPixelFormat = AV_PIX_FMT_YUV420P16LE;
                        _info.pixelType = image::PixelType::YUV_420P_U16;
                    }
                    break;
                case AV_PIX_FMT_YUV422P10BE:
                case AV_PIX_FMT_YUV422P10LE:
                case AV_PIX_FMT_YUV422P12BE:
                case AV_PIX_FMT_YUV422P12LE:
                case AV_PIX_FMT_YUV422P16BE:
                case AV_PIX_FMT_YUV422P16LE:
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
                case AV_PIX_FMT_YUV444P10BE:
                case AV_PIX_FMT_YUV444P10LE:
                case AV_PIX_FMT_YUV444P12BE:
                case AV_PIX_FMT_YUV444P12LE:
                case AV_PIX_FMT_YUV444P16BE:
                case AV_PIX_FMT_YUV444P16LE:
                    if (options.yuvToRGBConversion)
                    {
                        _avOutputPixelFormat = AV_PIX_FMT_RGB48;
                        _info.pixelType = image::PixelType::RGB_U16;
                    }
                    else
                    {
                        //! \todo Use the _info.layout.endian field instead of
                        //! converting endianness.
                        _avOutputPixelFormat = AV_PIX_FMT_YUV444P16LE;
                        _info.pixelType = image::PixelType::YUV_444P_U16;
                    }
                    break;
                case AV_PIX_FMT_YUVA420P:
                case AV_PIX_FMT_YUVA422P:
                case AV_PIX_FMT_YUVA444P:
                    //! \todo Support these formats natively.
                    _avOutputPixelFormat = AV_PIX_FMT_RGBA;
                    _info.pixelType = image::PixelType::RGBA_U8;
                    break;
                case AV_PIX_FMT_YUVA444P10BE:
                case AV_PIX_FMT_YUVA444P10LE:
                case AV_PIX_FMT_YUVA444P12BE:
                case AV_PIX_FMT_YUVA444P12LE:
                case AV_PIX_FMT_YUVA444P16BE:
                case AV_PIX_FMT_YUVA444P16LE:
                    //! \todo Support these formats natively.
                    _avOutputPixelFormat = AV_PIX_FMT_RGBA64;
                    _info.pixelType = image::PixelType::RGBA_U16;
                    break;
                default:
                    if (options.yuvToRGBConversion)
                    {
                        _avOutputPixelFormat = AV_PIX_FMT_RGB24;
                        _info.pixelType = image::PixelType::RGB_U8;
                    }
                    else
                    {
                        _avOutputPixelFormat = AV_PIX_FMT_YUV420P;
                        _info.pixelType = image::PixelType::YUV_420P_U8;
                    }
                    break;
                }
                if (_avCodecContext[_avStream]->color_range != AVCOL_RANGE_JPEG)
                {
                    _info.videoLevels = image::VideoLevels::LegalRange;
                }
                switch (_avCodecParameters[_avStream]->color_space)
                {
                case AVCOL_SPC_BT2020_NCL:
                    _info.yuvCoefficients = image::YUVCoefficients::BT2020;
                    break;
                default: break;
                }

                _avSpeed = avVideoStream->r_frame_rate;
                // Use avg_frame_rate if set
                if (avVideoStream->avg_frame_rate.num != 0 &&
                    avVideoStream->avg_frame_rate.den != 0)
                    _avSpeed = avVideoStream->avg_frame_rate;
                
                const double speed = av_q2d(_avSpeed);

                std::size_t sequenceSize = 0;
                if (avVideoStream->nb_frames > 0)
                {
                    sequenceSize = avVideoStream->nb_frames;
                }
                else if (avVideoStream->duration != AV_NOPTS_VALUE)
                {
                    sequenceSize = av_rescale_q(
                        avVideoStream->duration,
                        avVideoStream->time_base,
                        swap(avVideoStream->r_frame_rate));
                }
                else if (_avFormatContext->duration != AV_NOPTS_VALUE)
                {
                    sequenceSize = av_rescale_q(
                        _avFormatContext->duration,
                        av_get_time_base_q(),
                        swap(avVideoStream->r_frame_rate));
                }
        
                image::Tags tags;
                while ((tag = av_dict_get(_avFormatContext->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
                {
                    const std::string key(tag->key);
                    const std::string value(tag->value);
                    tags[key] = value;
                    if (string::compare(
                        key,
                        "timecode",
                        string::Compare::CaseInsensitive))
                    {
                        timecode = value;
                    }
                }

                otime::RationalTime startTime(0.0, speed);
                if (!time::compareExact(_options.startTime, time::invalidTime))
                {
                    startTime = _options.startTime;
                }
                else if (!timecode.empty())
                {
                    otime::ErrorStatus errorStatus;
                    const otime::RationalTime time = otime::RationalTime::from_timecode(
                        timecode,
                        speed,
                        &errorStatus);
                    if (!otime::is_error(errorStatus))
                    {
                        startTime = time::floor(time);
                    }
                }
                _timeRange = otime::TimeRange(
                    startTime,
                    otime::RationalTime(sequenceSize, speed));

                for (const auto& i : tags)
                {
                    _tags[i.first] = i.second;
                }
                _rotation = _getRotation(avVideoStream);
                if (_rotation != 0.F)
                {
                    std::stringstream ss;
                    ss << std::fixed;
                    ss << _rotation;
                    _tags["Video Rotation"] = ss.str();
                }
                {
                    std::stringstream ss;
                    ss << _info.size.w << " " << _info.size.h;
                    _tags["Video Resolution"] = ss.str();
                }
                {
                    std::stringstream ss;
                    ss.precision(2);
                    ss << std::fixed;
                    ss << _info.size.pixelAspectRatio;
                    _tags["Video Pixel Aspect Ratio"] = ss.str();
                }
                {
                    std::stringstream ss;
                    ss << _info.pixelType;
                    _tags["Video Pixel Type"] = ss.str();
                }
                {
                    _tags["Video Codec"] =
                        avcodec_get_name(_avCodecContext[_avStream]->codec_id);
                }
                {
                    std::stringstream ss;
                    ss << _info.videoLevels;
                    _tags["Video Levels"] = ss.str();
                }
                {
                    std::stringstream ss;
                    ss << _timeRange.start_time().to_timecode();
                    _tags["Video Start Time"] = ss.str();
                }
                {
                    std::stringstream ss;
                    ss << _timeRange.duration().to_timecode();
                    _tags["Video Duration"] = ss.str();
                }
                {
                    std::stringstream ss;
                    ss.precision(2);
                    ss << std::fixed;
                    ss << _timeRange.start_time().rate() << " FPS";
                    _tags["Video Speed"] = ss.str();
                }
            }
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
            for (auto i : _avCodecContext)
            {
                avcodec_close(i.second);
                avcodec_free_context(&i.second);
            }
            for (auto i : _avCodecParameters)
            {
                avcodec_parameters_free(&i.second);
            }
            if (_avIOContext)
            {
                avio_context_free(&_avIOContext);
            }
            //! \bug Free'd by avio_context_free()?
            //if (_avIOContextBuffer)
            //{
            //    av_free(_avIOContextBuffer);
            //}
            if (_avFormatContext)
            {
                avformat_close_input(&_avFormatContext);
            }
        }

        bool ReadVideo::isValid() const
        {
            return _avStream != -1;
        }

        const image::Info& ReadVideo::getInfo() const
        {
            return _info;
        }

        const otime::TimeRange& ReadVideo::getTimeRange() const
        {
            return _timeRange;
        }

        const image::Tags& ReadVideo::getTags() const
        {
            return _tags;
        }

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

        void ReadVideo::start()
        {
            if (_avStream != -1)
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

                    /*_swsContext = sws_getContext(
                        _avCodecParameters[_avStream]->width,
                        _avCodecParameters[_avStream]->height,
                        _avInputPixelFormat,
                        _avCodecParameters[_avStream]->width,
                        _avCodecParameters[_avStream]->height,
                        _avOutputPixelFormat,
                        swsScaleFlags,
                        0,
                        0,
                        0);
                    if (!_swsContext)
                    {
                        throw std::runtime_error(string::Format("{0}: Cannot get context").arg(_fileName));
                    }*/
                    _swsContext = sws_alloc_context();
                    if (!_swsContext)
                    {
                        throw std::runtime_error(string::Format("{0}: Cannot allocate context").arg(_fileName));
                    }
                    av_opt_set_defaults(_swsContext);
                    int r = av_opt_set_int(_swsContext, "srcw", _avCodecParameters[_avStream]->width, AV_OPT_SEARCH_CHILDREN);
                    r = av_opt_set_int(_swsContext, "srch", _avCodecParameters[_avStream]->height, AV_OPT_SEARCH_CHILDREN);
                    r = av_opt_set_int(_swsContext, "src_format", _avInputPixelFormat, AV_OPT_SEARCH_CHILDREN);
                    r = av_opt_set_int(_swsContext, "dstw", _avCodecParameters[_avStream]->width, AV_OPT_SEARCH_CHILDREN);
                    r = av_opt_set_int(_swsContext, "dsth", _avCodecParameters[_avStream]->height, AV_OPT_SEARCH_CHILDREN);
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
        }

        void ReadVideo::seek(const otime::RationalTime& time)
        {
            //std::cout << "video seek: " << time << std::endl;

            if (_avStream != -1)
            {
                avcodec_flush_buffers(_avCodecContext[_avStream]);

                if (av_seek_frame(
                    _avFormatContext,
                    _avStream,
                    av_rescale_q(
                        time.value() - _timeRange.start_time().value(),
                        swap(_avSpeed),
                        _avFormatContext->streams[_avStream]->time_base),
                    AVSEEK_FLAG_BACKWARD) < 0)
                {
                    //! \todo How should this be handled?
                }
            }

            _buffer.clear();
            _eof = false;
        }

        bool ReadVideo::process(const otime::RationalTime& currentTime)
        {
            bool out = false;
            if (_avStream != -1 &&
                _buffer.size() < _options.videoBufferSize)
            {
                Packet packet;
                int decoding = 0;
                while (0 == decoding)
                {
                    if (!_eof)
                    {
                        decoding = av_read_frame(_avFormatContext, packet.p);
                        if (AVERROR_EOF == decoding)
                        {
                            _eof = true;
                            decoding = 0;
                        }
                        else if (decoding < 0)
                        {
                            //! \todo How should this be handled?
                            break;
                        }
                    }
                    if ((_eof && _avStream != -1) || (_avStream == packet.p->stream_index))
                    {
                        decoding = avcodec_send_packet(
                            _avCodecContext[_avStream],
                            _eof ? nullptr : packet.p);
                        if (AVERROR_EOF == decoding)
                        {
                            decoding = 0;
                        }
                        else if (decoding < 0)
                        {
                            //! \todo How should this be handled?
                            break;
                        }
                        decoding = _decode(currentTime);
                        if (AVERROR(EAGAIN) == decoding)
                        {
                            decoding = 0;
                        }
                        else if (AVERROR_EOF == decoding)
                        {
                            break;
                        }
                        else if (decoding < 0)
                        {
                            //! \todo How should this be handled?
                            break;
                        }
                        else if (1 == decoding)
                        {
                            out = true;
                            break;
                        }
                    }
                    if (packet.p->buf)
                    {
                        av_packet_unref(packet.p);
                    }
                }
                if (packet.p->buf)
                {
                    av_packet_unref(packet.p);
                }
                //std::cout << "video buffer size: " << _buffer.size() << std::endl;
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

        int ReadVideo::_decode(const otime::RationalTime& currentTime)
        {
            int out = 0;
            while (0 == out)
            {
                out = avcodec_receive_frame(_avCodecContext[_avStream], _avFrame);
                if (out < 0)
                {
                    return out;
                }
                const int64_t timestamp = _avFrame->pts != AV_NOPTS_VALUE ? _avFrame->pts : _avFrame->pkt_dts;
                //std::cout << "video timestamp: " << timestamp << std::endl;

                const otime::RationalTime time(
                    _timeRange.start_time().value() +
                    av_rescale_q(
                        timestamp,
                        _avFormatContext->streams[_avStream]->time_base,
                        swap(_avFormatContext->streams[_avStream]->r_frame_rate)),
                    _timeRange.duration().rate());
                //std::cout << "video time: " << time << std::endl;

                if (time >= currentTime)
                {
                    //std::cout << "video time: " << time << std::endl;
                    auto image = image::Image::create(_info);
                    
                    auto tags = _tags;
                    AVDictionaryEntry* tag = nullptr;
                    while ((tag = av_dict_get(_avFrame->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
                    {
                        tags[tag->key] = tag->value;
                    }
                    image::HDRData hdrData;
                    toHDRData(_avFrame->side_data, _avFrame->nb_side_data, hdrData);
                    tags["hdr"] = nlohmann::json(hdrData).dump();
                    image->setTags(tags);

                    _copy(image);
                    _buffer.push_back(image);
                    out = 1;
                    break;
                }
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
                    if (_rotation != 0.F)
                    {
                        auto info = _info;
                        if (std::fabs(_rotation) != 180.F)
                        {
                            const auto tmp = info.size.w;
                            info.size.w = info.size.h;
                            info.size.h = tmp;
                        }
                        auto transposedImage = image::Image::create(info);
                        if (std::fabs(_rotation) == 180.F)
                        {
                            _flipYUV420P(transposedImage->getData(), data);
                        }
                        else
                        {
                            bool clockwise = true;
                            if (_rotation == 90.F ||
                                _rotation == -270.F)
                                clockwise = false;
                            _transposeYUV420P(
                                transposedImage->getData(), data, clockwise);
                        }
                        image = transposedImage;
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
        
        float ReadVideo::_getRotation(const AVStream* st)
        {
            float out = 0.F;
 
            uint8_t* displaymatrix =
                av_stream_get_side_data(st, AV_PKT_DATA_DISPLAYMATRIX, NULL);
            if (displaymatrix)
            {
                out = av_display_rotation_get(
                    reinterpret_cast<int32_t*>(displaymatrix));
            }
            return out;
        }
        
        void ReadVideo::_transposeYUV420P(uint8_t* out, const uint8_t* in,
                                          const bool clockwise)
        {
            const size_t w = _info.size.w;
            const size_t h = _info.size.h;
            const size_t w2 = w / 2;
            const size_t h2 = h / 2;
            const size_t size = w * h;
            const size_t size4 = size / 4;

            if (clockwise)
            {
                // Transpose Y plane
                for (size_t y = 0; y < h; ++y) {
                    for (size_t x = 0; x < w; ++x) {
                        out[x * h + ( h - 1 - y )] = in[y * w + x];
                    }
                }

                // Transpose U and V planes (downsampling)
                for (size_t y = 0; y < h / 2; ++y)
                {
                    for (size_t x = 0; x < w / 2; ++x)
                    {
                        out[size + x * h2 + (h2 - y - 1)] =
                            in[size + y * w2 + x];
                        out[size + size4 + x * h2 + (h2 - y - 1)] =
                            in[size + size4 + y * w2 + x];
                    }
                }
            }
            else
            {
                // Transpose Y plane counterclockwise
                for (size_t y = 0; y < h; ++y)
                {
                    for (size_t x = 0; x < w; ++x)
                    {
                        out[(w - 1 - x) * h + y] = in[y * w + x];
                    }
                }

                // Transpose U and V planes (downsampling) counterclockwise
                for (size_t y = 0; y < h2; ++y)
                {
                    for (size_t x = 0; x < w2; ++x)
                    {
                        out[size + (w2 - 1 - x) * h2 + y] =
                            in[size + x * h2 + y];
                        out[size + size4 + (w2 - 1 - x) * h2 + y] =
                            in[size + size4 + y * w2 + x];
                    }
                }
            }
        }

        void ReadVideo::_flipYUV420P(uint8_t* out, const uint8_t* in)
        {
            // Calculate the size of each plane
            const size_t w = _info.size.w;
            const size_t h = _info.size.h;
            const size_t w2 = w / 2;
            const size_t h2 = h / 2;
            const size_t size = w * h;
            const size_t size4 = size / 4;

            // Flip Y plane vertically
            for (size_t y = 0; y < h; ++y)
            {
                for (size_t x = 0; x < w; ++x)
                {
                    out[(h - 1 - y) * w + (w - 1 - x)] = in[y * w + x];
                }
            }

            // Flip U and V planes vertically
            for (size_t y = 0; y < h2; ++y)
            {
                for (size_t x = 0; x < w2; ++x)
                {
                    out[size + y * w2 + x] =
                        in[size + (h2 - 1 - y) * w2 + (w2 - 1 - x)];
                    out[size + size4 + y * w2 + x] =
                        in[size + size4 + (h2 - 1 - y) * w2 + (w2 - 1 - x)];
                }
            }
        }
    }
}
