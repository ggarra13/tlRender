// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#include <tlDevice/NDIUtil.h>

#include <array>

namespace tl
{
    namespace ndi
    {
        NDIlib_FourCC_video_type_e toNDI(device::PixelType value)
        {
            const std::array<
                NDIlib_FourCC_video_type_e,
                static_cast<size_t>(device::PixelType::Count)> data =
            {
                NDIlib_FourCC_video_type_max,
                NDIlib_FourCC_video_type_BGRA,// Planar 8bit, 4:4:4:4 video format.
                NDIlib_FourCC_type_YV12,      // Planar
                NDIlib_FourCC_video_type_max, // bmdFormat10BitRGB,
                NDIlib_FourCC_video_type_max, // bmdFormat10BitRGBX,
                NDIlib_FourCC_video_type_max, // bmdFormat10BitRGBXLE  
                NDIlib_FourCC_video_type_UYVA, // 4:2:2:4 (yuv + alpha in 8 bps)
                NDIlib_FourCC_video_type_P216, // 4:2:2 in 16bpp
                NDIlib_FourCC_video_type_PA16, // 4:2:2:4 (yuv + alpha in 16 bps)
                NDIlib_FourCC_video_type_I420, // same as YV12?
                NDIlib_FourCC_video_type_BGRX, // 8 bit, 4:4:4 (blue, green, red, 255 packed in 32bits
                NDIlib_FourCC_video_type_RGBA, // 4:4:4:4 red, green, blue, alpha
                NDIlib_FourCC_video_type_RGBX, // 4:4:4 red, green, blue, 255
            };
            return data[static_cast<size_t>(value)];
        }

        device::PixelType fromNDI(NDIlib_FourCC_video_type_e value)
        {
            device::PixelType out = device::PixelType::None;
            switch (value)
            {
            case NDIlib_FourCC_video_type_BGRA:
                out = device::PixelType::_8BitBGRA;
                break;
            case NDIlib_FourCC_type_YV12:
                out = device::PixelType::_8BitYUV;
                break;
            case NDIlib_FourCC_video_type_UYVA:
                out = device::PixelType::_8BitUYVA;
                break;
            case NDIlib_FourCC_video_type_P216:
                out = device::PixelType::_16BitP216;
                break;
            case NDIlib_FourCC_video_type_PA16:
                out = device::PixelType::_16BitPA16;
                break;
            case NDIlib_FourCC_video_type_I420:
                out = device::PixelType::_8BitI420;
                break;
            case NDIlib_FourCC_video_type_BGRX:
                out = device::PixelType::_8BitBGRX;
            case NDIlib_FourCC_video_type_RGBA:
                out = device::PixelType::_8BitRGBA;
            case NDIlib_FourCC_video_type_RGBX:
                out = device::PixelType::_8BitRGBX;
                break;
            default:
                break;
            }
            return out;
        }

        // std::string getVideoConnectionLabel(NDIVideoConnection value)
        // {
        //     std::string out;
        //     switch (value)
        //     {
        //     case ndiVideoConnectionUnspecified:     out = "Unspecified";     break;
        //     case ndiVideoConnectionSDI:             out = "SDI";             break;
        //     case ndiVideoConnectionHDMI:            out = "HDMI";            break;
        //     case ndiVideoConnectionOpticalSDI:      out = "OpticalSDI";      break;
        //     case ndiVideoConnectionComponent:       out = "Component";       break;
        //     case ndiVideoConnectionComposite:       out = "Composite";       break;
        //     case ndiVideoConnectionSVideo:          out = "SVideo";          break;
        //     //case ndiVideoConnectionEthernet:        out = "Ethernet";        break;
        //     //case ndiVideoConnectionOpticalEthernet: out = "OpticalEthernet"; break;
        //     default: break;
        //     };
        //     return out;
        // }

