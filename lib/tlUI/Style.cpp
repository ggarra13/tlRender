// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlUI/Style.h>

#include <tlCore/Error.h>
#include <tlCore/String.h>

#include <algorithm>
#include <sstream>

namespace tl
{
    namespace ui
    {
        TLRENDER_ENUM_IMPL(
            SizeRole,
            "None",
            "Margin",
            "MarginSmall",
            "MarginLarge",
            "MarginInside",
            "MarginDialog",
            "Spacing",
            "SpacingSmall",
            "SpacingLarge",
            "SpacingTool",
            "Border",
            "ScrollArea",
            "Slider",
            "Handle",
            "Shadow",
            "DragLength");
        TLRENDER_ENUM_SERIALIZE_IMPL(SizeRole);

        std::map<SizeRole, int> defaultSizeRoles()
        {
            std::map<SizeRole, int> out;
            out[SizeRole::Margin] = 10;
            out[SizeRole::MarginSmall] = 5;
            out[SizeRole::MarginLarge] = 20;
            out[SizeRole::MarginInside] = 2;
            out[SizeRole::MarginDialog] = 40;
            out[SizeRole::Spacing] = 10;
            out[SizeRole::SpacingSmall] = 5;
            out[SizeRole::SpacingLarge] = 20;
            out[SizeRole::SpacingTool] = 2;
            out[SizeRole::Border] = 1;
            out[SizeRole::ScrollArea] = 200;
            out[SizeRole::Slider] = 100;
            out[SizeRole::Handle] = 8;
            out[SizeRole::HandleSmall] = 6;
            out[SizeRole::Shadow] = 15;
            out[SizeRole::DragLength] = 10;
            return out;
        }

        TLRENDER_ENUM_IMPL(
            ColorRole,
            "None",

            "Window",
            "Base",
            "Button",
            "Text",
            "TextDisabled",
            "Border",
            "Hover",
            "Pressed",
            "Checked",
            "KeyFocus",
            "Overlay",
            "ToolTipWindow",
            "ToolTipText",

            "InOut",
            "VideoCache",
            "AudioCache",
            "VideoClip",
            "VideoGap",
            "AudioClip",
            "AudioGap",
            "Transition",

            "Red",
            "Green",
            "Blue",
            "Cyan",
            "Magenta",
            "Yellow");
        TLRENDER_ENUM_SERIALIZE_IMPL(ColorRole);

        std::map<ColorRole, imaging::Color4f> defaultColorRoles()
        {
            std::map<ColorRole, imaging::Color4f> out;
            out[ColorRole::None] = imaging::Color4f();

            out[ColorRole::Window] = imaging::Color4f(.2F, .2F, .2F);
            out[ColorRole::Base] = imaging::Color4f(.17F, .17F, .17F);
            out[ColorRole::Button] = imaging::Color4f(.3F, .3F, .3F);
            out[ColorRole::Text] = imaging::Color4f(1.F, 1.F, 1.F);
            out[ColorRole::TextDisabled] = imaging::Color4f(.5F, .5F, .5F);
            out[ColorRole::Border] = imaging::Color4f(.13F, .13F, .13F);
            out[ColorRole::Hover] = imaging::Color4f(1.F, 1.F, 1.F, .1F);
            out[ColorRole::Pressed] = imaging::Color4f(1.F, 1.F, 1.F, .2F);
            out[ColorRole::Checked] = imaging::Color4f(.6F, .4F, .2F);
            out[ColorRole::KeyFocus] = imaging::Color4f(.6F, .6F, .4F);
            out[ColorRole::Overlay] = imaging::Color4f(0.F, 0.F, 0.F, .5F);
            out[ColorRole::ToolTipWindow] = imaging::Color4f(1.F, .95F, .7F);
            out[ColorRole::ToolTipText] = imaging::Color4f(0.F, 0.F, 0.F);

            out[ColorRole::InOut] = imaging::Color4f(1.F, .7F, .2F, .1F);
            out[ColorRole::VideoCache] = imaging::Color4f(.2F, .4F, .4F);
            out[ColorRole::AudioCache] = imaging::Color4f(.3F, .25F, .4F);
            out[ColorRole::VideoClip] = imaging::Color4f(.2F, .4F, .4F);
            out[ColorRole::VideoGap] = imaging::Color4f(.25F, .31F, .31F);
            out[ColorRole::AudioClip] = imaging::Color4f(.3F, .25F, .4F);
            out[ColorRole::AudioGap] = imaging::Color4f(.25F, .24F, .3F);
            out[ColorRole::Transition] = imaging::Color4f(.4F, .3F, .3F);

            out[ColorRole::Red] = imaging::Color4f(.6F, .3F, .3F);
            out[ColorRole::Green] = imaging::Color4f(.3F, .6F, .3F);
            out[ColorRole::Blue] = imaging::Color4f(.3F, .3F, .6F);
            out[ColorRole::Cyan] = imaging::Color4f(.3F, .6F, .6F);
            out[ColorRole::Magenta] = imaging::Color4f(.6F, .3F, .6F);
            out[ColorRole::Yellow] = imaging::Color4f(.6F, .6F, .3F);
            return out;
        }

