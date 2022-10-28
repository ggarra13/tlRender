// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2022 Darby Johnston
// All rights reserved.

#include <tlCoreTest/FontSystemTest.h>

#include <tlCore/Assert.h>
#include <tlCore/FontSystem.h>

#include <sstream>

using namespace tl::imaging;

namespace tl
{
    namespace core_tests
    {
        FontSystemTest::FontSystemTest(const std::shared_ptr<system::Context>& context) :
            ITest("core_tests::FontSystemTest", context)
        {}

        std::shared_ptr<FontSystemTest> FontSystemTest::create(const std::shared_ptr<system::Context>& context)
        {
            return std::shared_ptr<FontSystemTest>(new FontSystemTest(context));
        }

        void FontSystemTest::run()
        {
            {
                const FontInfo fi;
                TLRENDER_ASSERT("NotoSans-Regular" == fi.family);
                TLRENDER_ASSERT(12 == fi.size);
            }
            {
                const FontInfo fi("NotoMono-Regular", 14);
                TLRENDER_ASSERT("NotoMono-Regular" == fi.family);
                TLRENDER_ASSERT(14 == fi.size);
            }
            {
                FontInfo a;
                FontInfo b;
                TLRENDER_ASSERT(a == b);
            }
            {
                FontInfo a("NotoMono-Regular", 14);
                FontInfo b;
                TLRENDER_ASSERT(a < b);
            }
            {
                const GlyphInfo gi;
                TLRENDER_ASSERT(0 == gi.code);
                TLRENDER_ASSERT(FontInfo() == gi.fontInfo);
            }
            {
                const FontInfo fi("NotoMono-Regular", 14);
                const GlyphInfo gi(1, fi);
                TLRENDER_ASSERT(1 == gi.code);
                TLRENDER_ASSERT(fi == gi.fontInfo);
            }
            {
                GlyphInfo a;
                GlyphInfo b;
                TLRENDER_ASSERT(a == b);
            }
            {
                GlyphInfo a;
                GlyphInfo b(1, FontInfo("NotoMono-Regular", 14));
                TLRENDER_ASSERT(a < b);
            }
#if defined(TLRENDER_FREETYPE)
            {
                auto fontSystem = FontSystem::create(_context);
                FontInfo fi("NotoMono-Regular", 14);
                auto fm = fontSystem->getMetrics(fi);
                std::vector<std::string> text =
                {
                    "Hello world!",
                    "Hello\nworld!",
                    "Hello world!"
                };
                std::vector<uint16_t> maxLineWidth =
                {
                    0,
                    0,
                    1
                };
                for (size_t i = 0; i < text.size(); ++i)
                {
                    {
                        std::stringstream ss;
                        ss << "Text: " << text[i];
                        _print(ss.str());
                    }
                    const auto size = fontSystem->measure(text[i], fi, maxLineWidth[i]);
                    {
                        std::stringstream ss;
                        ss << "Size: " << size;
                        _print(ss.str());
                    }
                    const auto sizes = fontSystem->measureGlyphs(text[i], fi, maxLineWidth[i]);
                    TLRENDER_ASSERT(text[i].size() == sizes.size());
                    for (size_t j = 0; j < text[i].size(); ++j)
                    {
                        std::stringstream ss;
                        ss << "BBox '" << text[i][j] << "': " << sizes[j];
                        _print(ss.str());
                    }
                    const auto glyphs = fontSystem->getGlyphs(text[i], fi);
                    TLRENDER_ASSERT(text[i].size() == glyphs.size());
                    for (size_t j = 0; j < text[i].size(); ++j)
                    {
                        uint16_t width = 0;
                        uint16_t height = 0;
                        if (glyphs[j])
                        {
                            width = glyphs[j]->width;
                            height = glyphs[j]->height;
                        }
                        std::stringstream ss;
                        ss << "Glyph '" << text[i][j] << "' size: " << width << "," << height;
                        _print(ss.str());
                    }
                    {
                        std::stringstream ss;
                        ss << "Glyph cache size: " << fontSystem->getGlyphCacheSize();
                        _print(ss.str());
                    }
                    {
                        std::stringstream ss;
                        ss << "Glyph cache percentage: " << fontSystem->getGlyphCachePercentage();
                        _print(ss.str());
                    }
                }
            }
#endif // TLRENDER_FREETYPE
        }
    }
}