        // std::string getAudioConnectionLabel(NDIAudioConnection value)
        // {
        //     std::string out;
        //     switch (value)
        //     {
        //     case ndiAudioConnectionEmbedded:   out = "Embedded";   break;
        //     case ndiAudioConnectionAESEBU:     out = "AESEBU";     break;
        //     case ndiAudioConnectionAnalog:     out = "Analog";     break;
        //     case ndiAudioConnectionAnalogXLR:  out = "AnalogXLR";  break;
        //     case ndiAudioConnectionAnalogRCA:  out = "AnalogRCA";  break;
        //     case ndiAudioConnectionMicrophone: out = "Microphone"; break;
        //     case ndiAudioConnectionHeadphones: out = "Headphones"; break;
        //     default: break;
        //     }
        //     return out;
        // }

        // std::string getDisplayModeLabel(NDIDisplayMode value)
        // {
        //     std::string out;
        //     switch (value)
        //     {
        //     case ndiModeNTSC:     out = "NTSC";     break;
        //     case ndiModeNTSC2398: out = "NTSC2398"; break;
        //     case ndiModePAL:      out = "PAL";      break;
        //     case ndiModeNTSCp:    out = "NTSCp";    break;
        //     case ndiModePALp:     out = "PALp";     break;

        //     case ndiModeHD1080p2398:  out = "HD1080p2398";  break;
        //     case ndiModeHD1080p24:    out = "HD1080p24";    break;
        //     case ndiModeHD1080p25:    out = "HD1080p25";    break;
        //     case ndiModeHD1080p2997:  out = "HD1080p2997";  break;
        //     case ndiModeHD1080p30:    out = "HD1080p30";    break;
        //     case ndiModeHD1080p4795:  out = "HD1080p4795";  break;
        //     case ndiModeHD1080p48:    out = "HD1080p48";    break;
        //     case ndiModeHD1080p50:    out = "HD1080p50";    break;
        //     case ndiModeHD1080p5994:  out = "HD1080p5994";  break;
        //     case ndiModeHD1080p6000:  out = "HD1080p6000";  break;
        //     case ndiModeHD1080p9590:  out = "HD1080p9590";  break;
        //     case ndiModeHD1080p96:    out = "HD1080p96";    break;
        //     case ndiModeHD1080p100:   out = "HD1080p100";   break;
        //     case ndiModeHD1080p11988: out = "HD1080p11988"; break;
        //     case ndiModeHD1080p120:   out = "HD1080p120";   break;
        //     case ndiModeHD1080i50:    out = "HD1080i50";    break;
        //     case ndiModeHD1080i5994:  out = "HD1080i5994";  break;
        //     case ndiModeHD1080i6000:  out = "HD1080i6000";  break;

        //     case ndiModeHD720p50:   out = "HD720p50";   break;
        //     case ndiModeHD720p5994: out = "HD720p5994"; break;
        //     case ndiModeHD720p60:   out = "HD720p60";   break;

        //     case ndiMode2k2398: out = "2k2398"; break;
        //     case ndiMode2k24:   out = "2k24";   break;
        //     case ndiMode2k25:   out = "2k25";   break;

        //     case ndiMode2kDCI2398:  out = "2kDCI2398";  break;
        //     case ndiMode2kDCI24:    out = "2kDCI24";    break;
        //     case ndiMode2kDCI25:    out = "2kDCI25";    break;
        //     case ndiMode2kDCI2997:  out = "2kDCI2997";  break;
        //     case ndiMode2kDCI30:    out = "2kDCI30";    break;
        //     case ndiMode2kDCI4795:  out = "2kDCI4795";  break;
        //     case ndiMode2kDCI48:    out = "2kDCI48";    break;
        //     case ndiMode2kDCI50:    out = "2kDCI50";    break;
        //     case ndiMode2kDCI5994:  out = "2kDCI5994";  break;
        //     case ndiMode2kDCI60:    out = "2kDCI60";    break;
        //     case ndiMode2kDCI9590:  out = "2kDCI9590";  break;
        //     case ndiMode2kDCI96:    out = "2kDCI96";    break;
        //     case ndiMode2kDCI100:   out = "2kDCI100";   break;
        //     case ndiMode2kDCI11988: out = "2kDCI11988"; break;
        //     case ndiMode2kDCI120:   out = "2kDCI120";   break;

