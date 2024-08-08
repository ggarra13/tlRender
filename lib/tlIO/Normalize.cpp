// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#include <tlIO/OpenEXRPrivate.h>

#include <tlCore/FileIO.h>
#include <tlCore/Locale.h>
#include <tlCore/LogSystem.h>
#include <tlCore/StringFormat.h>

#include <ImfInputPart.h>
#include <ImfChannelList.h>
#include <ImfRgbaFile.h>

#include <array>
#include <cstring>
#include <sstream>

namespace tl
{
    namespace io
    {

        void normalizeImage(std::shared_ptr<image::Image> inout,
                            const image::Info& info, const int minX, const int maxX, const int minY,
                            const int maxY)
        {
            const size_t channels = image::getChannelCount(info.pixelType);
            const size_t channelByteCount = image::getBitDepth(info.pixelType) / 8;
            const size_t cb = channels * channelByteCount;

            uint8_t* p = inout->getData();
                        
            float minValue = std::numeric_limits<float>::max();
            float maxValue = std::numeric_limits<float>::min();
            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    switch(info.pixelType)
                    {
                    case image::PixelType::L_F16:
                    {
                        float value = static_cast<float>(static_cast<half>(*p));
                        if (value < minValue)
                            minValue = value;
                        if (value > maxValue)
                            maxValue = value;
                        break;
                    }
                    case image::PixelType::L_F32:
                    {
                        float value = static_cast<float>(*p);
                        if (value < minValue)
                            minValue = value;
                        if (value > maxValue)
                            maxValue = value;
                        break;
                    }
                    case image::PixelType::RGB_F16:
                    {
                        half* values = reinterpret_cast<half*>(p);
                        for (int i = 0; i < 3; ++i)
                        {
                            if (values[i] < minValue)
                                minValue = values[i];
                            if (values[i] > maxValue)
                                maxValue = values[i];
                        }
                        break;
                    }
                    case image::PixelType::RGB_F32:
                    {
                        float* values = reinterpret_cast<float*>(p);
                        for (int i = 0; i < 3; ++i)
                        {
                            if (values[i] < minValue)
                                minValue = values[i];
                            if (values[i] > maxValue)
                                maxValue = values[i];
                        }
                        break;
                    }
                    case image::PixelType::RGBA_F16:
                    {
                        half* values = reinterpret_cast<half*>(p);
                        for (int i = 0; i < 4; ++i)
                        {
                            if (values[i] < minValue)
                                minValue = values[i];
                            if (values[i] > maxValue)
                                maxValue = values[i];
                        }
                        break;
                    }
                    case image::PixelType::RGBA_F32:
                    {
                        float* values = reinterpret_cast<float*>(p);
                        for (int i = 0; i < 4; ++i)
                        {
                            if (values[i] < minValue)
                                minValue = values[i];
                            if (values[i] > maxValue)
                                maxValue = values[i];
                        }
                        break;
                    }
                    default:
                        break;
                    }
                    p += cb;
                }
            }

            p = inout->getData();
            const float range = maxValue - minValue;
            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    switch(info.pixelType)
                    {
                    case image::PixelType::L_F16:
                    {
                        half* n = reinterpret_cast<half*>(p);
                        *n = (*n - minValue) / range;
                        break;
                    }
                    case image::PixelType::L_F32:
                    {
                        float* n = reinterpret_cast<float*>(p);
                        *n = (*n - minValue) / range;
                        break;
                    }
                    case image::PixelType::RGB_F16:
                    {
                        half* n = reinterpret_cast<half*>(p);
                        for (short i = 0; i < 3; ++i)
                        {
                            n[i] = (n[i] - minValue) / range;
                        }
                        break;
                    }
                    case image::PixelType::RGB_F32:
                    {
                        float* n = reinterpret_cast<float*>(p);
                        for (short i = 0; i < 3; ++i)
                        {
                            n[i] = (n[i] - minValue) / range;
                        }
                        break;
                    }
                    case image::PixelType::RGBA_F16:
                    {
                        half* n = reinterpret_cast<half*>(p);
                        for (short i = 0; i < 3; ++i)
                        {
                            n[i] = (n[i] - minValue) / range;
                        }
                        break;
                    }
                    case image::PixelType::RGBA_F32:
                    {
                        float* n = reinterpret_cast<float*>(p);
                        for (short i = 0; i < 3; ++i)
                        {
                            n[i] = (n[i] - minValue) / range;
                        }
                        break;
                    }
                    default:
                        break;
                    }
                    p += cb;
                }
            }
        }
    }
}
