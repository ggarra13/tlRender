// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <cassert>

#include <tlIO/FFmpeg.h>

#include <tlCore/StringFormat.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/audio_fifo.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
    
#include <libavutil/timestamp.h>
}

namespace tl
{
    namespace ffmpeg
    {
        namespace
        {

            void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt,
                bool audio = false)
            {
                std::cerr << "pkt->stream_index=" << pkt->stream_index;
                if (audio)
                    std::cerr << " audio" << std::endl;
                else
                    std::cerr << " video" << std::endl;
                AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

                char buf[AV_TS_MAX_STRING_SIZE];
                char buf2[AV_TS_MAX_STRING_SIZE];

                fprintf( stderr, "pts:%s pts_time:%s ",
                       av_ts_make_string(buf, pkt->pts),
                       av_ts_make_time_string(buf2, pkt->pts, time_base) );

                fprintf( stderr, "dts:%s dts_time:%s ",
                        av_ts_make_string(buf, pkt->dts),
                        av_ts_make_time_string(buf2, pkt->dts, time_base) );

                fprintf( stderr, "duration:%s duration_time:%s stream_index:%d\n",
                        av_ts_make_string(buf, pkt->duration),
                        av_ts_make_time_string(buf2, pkt->duration, time_base),
                        pkt->stream_index);
            }

            AVSampleFormat toPlanarFormat(const enum AVSampleFormat s)
            {
                switch(s)
                {
                case AV_SAMPLE_FMT_U8:
                    return AV_SAMPLE_FMT_U8P;
                case AV_SAMPLE_FMT_S16:
                    return AV_SAMPLE_FMT_S16P;
                case AV_SAMPLE_FMT_S32:
                    return AV_SAMPLE_FMT_S32P;
                case AV_SAMPLE_FMT_FLT:
                    return AV_SAMPLE_FMT_FLTP;
                case AV_SAMPLE_FMT_DBL:
                    return AV_SAMPLE_FMT_DBLP;
                default:
                    return s;
                }
            }
            
            //! Check that a given sample format is supported by the encoder
            bool check_sample_fmt(const AVCodec *codec, enum AVSampleFormat sample_fmt)
            {
                const enum AVSampleFormat *p = codec->sample_fmts;

                while (*p != AV_SAMPLE_FMT_NONE) {
                    if (*p == sample_fmt)
                        return true;
                    p++;
                }
                return false;
            }

            //! Select layout with the highest channel count
            //! Borrowed from FFmpeg's examples
            int select_channel_layout(const AVCodec *codec,
                                      AVChannelLayout *dst)
            {
                const AVChannelLayout *p, *best_ch_layout;
                int best_nb_channels   = 0;
                
                if (!codec->ch_layouts)
                {
                    AVChannelLayout stereoLayout = AV_CHANNEL_LAYOUT_STEREO;
                    return av_channel_layout_copy(dst, &stereoLayout);
                }

                p = codec->ch_layouts;
                while (p->nb_channels) {
                    int nb_channels = p->nb_channels;
                    
                    if (nb_channels > best_nb_channels) {
                        best_ch_layout   = p;
                        best_nb_channels = nb_channels;
                    }
                    p++;
                }
                return av_channel_layout_copy(dst, best_ch_layout);
            }
            
            //! Return an equal or higher supported samplerate
            int select_sample_rate(const AVCodec* codec, const int sampleRate)
            {
                const int *p;
                int best_samplerate = 0;

                if (!codec->supported_samplerates)
                    return 44100;

                p = codec->supported_samplerates;
                while (*p) {

                    if (*p == sampleRate)
                    {
                        std::cerr << "Equal sample rate= " << sampleRate
                                  << std::endl;
                        return sampleRate;
                    }
                    
                    if (!best_samplerate || abs(44100 - *p) < abs(44100 - best_samplerate))
                        best_samplerate = *p;
                    p++;
                }
                std::cerr << "different sample rate was " << sampleRate
                          << " but found " << best_samplerate << std::endl;
                return best_samplerate;
            }
        }
        
        struct Write::Private
        {
            std::string fileName;
            AVFormatContext* avFormatContext = nullptr;