        //     case ndiMode4K2160p2398:  out = "4K2160p2398";  break;
        //     case ndiMode4K2160p24:    out = "4K2160p24";    break;
        //     case ndiMode4K2160p25:    out = "4K2160p25";    break;
        //     case ndiMode4K2160p2997:  out = "4K2160p2997";  break;
        //     case ndiMode4K2160p30:    out = "4K2160p30";    break;
        //     case ndiMode4K2160p4795:  out = "4K2160p4795";  break;
        //     case ndiMode4K2160p48:    out = "4K2160p48";    break;
        //     case ndiMode4K2160p50:    out = "4K2160p50";    break;
        //     case ndiMode4K2160p5994:  out = "4K2160p5994";  break;
        //     case ndiMode4K2160p60:    out = "4K2160p60";    break;
        //     case ndiMode4K2160p9590:  out = "4K2160p9590";  break;
        //     case ndiMode4K2160p96:    out = "4K2160p96";    break;
        //     case ndiMode4K2160p100:   out = "4K2160p100";   break;
        //     case ndiMode4K2160p11988: out = "4K2160p11988"; break;
        //     case ndiMode4K2160p120:   out = "4K2160p120";   break;

        //     case ndiMode4kDCI2398:  out = "4kDCI2398";  break;
        //     case ndiMode4kDCI24:    out = "4kDCI24";    break;
        //     case ndiMode4kDCI25:    out = "4kDCI25";    break;
        //     case ndiMode4kDCI2997:  out = "4kDCI2997";  break;
        //     case ndiMode4kDCI30:    out = "4kDCI30";    break;
        //     case ndiMode4kDCI4795:  out = "4kDCI4795";  break;
        //     case ndiMode4kDCI48:    out = "4kDCI48";    break;
        //     case ndiMode4kDCI50:    out = "4kDCI50";    break;
        //     case ndiMode4kDCI5994:  out = "4kDCI5994";  break;
        //     case ndiMode4kDCI60:    out = "4kDCI60";    break;
        //     case ndiMode4kDCI9590:  out = "4kDCI9590";  break;
        //     case ndiMode4kDCI96:    out = "4kDCI96";    break;
        //     case ndiMode4kDCI100:   out = "4kDCI100";   break;
        //     case ndiMode4kDCI11988: out = "4kDCI11988"; break;
        //     case ndiMode4kDCI120:   out = "4kDCI120";   break;

        //     case ndiMode8K4320p2398: out = "8K4320p2398"; break;
        //     case ndiMode8K4320p24:   out = "8K4320p24";   break;
        //     case ndiMode8K4320p25:   out = "8K4320p25";   break;
        //     case ndiMode8K4320p2997: out = "8K4320p2997"; break;
        //     case ndiMode8K4320p30:   out = "8K4320p30";   break;
        //     case ndiMode8K4320p4795: out = "8K4320p4795"; break;
        //     case ndiMode8K4320p48:   out = "8K4320p48";   break;
        //     case ndiMode8K4320p50:   out = "8K4320p50";   break;
        //     case ndiMode8K4320p5994: out = "8K4320p5994"; break;
        //     case ndiMode8K4320p60:   out = "8K4320p60";   break;

