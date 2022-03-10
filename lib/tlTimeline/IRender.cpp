// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2022 Darby Johnston
// All rights reserved.

#include <tlTimeline/IRender.h>

#include <tlCore/Error.h>
#include <tlCore/String.h>

#include <array>
#include <cmath>
#include <cstdlib>

namespace tl
{
    namespace timeline
    {
        TLRENDER_ENUM_IMPL(
            YUVRange,
            "FromFile",
            "Full",
            "Video");
        TLRENDER_ENUM_SERIALIZE_IMPL(YUVRange);

        TLRENDER_ENUM_IMPL(
            Channels,
            "Color",
            "Red",
            "Green",
            "Blue",
            "Alpha");
        TLRENDER_ENUM_SERIALIZE_IMPL(Channels);

        TLRENDER_ENUM_IMPL(
            AlphaBlend,
            "None",
            "Straight",
            "Premultiplied");
        TLRENDER_ENUM_SERIALIZE_IMPL(AlphaBlend);

        math::Matrix4x4f brightness(const math::Vector3f& value)
        {
            return math::Matrix4x4f(
                value.x, 0.F, 0.F, 0.F,
                0.F, value.y, 0.F, 0.F,
                0.F, 0.F, value.z, 0.F,
                0.F, 0.F, 0.F, 1.F);
        }

        math::Matrix4x4f contrast(const math::Vector3f& value)
        {
            return
                math::Matrix4x4f(
                    1.F, 0.F, 0.F, -.5F,
                    0.F, 1.F, 0.F, -.5F,
                    0.F, 0.F, 1.F, -.5F,
                    0.F, 0.F, 0.F, 1.F) *
                math::Matrix4x4f(
                    value.x, 0.F, 0.F, 0.F,
                    0.F, value.y, 0.F, 0.F,
                    0.F, 0.F, value.z, 0.F,
                    0.F, 0.F, 0.F, 1.F) *
                math::Matrix4x4f(
                    1.F, 0.F, 0.F, .5F,
                    0.F, 1.F, 0.F, .5F,
                    0.F, 0.F, 1.F, .5F,
                    0.F, 0.F, 0.F, 1.F);
        }

        math::Matrix4x4f saturation(const math::Vector3f& value)
        {
            const math::Vector3f s(
                (1.F - value.x) * .3086F,
                (1.F - value.y) * .6094F,
                (1.F - value.z) * .0820F);
            return math::Matrix4x4f(
                s.x + value.x, s.y, s.z, 0.F,
                s.x, s.y + value.y, s.z, 0.F,
                s.x, s.y, s.z + value.z, 0.F,
                0.F, 0.F, 0.F, 1.F);
        }

        math::Matrix4x4f tint(float v)
        {
            const float c = cos(v * M_PI * 2.F);
            const float c2 = 1.F - c;
            const float c3 = 1.F / 3.F * c2;
            const float s = sin(v * M_PI * 2.F);
            const float sq = sqrtf(1.F / 3.F);
            return math::Matrix4x4f(
                c + c2 / 3.F, c3 - sq * s, c3 + sq * s, 0.F,
                c3 + sq * s, c + c3, c3 - sq * s, 0.F,
                c3 - sq * s, c3 + sq * s, c + c3, 0.F,
                0.F, 0.F, 0.F, 1.F);
        }

        math::Matrix4x4f color(const Color& in)
        {
            return
                brightness(in.brightness) *
                contrast(in.contrast) *
                saturation(in.saturation) *
                tint(in.tint);
        }

        TLRENDER_ENUM_IMPL(
            CompareMode,
            "A",
            "B",
            "Wipe",
            "Overlay",
            "Horizontal",
            "Vertical",
            "Tile");
        TLRENDER_ENUM_SERIALIZE_IMPL(CompareMode);

        std::vector<math::BBox2i> tiles(CompareMode mode, const std::vector<imaging::Size>& sizes)
        {
            std::vector<math::BBox2i> out;
            const size_t size = sizes.size();
            switch (mode)
            {

            case CompareMode::A:
            case CompareMode::B:
            case CompareMode::Wipe:
            case CompareMode::Overlay:
                if (size > 0)
                {
                    out.push_back(math::BBox2i(0, 0, sizes[0].w, sizes[0].h));
                }
                break;
            case CompareMode::Horizontal:
                if (size > 0)
                {
                    out.push_back(math::BBox2i(
                        0,
                        0,
                        sizes[0].w / 2,
                        sizes[0].h));
                }
                if (size > 1)
                {
                    out.push_back(math::BBox2i(
                        sizes[0].w / 2,
                        0,
                        sizes[0].w / 2,
                        sizes[0].h));
                }
                break;
            case CompareMode::Vertical:
                if (size >  0)
                {
                    out.push_back(math::BBox2i(
                        0,
                        0,
                        sizes[0].w,
                        sizes[0].h / 2));
                }
                if (size > 1)
                {
                    out.push_back(math::BBox2i(
                        0,
                        sizes[0].h / 2,
                        sizes[0].w,
                        sizes[0].h / 2));
                }
                break;
            case CompareMode::Tile:
                if (size > 0)
                {
                    //! \todo Temporarily revert to previous functionality.
                    /*imaging::Size tileSize;
                    for (const auto& i : sizes)
                    {
                        tileSize.w = std::max(tileSize.w, i.w);
                        tileSize.h = std::max(tileSize.h, i.h);
                    }
                    out.first.w = tileSize.w * columns;
                    out.first.h = tileSize.h * rows;

                    int i = 0;
                    for (int r = 0, y = 0; r < rows; ++r)
                    {
                        for (int c = 0, x = 0; c < columns; ++c, ++i)
                        {
                            if (i < size)
                            {
                                const auto& s = sizes[i];
                                const math::BBox2i bbox(
                                    x + tileSize.w / 2 - s.w / 2,
                                    y + tileSize.h / 2 - s.h / 2,
                                    s.w,
                                    s.h);
                                out.second.push_back(bbox);
                            }
                            x += tileSize.w;
                        }
                        y += tileSize.h;
                    }*/
                    int columns = 0;
                    int rows = 0;
                    switch (size)
                    {
                    case 1: columns = 1; rows = 1; break;
                    case 2: columns = 1; rows = 2; break;
                    default:
                    {
                        const float sqrt = std::sqrt(size);
                        columns = std::ceil(sqrt);
                        const std::div_t d = std::div(size, columns);
                        rows = d.quot + (d.rem > 0 ? 1 : 0);
                        break;
                    }
                    }
                    const int w = sizes[0].w / columns;
                    const int h = sizes[0].h / rows;
                    for (int row = 0, y = 0; row < rows; ++row, y += h)
                    {
                        for (int column = 0, x = 0; column < columns; ++column, x += w)
                        {
                            out.push_back(math::BBox2i(x, y, w, h));
                        }
                    }
                }
                break;
            default: break;
            }
            return out;
        }

        imaging::Size getRenderSize(CompareMode mode, const std::vector<imaging::Size>& sizes)
        {
            math::BBox2i bbox;
            const auto tiles = timeline::tiles(mode, sizes);
            if (!tiles.empty())
            {
                bbox = tiles[0];
            }
            for (size_t i = 1; i < tiles.size(); ++i)
            {
                bbox.expand(tiles[i]);
            }
            return imaging::Size(bbox.w(), bbox.h());
        }

        void IRender::_init(const std::shared_ptr<system::Context>& context)
        {
            _context = context;
        }

        IRender::IRender()
        {}

        IRender::~IRender()
        {}
    }
}