            // Video
            AVCodecContext* avCodecContext = nullptr;
            AVStream* avVideoStream = nullptr;
            AVPacket* avPacket = nullptr;
            AVFrame* avFrame = nullptr;
            AVPixelFormat avPixelFormatIn = AV_PIX_FMT_NONE;
            AVFrame* avFrame2 = nullptr;
            SwsContext* swsContext = nullptr;

            // Audio
            AVCodecContext* avAudioCodecContext = nullptr;
            AVStream* avAudioStream = nullptr;
            AVAudioFifo* avAudioFifo = nullptr;
            AVFrame* avAudioFrame = nullptr;
            AVPacket* avAudioPacket = nullptr;
            uint64_t totalSamples = 0;

            otime::RationalTime lastTime = time::invalidTime;
            
            bool opened = false;
        };

        void Write::_init(
            const file::Path& path,
            const io::Info& info,
            const io::Options& options,
            const std::weak_ptr<log::System>& logSystem)
        {
            IWrite::_init(path, options, info, logSystem);

            TLRENDER_P();

            p.fileName = path.get();
            if (info.video.empty())
            {
                throw std::runtime_error(string::Format("{0}: No video").arg(p.fileName));
            }
            
            int r = avformat_alloc_output_context2(&p.avFormatContext, NULL, NULL, p.fileName.c_str());
            
            // p.avFormatContext->flags |= AVFMT_FLAG_NOBUFFER|AVFMT_FLAG_FLUSH_PACKETS;
            // p.avFormatContext->max_interleave_delta = 1;
    
            if (info.audio.isValid())
            {
                AVCodecID avCodecID = AV_CODEC_ID_AAC; // don't hard code this
                const AVCodec* avCodec = avcodec_find_encoder(avCodecID);
                if (!avCodec)
                    throw std::runtime_error("Could not find audio encoder");
                
                p.avAudioStream = avformat_new_stream(p.avFormatContext,
                                                      avCodec);
                if (!p.avAudioStream)
                {
                    throw std::runtime_error(
                        string::Format("{0}: Cannot allocate audio stream")
                            .arg(p.fileName));
                }

                p.avAudioStream->id = p.avFormatContext->nb_streams - 1;
                
                p.avAudioCodecContext = avcodec_alloc_context3(avCodec);
                if (!p.avAudioCodecContext)
                {
                    throw std::runtime_error(
                        string::Format(
                            "{0}: Cannot allocate audio codec context")
                            .arg(p.fileName));
                }
                
                // p.avAudioCodecContext->audio_service_type = AV_AUDIO_SERVICE_TYPE_MAIN;
                p.avAudioCodecContext->sample_fmt =
                    toPlanarFormat(fromAudioType(info.audio.dataType));
                if (!check_sample_fmt(avCodec, p.avAudioCodecContext->sample_fmt))
                {
                    p.avAudioCodecContext->sample_fmt = AV_SAMPLE_FMT_S16;
                    if (!check_sample_fmt(avCodec, p.avAudioCodecContext->sample_fmt))
                    {
                        throw std::runtime_error(
                            string::Format("Sample format {0} not supported!")
                                .arg(av_get_sample_fmt_name(
                                    p.avAudioCodecContext->sample_fmt)));
                    }
                }
                p.avAudioCodecContext->bit_rate = 69000; // @todo:
                p.avAudioCodecContext->sample_rate = info.audio.sampleRate;

                r = select_channel_layout(
                    avCodec, &p.avAudioCodecContext->ch_layout);
                if (r < 0)
                    throw std::runtime_error(
                        string::Format(
                            "{0}: Could not select audio channel layout")
                            .arg(p.fileName));

                select_sample_rate(avCodec, info.audio.sampleRate);
                
                p.avAudioCodecContext->time_base.num = 1;
                p.avAudioCodecContext->time_base.den = info.audio.sampleRate;

                if (p.avAudioCodecContext->codec->capabilities &
                    AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
                {
                    p.avAudioCodecContext->frame_size = 10000;
                }
                else
                {
                    p.avAudioCodecContext->frame_size = info.audio.sampleRate;
                }
    
                if ((p.avAudioCodecContext->block_align == 1 ||
                     p.avAudioCodecContext->block_align == 1152 ||
                     p.avAudioCodecContext->block_align == 576) &&
                    p.avAudioCodecContext->codec_id == AV_CODEC_ID_MP3)
                    p.avAudioCodecContext->block_align = 0;

                if (avCodecID == AV_CODEC_ID_AC3)
                    p.avAudioCodecContext->block_align = 0;

                r = avcodec_open2(p.avAudioCodecContext, avCodec, NULL);
                if (r < 0)
                {
                    throw std::runtime_error(
                        string::Format("{0}: Could not open audio codec - {1}.")
                            .arg(p.fileName)
                            .arg(getErrorLabel(r)));
                }

                r = avcodec_parameters_from_context(
                    p.avAudioStream->codecpar, p.avAudioCodecContext);
                if (r < 0)
                {
                    throw std::runtime_error(
                        string::Format(
                            "{0}: Could not copy parameters from context - {1}.")
                            .arg(p.fileName)
                            .arg(getErrorLabel(r)));
                }
    
                p.avAudioPacket = av_packet_alloc();
                if (!p.avAudioPacket)
                {
                    throw std::runtime_error(
                        string::Format("{0}: Cannot allocate audio packet")
                            .arg(p.fileName));
                }
            
                p.avAudioFifo = av_audio_fifo_alloc(
                    p.avAudioCodecContext->sample_fmt, info.audio.channelCount,
                    1);
                if (!p.avAudioFifo)
                {
                    throw std::runtime_error(
                        string::Format(
                            "{0}: Cannot allocate audio FIFO buffer - {1}.")
                        .arg(p.fileName)
                        .arg(getErrorLabel(r)));
                }
                    
                p.avAudioFrame = av_frame_alloc();
                if (!p.avAudioFrame)
                {
                    throw std::runtime_error(
                        string::Format("{0}: Cannot allocate audio frame - {1}.")
                        .arg(p.fileName)
                        .arg(getErrorLabel(r)));
                }

                p.avAudioFrame->nb_samples = p.avAudioCodecContext->frame_size;
                p.avAudioFrame->format = p.avAudioCodecContext->sample_fmt;
                p.avAudioFrame->sample_rate = info.audio.sampleRate;
                r = av_channel_layout_copy(
                    &p.avAudioFrame->ch_layout,
                    &p.avAudioCodecContext->ch_layout);
                if (r < 0)
                {
                    throw std::runtime_error(
                        string::Format("{0}: Could not copy channel layout to "
                                       "audio frame - {1}.")
                        .arg(p.fileName)
                        .arg(getErrorLabel(r)));
                }
                
                /* allocate the data buffers */
                r = av_frame_get_buffer(p.avAudioFrame, 0);
                if (r < 0)
                {
                    throw std::runtime_error(
                        string::Format("{0}: Could not allocate buffer for "
                                       "audio frame - {1}.")
                        .arg(p.fileName)
                        .arg(getErrorLabel(r)));
                }
            }

            AVCodecID avCodecID = AV_CODEC_ID_MPEG4;
            Profile profile = Profile::None;
            int avProfile = FF_PROFILE_UNKNOWN;
            auto option = options.find("FFmpeg/WriteProfile");
            if (option != options.end())
            {
                std::stringstream ss(option->second);
                ss >> profile;
            }
            switch (profile)
            {
            case Profile::H264:
                avCodecID = AV_CODEC_ID_H264;
                avProfile = FF_PROFILE_H264_HIGH;
                break;
            case Profile::ProRes:
                avCodecID = AV_CODEC_ID_PRORES;
                avProfile = FF_PROFILE_PRORES_STANDARD;
                break;
            case Profile::ProRes_Proxy:
                avCodecID = AV_CODEC_ID_PRORES;
                avProfile = FF_PROFILE_PRORES_PROXY;
                break;
            case Profile::ProRes_LT:
                avCodecID = AV_CODEC_ID_PRORES;
                avProfile = FF_PROFILE_PRORES_LT;
                break;
            case Profile::ProRes_HQ:
                avCodecID = AV_CODEC_ID_PRORES;
                avProfile = FF_PROFILE_PRORES_HQ;
                break;
            case Profile::ProRes_4444:
                avCodecID = AV_CODEC_ID_PRORES;
                avProfile = FF_PROFILE_PRORES_4444;
                break;
            case Profile::ProRes_XQ:
                avCodecID = AV_CODEC_ID_PRORES;
                avProfile = FF_PROFILE_PRORES_XQ;
                break;
            default: break;
            }

            if (r < 0)
            {
                throw std::runtime_error(string::Format("{0}: {1}").arg(p.fileName).arg(getErrorLabel(r)));
            }
            const AVCodec* avCodec = avcodec_find_encoder(avCodecID);
            if (!avCodec)
            {
                throw std::runtime_error(string::Format("{0}: Cannot find encoder").arg(p.fileName));
            }
            p.avCodecContext = avcodec_alloc_context3(avCodec);
            if (!p.avCodecContext)
            {
                throw std::runtime_error(string::Format("{0}: Cannot allocate context").arg(p.fileName));
            }
            p.avVideoStream = avformat_new_stream(p.avFormatContext, avCodec);
            if (!p.avVideoStream)
            {
                throw std::runtime_error(string::Format("{0}: Cannot allocate stream").arg(p.fileName));
            }
            p.avVideoStream->id = p.avFormatContext->nb_streams - 1;
            if (!avCodec->pix_fmts)
            {
                throw std::runtime_error(string::Format("{0}: No pixel formats available").arg(p.fileName));
            }

            p.avCodecContext->codec_id = avCodec->id;
            p.avCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;
            const auto& videoInfo = info.video[0];
            p.avCodecContext->width = videoInfo.size.w;
            p.avCodecContext->height = videoInfo.size.h;
            p.avCodecContext->sample_aspect_ratio = AVRational({ 1, 1 });
            p.avCodecContext->pix_fmt = avCodec->pix_fmts[0];
            const auto rational = time::toRational(info.videoTime.duration().rate());
            p.avCodecContext->time_base = { rational.second, rational.first };
            p.avCodecContext->framerate = { rational.first, rational.second };
            p.avCodecContext->profile = avProfile;
            if (p.avFormatContext->oformat->flags & AVFMT_GLOBALHEADER)
            {
                p.avCodecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            }
            p.avCodecContext->thread_count = 0;
            p.avCodecContext->thread_type = FF_THREAD_FRAME;

            r = avcodec_open2(p.avCodecContext, avCodec, NULL);
            if (r < 0)
            {
                throw std::runtime_error(string::Format("{0}: {1}").arg(p.fileName).arg(getErrorLabel(r)));
            }

            r = avcodec_parameters_from_context(p.avVideoStream->codecpar, p.avCodecContext);
            if (r < 0)
            {
                throw std::runtime_error(string::Format("{0}: {1}").arg(p.fileName).arg(getErrorLabel(r)));
            }

            p.avVideoStream->time_base = { rational.second, rational.first };
            p.avVideoStream->avg_frame_rate = { rational.first, rational.second };

            for (const auto& i : info.tags)
            {
                av_dict_set(&p.avFormatContext->metadata, i.first.c_str(), i.second.c_str(), 0);
            }

            //av_dump_format(p.avFormatContext, 0, p.fileName.c_str(), 1);

            r = avio_open(&p.avFormatContext->pb, p.fileName.c_str(), AVIO_FLAG_WRITE);
            if (r < 0)
            {
                throw std::runtime_error(string::Format("{0}: {1}").arg(p.fileName).arg(getErrorLabel(r)));
            }

            r = avformat_write_header(p.avFormatContext, NULL);
            if (r < 0)
            {
                throw std::runtime_error(string::Format("{0}: {1}").arg(p.fileName).arg(getErrorLabel(r)));
            }

            p.avPacket = av_packet_alloc();
            if (!p.avPacket)
            {
                throw std::runtime_error(string::Format("{0}: Cannot allocate packet").arg(p.fileName));
            }

            p.avFrame = av_frame_alloc();
            if (!p.avFrame)
            {
                throw std::runtime_error(string::Format("{0}: Cannot allocate frame").arg(p.fileName));
            }
            p.avFrame->format = p.avVideoStream->codecpar->format;
            p.avFrame->width = p.avVideoStream->codecpar->width;
            p.avFrame->height = p.avVideoStream->codecpar->height;
            r = av_frame_get_buffer(p.avFrame, 0);
            if (r < 0)
            {
                throw std::runtime_error(string::Format("{0}: {1}").arg(p.fileName).arg(getErrorLabel(r)));
            }

            p.avFrame2 = av_frame_alloc();
            if (!p.avFrame2)
            {
                throw std::runtime_error(string::Format("{0}: Cannot allocate frame").arg(p.fileName));
            }
            switch (videoInfo.pixelType)
            {
            case image::PixelType::L_U8:     p.avPixelFormatIn = AV_PIX_FMT_GRAY8;  break;
            case image::PixelType::RGB_U8:   p.avPixelFormatIn = AV_PIX_FMT_RGB24;  break;
            case image::PixelType::RGBA_U8:  p.avPixelFormatIn = AV_PIX_FMT_RGBA;   break;
            case image::PixelType::L_U16:    p.avPixelFormatIn = AV_PIX_FMT_GRAY16; break;
            case image::PixelType::RGB_U16:  p.avPixelFormatIn = AV_PIX_FMT_RGB48;  break;
            case image::PixelType::RGBA_U16: p.avPixelFormatIn = AV_PIX_FMT_RGBA64; break;
            default:
                throw std::runtime_error(string::Format("{0}: Incompatible pixel type").arg(p.fileName));
                break;
            }
            /*p.swsContext = sws_getContext(
                videoInfo.size.w,
                videoInfo.size.h,
                p.avPixelFormatIn,
                videoInfo.size.w,
                videoInfo.size.h,
                p.avCodecContext->pix_fmt,
                swsScaleFlags,
                0,
                0,
                0);*/
            p.swsContext = sws_alloc_context();
            if (!p.swsContext)
            {
                throw std::runtime_error(string::Format("{0}: Cannot allocate context").arg(p.fileName));
            }
            av_opt_set_defaults(p.swsContext);
            r = av_opt_set_int(p.swsContext, "srcw", videoInfo.size.w, AV_OPT_SEARCH_CHILDREN);
            r = av_opt_set_int(p.swsContext, "srch", videoInfo.size.h, AV_OPT_SEARCH_CHILDREN);
            r = av_opt_set_int(p.swsContext, "src_format", p.avPixelFormatIn, AV_OPT_SEARCH_CHILDREN);
            r = av_opt_set_int(p.swsContext, "dstw", videoInfo.size.w, AV_OPT_SEARCH_CHILDREN);
            r = av_opt_set_int(p.swsContext, "dsth", videoInfo.size.h, AV_OPT_SEARCH_CHILDREN);
            r = av_opt_set_int(p.swsContext, "dst_format", p.avCodecContext->pix_fmt, AV_OPT_SEARCH_CHILDREN);
            r = av_opt_set_int(p.swsContext, "sws_flags", swsScaleFlags, AV_OPT_SEARCH_CHILDREN);
            r = av_opt_set_int(p.swsContext, "threads", 0, AV_OPT_SEARCH_CHILDREN);
            r = sws_init_context(p.swsContext, nullptr, nullptr);
            if (r < 0)
            {
                throw std::runtime_error(string::Format("{0}: Cannot initialize sws context").arg(p.fileName));
            }

            p.opened = true;
        }

        Write::Write() :
            _p(new Private)
        {}

        Write::~Write()
        {
            TLRENDER_P();
            
            if (p.opened)
            {
                if (p.avAudioFifo)
                    _flushAudio();
                _encodeAudio(nullptr);
                _encodeVideo(nullptr);
                av_write_trailer(p.avFormatContext);
            }

            if (p.swsContext)
            {
                sws_freeContext(p.swsContext);
            }
            if (p.avFrame2)
            {
                av_frame_free(&p.avFrame2);
            }
            if (p.avFrame)
            {
                av_frame_free(&p.avFrame);
            }
            if (p.avAudioFrame)
            {
                av_frame_free(&p.avAudioFrame);
            }
            if (p.avPacket)
            {
                av_packet_free(&p.avPacket);
            }
            if (p.avAudioPacket)
            {
                av_packet_free(&p.avAudioPacket);
            }
            if (p.avAudioFifo)
            {
                av_audio_fifo_free( p.avAudioFifo );
                p.avAudioFifo = nullptr;
            }
            if (p.avAudioCodecContext)
            {
                avcodec_free_context(&p.avAudioCodecContext);
            }
            if (p.avCodecContext)
            {
                avcodec_free_context(&p.avCodecContext);
            }
            if (p.avFormatContext && p.avFormatContext->pb)
            {
                avio_closep(&p.avFormatContext->pb);
            }
            if (p.avFormatContext)
            {
                avformat_free_context(p.avFormatContext);
            }
        }

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

        void Write::writeVideo(
            const otime::RationalTime& time,
            const std::shared_ptr<image::Image>& image,
            const io::Options&)
        {
            TLRENDER_P();

            const auto& info = image->getInfo();
            av_image_fill_arrays(
                p.avFrame2->data,
                p.avFrame2->linesize,
                image->getData(),
                p.avPixelFormatIn,
                info.size.w,
                info.size.h,
                info.layout.alignment);

            // Flip the image vertically.
            switch (info.pixelType)
            {
            case image::PixelType::L_U8:
            case image::PixelType::L_U16:
            case image::PixelType::RGB_U8:
            case image::PixelType::RGB_U16:
            case image::PixelType::RGBA_U8:
            case image::PixelType::RGBA_U16:
            {
                const size_t channelCount = image::getChannelCount(info.pixelType);
                for (size_t i = 0; i < channelCount; i++)
                {
                    p.avFrame2->data[i] += p.avFrame2->linesize[i] * (info.size.h - 1);
                    p.avFrame2->linesize[i] = -p.avFrame2->linesize[i];
                }
                break;
            }
            case image::PixelType::YUV_420P_U8:
            case image::PixelType::YUV_422P_U8:
            case image::PixelType::YUV_444P_U8:
            case image::PixelType::YUV_420P_U16:
            case image::PixelType::YUV_422P_U16:
            case image::PixelType::YUV_444P_U16:
                //! \bug How do we flip YUV data?
                throw std::runtime_error(string::Format("{0}: Incompatible pixel type").arg(p.fileName));
                break;
            default: break;
            }

            sws_scale(
                p.swsContext,
                (uint8_t const* const*)p.avFrame2->data,
                p.avFrame2->linesize,
                0,
                p.avVideoStream->codecpar->height,
                p.avFrame->data,
                p.avFrame->linesize);

            const auto timeRational = time::toRational(time.rate());
            p.avFrame->pts = av_rescale_q(
                time.value(),
                { timeRational.second, timeRational.first },
                p.avVideoStream->time_base);
            _encodeVideo(p.avFrame);
        }

        void Write::_encodeVideo(AVFrame* frame)
        {
            TLRENDER_P();

            int r = avcodec_send_frame(p.avCodecContext, frame);
            if (r < 0)
            {
                throw std::runtime_error(
                    string::Format("{0}: Cannot send video frame")
                        .arg(p.fileName));
            }

            while (r >= 0)
            {
                r = avcodec_receive_packet(p.avCodecContext, p.avPacket);
                if (r == AVERROR(EAGAIN) || r == AVERROR_EOF)
                {
                    return;
                }
                else if (r < 0)
                {
                    throw std::runtime_error(
                        string::Format("{0}: Cannot receive video packet")
                            .arg(p.fileName));
                }
                
                // Needed 
                p.avPacket->stream_index = p.avVideoStream->index;
                
                // log_packet(p.avFormatContext, p.avPacket);
                r = av_interleaved_write_frame(p.avFormatContext, p.avPacket);
                if (r < 0)
                {
                    throw std::runtime_error(
                        string::Format("{0}: Cannot write video frame")
                            .arg(p.fileName));
                }
                av_packet_unref(p.avPacket);
            }
        }
        void Write::writeAudio(
            const otime::RationalTime& time,
            const std::shared_ptr<audio::Audio>& audioIn,
            const io::Options&)
        {
            TLRENDER_P();

            int r = 0;
            const auto& info = audioIn->getInfo();
            if (!info.isValid())
            {
                throw std::runtime_error(
                    "Write audio called without a valid audio timeline.");
            }
            if (!p.avAudioFifo)
            {
                throw std::runtime_error(
                    "Audio FIFO buffer was not allocated.");
            }

            if (p.lastTime != time)
            {
                p.lastTime = time;

                long unsigned* t = (long unsigned*) audioIn->getData();
                std::cerr << time << " audioIn=" << (void*)audioIn->getData()
                          << " first chars="
                          << t[0]
                          << std::endl;

                const auto audio = planarDeinterleave(audioIn);
                
                uint8_t* data = audio->getData();
                const size_t sampleCount = audio->getSampleCount();

                uint8_t* flatData[8];
                size_t channels = audio->getChannelCount();
                for (size_t i = 0; i < channels; ++i)
                {
                    flatData[i] =
                        data + i * sampleCount *
                                   audio::getByteCount(audio->getDataType());
                }
                
                r = av_audio_fifo_write(
                    p.avAudioFifo, reinterpret_cast<void**>(flatData),
                    sampleCount);
                if (r < 0)
                {
                    throw std::runtime_error(
                        string::Format(
                            "Could not write to fifo buffer at time {0}.")
                        .arg(time));
                }
                if (r != sampleCount)
                {
                    throw std::runtime_error(
                        string::Format(
                            "Could not write all samples fifo buffer at time {0}.")
                        .arg(time));
                }
            }

            const int frameSize = p.avAudioCodecContext->frame_size;
            const AVRational ratio = { 1, p.avAudioCodecContext->sample_rate };
            while (av_audio_fifo_size(p.avAudioFifo) >= info.sampleRate)
            {
                r = av_frame_make_writable(p.avAudioFrame);
                if (r < 0)
                {
                    throw std::runtime_error(
                        string::Format(
                            "Could not make audio frame writable at time {0}.")
                        .arg(time));
                }

                r = av_audio_fifo_read(
                    p.avAudioFifo,
                    reinterpret_cast<void**>(p.avAudioFrame->extended_data),
                    frameSize);
                if (r < 0)
                {
                    throw std::runtime_error(
                        string::Format("Could not read from audio fifo buffer "
                                       "at time {0}.")
                            .arg(time));
                }
                
                p.avAudioFrame->pts = av_rescale_q(
                    p.totalSamples, ratio, p.avAudioCodecContext->time_base);
                
                _encodeAudio(p.avAudioFrame);
                
                p.totalSamples += frameSize;
            }
        }
        
        void Write::_flushAudio()
        {
            TLRENDER_P();

            int frameSize = p.avAudioCodecContext->frame_size;
            const AVRational ratio = { 1, p.avAudioCodecContext->sample_rate };
            int fifoSize = av_audio_fifo_size(p.avAudioFifo);
            while (fifoSize > 0)
            {
                frameSize = std::min(fifoSize, frameSize);
                
                int r = av_frame_make_writable(p.avAudioFrame);
                if (r < 0)
                    return;

                r = av_audio_fifo_read(
                    p.avAudioFifo,
                    reinterpret_cast<void**>(p.avAudioFrame->extended_data),
                    frameSize);
                if (r < 0)
                    return;
                
                p.avAudioFrame->pts = av_rescale_q(
                    p.totalSamples, ratio, p.avAudioCodecContext->time_base);

                _encodeAudio(p.avAudioFrame);

                fifoSize -= frameSize;
                p.totalSamples += frameSize;
            }
        }
        
        void Write::_encodeAudio(AVFrame* frame)
        {
            TLRENDER_P();
                
            int r = avcodec_send_frame(p.avAudioCodecContext, frame);
            if (r < 0)
            {
                throw std::runtime_error(
                    string::Format("{0}: Cannot send audio frame")
                        .arg(p.fileName));
            }

            while (r >= 0)
            {
                r = avcodec_receive_packet(
                    p.avAudioCodecContext, p.avAudioPacket);
                if (r == AVERROR(EAGAIN) || r == AVERROR_EOF)
                {
                    return;
                }
                else if (r < 0) 
                {
                    throw std::runtime_error(
                        string::Format("{0}: Cannot receive audio packet")
                            .arg(p.fileName));
                }

                // Needed 
                p.avAudioPacket->stream_index = p.avAudioStream->index;

                // log_packet(p.avFormatContext, p.avAudioPacket, true);
                // av_packet_rescale_ts(
                //     p.avAudioPacket, p.avAudioCodecContext->time_base,
                //     p.avAudioStream->time_base);
                r = av_interleaved_write_frame(p.avFormatContext, p.avAudioPacket);
                if (r < 0)
                {
                    throw std::runtime_error(
                        string::Format("{0}: Cannot write audio frame")
                        .arg(p.fileName));
                }
                av_packet_unref(p.avAudioPacket);
            }
        
        }


    }
}