        //     case ndiMode8kDCI2398: out = "8kDCI2398"; break;
        //     case ndiMode8kDCI24:   out = "8kDCI24";   break;
        //     case ndiMode8kDCI25:   out = "8kDCI25";   break;
        //     case ndiMode8kDCI2997: out = "8kDCI2997"; break;
        //     case ndiMode8kDCI30:   out = "8kDCI30";   break;
        //     case ndiMode8kDCI4795: out = "8kDCI4795"; break;
        //     case ndiMode8kDCI48:   out = "8kDCI48";   break;
        //     case ndiMode8kDCI50:   out = "8kDCI50";   break;
        //     case ndiMode8kDCI5994: out = "8kDCI5994"; break;
        //     case ndiMode8kDCI60:   out = "8kDCI60";   break;

        //     case ndiMode640x480p60:   out = "640x480p60";   break;
        //     case ndiMode800x600p60:   out = "800x600p60";   break;
        //     case ndiMode1440x900p50:  out = "1440x900p50";  break;
        //     case ndiMode1440x900p60:  out = "1440x900p60";  break;
        //     case ndiMode1440x1080p50: out = "1440x1080p50"; break;
        //     case ndiMode1440x1080p60: out = "1440x1080p60"; break;
        //     case ndiMode1600x1200p50: out = "1600x1200p50"; break;
        //     case ndiMode1600x1200p60: out = "1600x1200p60"; break;
        //     case ndiMode1920x1200p50: out = "1920x1200p50"; break;
        //     case ndiMode1920x1200p60: out = "1920x1200p60"; break;
        //     case ndiMode1920x1440p50: out = "1920x1440p50"; break;
        //     case ndiMode1920x1440p60: out = "1920x1440p60"; break;
        //     case ndiMode2560x1440p50: out = "2560x1440p50"; break;
        //     case ndiMode2560x1440p60: out = "2560x1440p60"; break;
        //     case ndiMode2560x1600p50: out = "2560x1600p50"; break;
        //     case ndiMode2560x1600p60: out = "2560x1600p60"; break;

        //     case ndiModeUnknown: out = "Unknown"; break;
        //     default: break;
        //     }
        //     return out;
        // }

        // std::string getPixelFormatLabel(NDIPixelFormat value)
        // {
        //     std::string out;
        //     switch (value)
        //     {
        //     case ndiFormatUnspecified: out = "ndiFormatUnspecified"; break;
        //     case ndiFormat8BitYUV:     out = "ndiFormat8BitYUV";     break;
        //     case ndiFormat10BitYUV:    out = "ndiFormat10BitYUV";    break;
        //     case ndiFormat8BitARGB:    out = "ndiFormat8BitARGB";    break;
        //     case ndiFormat8BitBGRA:    out = "ndiFormat8BitBGRA";    break;
        //     case ndiFormat10BitRGB:    out = "ndiFormat10BitRGB";    break;
        //     case ndiFormat12BitRGB:    out = "ndiFormat12BitRGB";    break;
        //     case ndiFormat12BitRGBLE:  out = "ndiFormat12BitRGBLE";  break;
        //     case ndiFormat10BitRGBXLE: out = "ndiFormat10BitRGBXLE"; break;
        //     case ndiFormat10BitRGBX:   out = "ndiFormat10BitRGBX";   break;
        //     case ndiFormatH265:        out = "ndiFormatH265";        break;
        //     case ndiFormatDNxHR:       out = "ndiFormatDNxHR";       break;
        //     default: break;
        //     };
        //     return out;
        // }

        // std::string getOutputFrameCompletionResultLabel(NDIOutputFrameCompletionResult value)
        // {
        //     std::string out;
        //     switch (value)
        //     {
        //     case ndiOutputFrameCompleted:     out = "Completed";      break;
        //     case ndiOutputFrameDisplayedLate: out = "Displayed Late"; break;
        //     case ndiOutputFrameDropped:       out = "Dropped";        break;
        //     case ndiOutputFrameFlushed:       out = "Flushed";        break;
        //     };
        //     return out;
        // }