        TLRENDER_ENUM_IMPL(
            FontRole,
            "None",
            "Label",
            "Mono",
            "Title");
        TLRENDER_ENUM_SERIALIZE_IMPL(FontRole);

        std::map<FontRole, imaging::FontInfo> defaultFontRoles()
        {
            std::map<FontRole, imaging::FontInfo> out;
            out[FontRole::Label] = imaging::FontInfo("NotoSans-Regular", 12 * 1);
            out[FontRole::Mono] = imaging::FontInfo("NotoMono-Regular", 12 * 1);
            out[FontRole::Title] = imaging::FontInfo("NotoSans-Bold", 16 * 1);
            return out;
        }

        struct Style::Private
        {
            std::weak_ptr<system::Context> context;
            std::shared_ptr<observer::Value<bool> > changed;
        };

        void Style::_init(
            const std::shared_ptr<system::Context>& context)
        {
            TLRENDER_P();
            p.context = context;
            _sizeRoles = defaultSizeRoles();
            _colorRoles = defaultColorRoles();
            _fontRoles = defaultFontRoles();
            p.changed = observer::Value<bool>::create();
        }

        Style::Style() :
            _p(new Private)
        {}

        Style::~Style()
        {}

        std::shared_ptr<Style> Style::create(
            const std::shared_ptr<system::Context>& context)
        {
            auto out = std::shared_ptr<Style>(new Style);
            out->_init(context);
            return out;
        }

        void Style::setSizeRole(SizeRole role, int value)
        {
            TLRENDER_P();
            if (_sizeRoles[role] == value)
                return;
            _sizeRoles[role] = value;
            p.changed->setAlways(true);
        }

        void Style::setSizeRoles(const std::map<SizeRole, int>& value)
        {
            TLRENDER_P();
            if (_sizeRoles == value)
                return;
            _sizeRoles = value;
            p.changed->setAlways(true);
        }

        void Style::setColorRole(ColorRole role, const imaging::Color4f& value)
        {
            TLRENDER_P();
            if (_colorRoles[role] == value)
                return;
            _colorRoles[role] = value;
            p.changed->setAlways(true);
        }

        void Style::setColorRoles(const std::map<ColorRole, imaging::Color4f>& value)
        {
            TLRENDER_P();
            if (_colorRoles == value)
                return;
            _colorRoles = value;
            p.changed->setAlways(true);
        }

        void Style::setFontRole(FontRole role, const imaging::FontInfo& value)
        {
            TLRENDER_P();
            if (_fontRoles[role] == value)
                return;
            _fontRoles[role] = value;
            p.changed->setAlways(true);
        }

        void Style::setFontRoles(const std::map<FontRole, imaging::FontInfo>& value)
        {
            TLRENDER_P();
            if (_fontRoles == value)
                return;
            _fontRoles = value;
            p.changed->setAlways(true);
        }

        std::shared_ptr<observer::IValue<bool> > Style::observeChanged() const
        {
            return _p->changed;
        }

        void to_json(nlohmann::json& json, const std::map<ColorRole, imaging::Color4f>& in)
        {
            std::map<std::string, std::string> s;
            for (const auto& i : in)
            {
                std::stringstream ss;
                ss << i.first;
                std::stringstream ss2;
                ss2 << i.second;
                s[ss.str()] = ss2.str();
            }
            json = s;
        }

        void from_json(const nlohmann::json& json, std::map<ColorRole, imaging::Color4f>& out)
        {
            for (auto i = json.begin(); i != json.end(); ++i)
            {
                std::stringstream ss(i.key());
                ColorRole colorRole = ColorRole::None;
                ss >> colorRole;
                std::stringstream ss2(std::string(i.value()));
                imaging::Color4f color;
                ss2 >> color;
                out[colorRole] = color;
            }
        }
    }
}
