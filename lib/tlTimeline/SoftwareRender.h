// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2022 Darby Johnston
// All rights reserved.

#pragma once

#include <tlTimeline/IRender.h>

namespace tl
{
    namespace timeline
    {
        //! Software renderer.
        class SoftwareRender : public IRender
        {
            TLRENDER_NON_COPYABLE(SoftwareRender);

        protected:
            void _init(const std::shared_ptr<system::Context>&);
            SoftwareRender();

        public:
            ~SoftwareRender() override;

            //! Create a new renderer.
            static std::shared_ptr<SoftwareRender> create(const std::shared_ptr<system::Context>&);

            //! Get the frame buffer.
            const std::shared_ptr<imaging::Image> getFrameBuffer() const;

            //! Copy the frame buffer.
            std::shared_ptr<imaging::Image> copyFrameBuffer(imaging::PixelType) const;

            void setTextureCacheSize(size_t) override;
            void setColorConfig(const imaging::ColorConfig&) override;
            void begin(const imaging::Size&) override;
            void end() override;
            void drawRect(
                const math::BBox2i&,
                const imaging::Color4f&) override;
            void drawImage(
                const std::shared_ptr<imaging::Image>&,
                const math::BBox2i&,
                const imaging::Color4f & = imaging::Color4f(1.F, 1.F, 1.F),
                const ImageOptions & = ImageOptions()) override;
            void drawVideo(
                const std::vector<timeline::VideoData>&,
                const std::vector<ImageOptions> & = {},
                const CompareOptions & = CompareOptions()) override;
            void drawText(
                const std::vector<std::shared_ptr<imaging::Glyph> >& glyphs,
                const math::Vector2i& position,
                const imaging::Color4f&) override;

        private:
            TLRENDER_PRIVATE();
        };
    }
}