        device::PixelType getOutputType(device::PixelType value)
        {
            device::PixelType out = device::PixelType::None;
            switch (value)
            {
            case device::PixelType::_8BitBGRA:
            case device::PixelType::_10BitRGB:
            case device::PixelType::_10BitRGBX:
            case device::PixelType::_10BitRGBXLE:
            case device::PixelType::_12BitRGB:
            case device::PixelType::_12BitRGBLE:
            case device::PixelType::_8BitUYVA:
            case device::PixelType::_16BitP216:
            case device::PixelType::_16BitPA16:
            case device::PixelType::_8BitI420:
            case device::PixelType::_8BitBGRX:
            case device::PixelType::_8BitRGBA:
            case device::PixelType::_8BitRGBX:
                out = value;
                break;
            case device::PixelType::_8BitYUV:
                out = device::PixelType::_8BitBGRA;
                break;
            //case device::PixelType::_10BitYUV:
            //    out = device::PixelType::_10BitRGBXLE;
            //    break;
            default: break;
            }
            return out;
        }

        image::PixelType getColorBuffer(device::PixelType value)
        {
            const std::array<image::PixelType, static_cast<size_t>(device::PixelType::Count)> data =
            {
                image::PixelType::None,
                image::PixelType::RGBA_U8,
                image::PixelType::RGBA_U8,
                image::PixelType::RGB_U16,
                image::PixelType::RGB_U16,
                image::PixelType::RGB_U16,
                //image::PixelType::RGB_U10,
                image::PixelType::RGB_U16,
                image::PixelType::RGB_U16,
                image::PixelType::RGBA_U8,
                image::PixelType::RGB_U16,
                image::PixelType::RGBA_U16,
                image::PixelType::RGB_U16,
                image::PixelType::RGB_U8,
                image::PixelType::RGBA_U8,
                image::PixelType::RGB_U8,
            };
            return data[static_cast<size_t>(value)];
        }

        size_t getPackPixelsSize(const math::Size2i& size,
                                 device::PixelType pixelType)
        {
            size_t out = 0;
            switch (pixelType)
            {
            case device::PixelType::_8BitBGRA:
            case device::PixelType::_8BitYUV:
            //case PixelType::_10BitYUV:
                out = getDataByteCount(size, pixelType);
                break;
            case device::PixelType::_10BitRGB:
            case device::PixelType::_10BitRGBX:
            case device::PixelType::_10BitRGBXLE:
            case device::PixelType::_12BitRGB:
            case device::PixelType::_12BitRGBLE:
                out = size.w * size.h * 3 * sizeof(uint16_t);
                break;
            default: break;
            }
            return out;
        }

        GLenum getPackPixelsFormat(device::PixelType value)
        {
            const std::array<GLenum, static_cast<size_t>(device::PixelType::Count)> data =
            {
                GL_NONE,
                GL_BGRA,
                GL_BGRA,
                GL_RGB,
                GL_RGB,
                GL_RGB,
                //GL_RGBA,
                GL_RGB,
                GL_RGB
            };
            return data[static_cast<size_t>(value)];
        }

        GLenum getPackPixelsType(device::PixelType value)
        {
            const std::array<GLenum, static_cast<size_t>(device::PixelType::Count)> data =
            {
                GL_NONE,
                GL_UNSIGNED_BYTE,
                GL_UNSIGNED_BYTE,
                GL_UNSIGNED_SHORT,
                GL_UNSIGNED_SHORT,
                GL_UNSIGNED_SHORT,
                //GL_UNSIGNED_INT_10_10_10_2,
                GL_UNSIGNED_SHORT,
                GL_UNSIGNED_SHORT
            };
            return data[static_cast<size_t>(value)];
        }

        GLint getPackPixelsAlign(device::PixelType value)
        {
            const std::array<GLint, static_cast<size_t>(device::PixelType::Count)> data =
            {
                0,
                4,
                4,
                1,
                1,
                1,
                //! \bug OpenGL only allows alignment values of 1, 2, 4, and 8.
                //8, // 128,
                1,
                1
            };
            return data[static_cast<size_t>(value)];
        }

        GLint getPackPixelsSwap(device::PixelType value)
        {
            const std::array<GLint, static_cast<size_t>(device::PixelType::Count)> data =
            {
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                GL_FALSE,
                //GL_FALSE,
                GL_FALSE,
                GL_FALSE
            };
            return data[static_cast<size_t>(value)];
        }

        void copyPackPixels(
            const void* inP,
            void* outP,
            const math::Size2i& size,
            device::PixelType pixelType)
        {
            const size_t rowByteCount = getRowByteCount(size.w, pixelType);
            switch (pixelType)
            {
            case device::PixelType::_10BitRGB:
                for (int y = 0; y < size.h; ++y)
                {
                    const uint16_t* in16 = (const uint16_t*)inP + y * size.w * 3;
                    uint8_t* out8 = (uint8_t*)outP + y * rowByteCount;
                    for (int x = 0; x < size.w; ++x)
                    {
                        const uint16_t r10 = in16[0] >> 6;
                        const uint16_t g10 = in16[1] >> 6;
                        const uint16_t b10 = in16[2] >> 6;
                        out8[3] = b10;
                        out8[2] = (b10 >> 8) | (g10 << 2);
                        out8[1] = (g10 >> 6) | (r10 << 4);
                        out8[0] = r10 >> 4;

                        in16 += 3;
                        out8 += 4;
                    }
                }
                break;
            case device::PixelType::_10BitRGBX:
                for (int y = 0; y < size.h; ++y)
                {
                    const uint16_t* in16 = (const uint16_t*)inP + y * size.w * 3;
                    uint8_t* out8 = (uint8_t*)outP + y * rowByteCount;
                    for (int x = 0; x < size.w; ++x)
                    {
                        const uint16_t r10 = in16[0] >> 6;
                        const uint16_t g10 = in16[1] >> 6;
                        const uint16_t b10 = in16[2] >> 6;
                        out8[3] = b10 << 2;
                        out8[2] = (b10 >> 6) | (g10 << 4);
                        out8[1] = (g10 >> 4) | (r10 << 6);
                        out8[0] = r10 >> 2;

                        in16 += 3;
                        out8 += 4;
                    }
                }
                break;
            case device::PixelType::_10BitRGBXLE:
                for (int y = 0; y < size.h; ++y)
                {
                    const uint16_t* in16 = (const uint16_t*)inP + y * size.w * 3;
                    uint8_t* out8 = (uint8_t*)outP + y * rowByteCount;
                    for (int x = 0; x < size.w; ++x)
                    {
                        const uint16_t r10 = in16[0] >> 6;
                        const uint16_t g10 = in16[1] >> 6;
                        const uint16_t b10 = in16[2] >> 6;
                        out8[0] = b10 << 2;
                        out8[1] = (b10 >> 6) | (g10 << 4);
                        out8[2] = (g10 >> 4) | (r10 << 6);
                        out8[3] = r10 >> 2;

                        in16 += 3;
                        out8 += 4;
                    }
                }
                break;
            case device::PixelType::_12BitRGB:
                for (int y = 0; y < size.h; ++y)
                {
                    const uint16_t* in16 = (const uint16_t*)inP + y * size.w * 3;
                    uint8_t* out8 = (uint8_t*)outP + y * rowByteCount;
                    for (int x = 0; x < size.w; x += 8)
                    {
                        uint16_t r12 = in16[0] >> 4;
                        uint16_t g12 = in16[1] >> 4;
                        uint16_t b12 = in16[2] >> 4;
                        out8[0 + (3 - 0)] = r12;
                        out8[0 + (3 - 1)] = r12 >> 8;
                        out8[0 + (3 - 1)] |= g12 << 4;
                        out8[0 + (3 - 2)] = g12 >> 4;
                        out8[0 + (3 - 3)] = b12;
                        out8[4 + (3 - 0)] = b12 >> 8;

                        r12 = in16[3] >> 4;
                        g12 = in16[4] >> 4;
                        b12 = in16[5] >> 4;
                        out8[4 + (3 - 0)] |= r12 << 4;
                        out8[4 + (3 - 1)] = r12 >> 4;
                        out8[4 + (3 - 2)] = g12;
                        out8[4 + (3 - 3)] = g12 >> 8;
                        out8[4 + (3 - 3)] |= b12 << 4;
                        out8[8 + (3 - 0)] = b12 >> 4;

                        r12 = in16[6] >> 4;
                        g12 = in16[7] >> 4;
                        b12 = in16[8] >> 4;
                        out8[8 + (3 - 1)] = r12;
                        out8[8 + (3 - 2)] = r12 >> 8;
                        out8[8 + (3 - 2)] |= g12 << 4;
                        out8[8 + (3 - 3)] = g12 >> 4;
                        out8[12 + (3 - 0)] = b12;
                        out8[12 + (3 - 1)] = b12 >> 8;

                        r12 = in16[9] >> 4;
                        g12 = in16[10] >> 4;
                        b12 = in16[11] >> 4;
                        out8[12 + (3 - 1)] |= r12 << 4;
                        out8[12 + (3 - 2)] = r12 >> 4;
                        out8[12 + (3 - 3)] = g12;
                        out8[16 + (3 - 0)] = g12 >> 8;
                        out8[16 + (3 - 0)] |= b12 << 4;
                        out8[16 + (3 - 1)] = b12 >> 4;

                        r12 = in16[12] >> 4;
                        g12 = in16[13] >> 4;
                        b12 = in16[14] >> 4;
                        out8[16 + (3 - 2)] = r12;
                        out8[16 + (3 - 3)] = r12 >> 8;
                        out8[16 + (3 - 3)] |= g12 << 4;
                        out8[20 + (3 - 0)] = g12 >> 4;
                        out8[20 + (3 - 1)] = b12;
                        out8[20 + (3 - 2)] = b12 >> 8;

                        r12 = in16[15] >> 4;
                        g12 = in16[16] >> 4;
                        b12 = in16[17] >> 4;
                        out8[20 + (3 - 2)] |= r12 << 4;
                        out8[20 + (3 - 3)] = r12 >> 4;
                        out8[24 + (3 - 0)] = g12;
                        out8[24 + (3 - 1)] = g12 >> 8;
                        out8[24 + (3 - 1)] |= b12 << 4;
                        out8[24 + (3 - 2)] = b12 >> 4;

                        r12 = in16[18] >> 4;
                        g12 = in16[19] >> 4;
                        b12 = in16[20] >> 4;
                        out8[24 + (3 - 3)] = r12;
                        out8[28 + (3 - 0)] = r12 >> 8;
                        out8[28 + (3 - 0)] |= g12 << 4;
                        out8[28 + (3 - 1)] = g12 >> 4;
                        out8[28 + (3 - 2)] = b12;
                        out8[28 + (3 - 3)] = b12 >> 8;

                        r12 = in16[21] >> 4;
                        g12 = in16[22] >> 4;
                        b12 = in16[23] >> 4;
                        out8[28 + (3 - 3)] |= r12 << 4;
                        out8[32 + (3 - 0)] = r12 >> 4;
                        out8[32 + (3 - 1)] = g12;
                        out8[32 + (3 - 2)] = g12 >> 8;
                        out8[32 + (3 - 2)] |= b12 << 4;
                        out8[32 + (3 - 3)] = b12 >> 4;

                        in16 += 8 * 3;
                        out8 += 36;
                    }
                }
                break;
            case device::PixelType::_12BitRGBLE:
                for (int y = 0; y < size.h; ++y)
                {
                    const uint16_t* in16 = (const uint16_t*)inP + y * size.w * 3;
                    uint8_t* out8 = (uint8_t*)outP + y * rowByteCount;
                    for (int x = 0; x < size.w; x += 8)
                    {
                        uint16_t r12 = in16[0] >> 4;
                        uint16_t g12 = in16[1] >> 4;
                        uint16_t b12 = in16[2] >> 4;
                        out8[0 + 0] = r12;
                        out8[0 + 1] = r12 >> 8;
                        out8[0 + 1] |= g12 << 4;
                        out8[0 + 2] = g12 >> 4;
                        out8[0 + 3] = b12;
                        out8[4 + 0] = b12 >> 8;

                        r12 = in16[3] >> 4;
                        g12 = in16[4] >> 4;
                        b12 = in16[5] >> 4;
                        out8[4 + 0] |= r12 << 4;
                        out8[4 + 1] = r12 >> 4;
                        out8[4 + 2] = g12;
                        out8[4 + 3] = g12 >> 8;
                        out8[4 + 3] |= b12 << 4;
                        out8[8 + 0] = b12 >> 4;

                        r12 = in16[6] >> 4;
                        g12 = in16[7] >> 4;
                        b12 = in16[8] >> 4;
                        out8[8 + 1] = r12;
                        out8[8 + 2] = r12 >> 8;
                        out8[8 + 2] |= g12 << 4;
                        out8[8 + 3] = g12 >> 4;
                        out8[12 + 0] = b12;
                        out8[12 + 1] = b12 >> 8;

                        r12 = in16[9] >> 4;
                        g12 = in16[10] >> 4;
                        b12 = in16[11] >> 4;
                        out8[12 + 1] |= r12 << 4;
                        out8[12 + 2] = r12 >> 4;
                        out8[12 + 3] = g12;
                        out8[16 + 0] = g12 >> 8;
                        out8[16 + 0] |= b12 << 4;
                        out8[16 + 1] = b12 >> 4;

                        r12 = in16[12] >> 4;
                        g12 = in16[13] >> 4;
                        b12 = in16[14] >> 4;
                        out8[16 + 2] = r12;
                        out8[16 + 3] = r12 >> 8;
                        out8[16 + 3] |= g12 << 4;
                        out8[20 + 0] = g12 >> 4;
                        out8[20 + 1] = b12;
                        out8[20 + 2] = b12 >> 8;

                        r12 = in16[15] >> 4;
                        g12 = in16[16] >> 4;
                        b12 = in16[17] >> 4;
                        out8[20 + 2] |= r12 << 4;
                        out8[20 + 3] = r12 >> 4;
                        out8[24 + 0] = g12;
                        out8[24 + 1] = g12 >> 8;
                        out8[24 + 1] |= b12 << 4;
                        out8[24 + 2] = b12 >> 4;

                        r12 = in16[18] >> 4;
                        g12 = in16[19] >> 4;
                        b12 = in16[20] >> 4;
                        out8[24 + 3] = r12;
                        out8[28 + 0] = r12 >> 8;
                        out8[28 + 0] |= g12 << 4;
                        out8[28 + 1] = g12 >> 4;
                        out8[28 + 2] = b12;
                        out8[28 + 3] = b12 >> 8;

                        r12 = in16[21] >> 4;
                        g12 = in16[22] >> 4;
                        b12 = in16[23] >> 4;
                        out8[28 + 3] |= r12 << 4;
                        out8[32 + 0] = r12 >> 4;
                        out8[32 + 1] = g12;
                        out8[32 + 2] = g12 >> 8;
                        out8[32 + 2] |= b12 << 4;
                        out8[32 + 3] = b12 >> 4;

                        in16 += 8 * 3;
                        out8 += 36;
                    }
                }
                break;
            default:
                memcpy(outP, inP, getDataByteCount(size, pixelType));
                break;
            }
        }
    }
}
