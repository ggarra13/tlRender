// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlTimelineUI/AudioClipItem.h>

#include <tlUI/DrawUtil.h>

#include <tlTimeline/RenderUtil.h>
#include <tlTimeline/Util.h>

#include <tlCore/AudioConvert.h>
#include <tlCore/Mesh.h>

#include <opentimelineio/track.h>

#include <sstream>

namespace tl
{
    namespace timelineui
    {
        struct AudioClipItem::Private
        {
            const otio::Clip* clip = nullptr;
            const otio::Track* track = nullptr;
            file::Path path;
            std::vector<file::MemoryRead> memoryRead;
            otime::TimeRange timeRange = time::invalidTimeRange;
            std::string label;
            ui::FontRole labelFontRole = ui::FontRole::Label;
            std::string durationLabel;
            ui::FontRole durationFontRole = ui::FontRole::Mono;
            bool ioInfoInit = true;
            io::Info ioInfo;

            struct SizeData
            {
                int margin = 0;
                int spacing = 0;
                int border = 0;
                imaging::FontInfo labelFontInfo = imaging::FontInfo("", 0);
                imaging::FontInfo durationFontInfo = imaging::FontInfo("", 0);
                int lineHeight = 0;
                bool textUpdate = true;
                math::Vector2i labelSize;
                math::Vector2i durationSize;
                int waveformWidth = 0;
                math::BBox2i clipRect;
            };
            SizeData size;

            struct DrawData
            {
                std::vector<std::shared_ptr<imaging::Glyph> > labelGlyphs;
                std::vector<std::shared_ptr<imaging::Glyph> > durationGlyphs;
            };
            DrawData draw;

            struct AudioFuture
            {
                std::future<io::AudioData> future;
                math::Vector2i size;
            };
            std::map<otime::RationalTime, AudioFuture> audioDataFutures;
            struct AudioData
            {
                io::AudioData audio;
                math::Vector2i size;
                std::future<std::shared_ptr<geom::TriangleMesh2> > meshFuture;
                std::shared_ptr<geom::TriangleMesh2> mesh;
                std::chrono::steady_clock::time_point time;
            };
            std::map<otime::RationalTime, AudioData> audioData;
            std::shared_ptr<observer::ValueObserver<bool> > cancelObserver;
        };

        void AudioClipItem::_init(
            const otio::Clip* clip,
            const ItemData& itemData,
            const std::shared_ptr<system::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            IItem::_init("tl::timelineui::AudioClipItem", itemData, context, parent);
            TLRENDER_P();

            p.clip = clip;
            p.track = dynamic_cast<otio::Track*>(clip->parent());

            p.path = timeline::getPath(
                p.clip->media_reference(),
                itemData.directory,
                itemData.pathOptions);
            p.memoryRead = timeline::getMemoryRead(
                p.clip->media_reference());

            auto rangeOpt = clip->trimmed_range_in_parent();
            if (rangeOpt.has_value())
            {
                p.timeRange = rangeOpt.value();
            }

            p.label = p.path.get(-1, false);
            _textUpdate();

            p.cancelObserver = observer::ValueObserver<bool>::create(
                _data.ioManager->observeCancelRequests(),
                [this](bool)
                {
                    _p->audioDataFutures.clear();
                });
        }

        AudioClipItem::AudioClipItem() :
            _p(new Private)
        {}

        AudioClipItem::~AudioClipItem()
        {}

        std::shared_ptr<AudioClipItem> AudioClipItem::create(
            const otio::Clip* clip,
            const ItemData& itemData,
            const std::shared_ptr<system::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<AudioClipItem>(new AudioClipItem);
            out->_init(clip, itemData, context, parent);
            return out;
        }

        void AudioClipItem::setScale(double value)
        {
            const bool changed = value != _scale;
            IItem::setScale(value);
            TLRENDER_P();
            if (changed)
            {
                _data.ioManager->cancelRequests();
                p.audioData.clear();
                _updates |= ui::Update::Draw;
            }
        }

        void AudioClipItem::setOptions(const ItemOptions& value)
        {
            const bool thumbnailsChanged =
                value.thumbnails != _options.thumbnails ||
                value.waveformHeight != _options.waveformHeight;
            IItem::setOptions(value);
            TLRENDER_P();
            if (thumbnailsChanged)
            {
                _data.ioManager->cancelRequests();
                p.audioData.clear();
                _updates |= ui::Update::Draw;
            }
        }

        namespace
        {
            std::shared_ptr<geom::TriangleMesh2> audioMesh(
                const std::shared_ptr<audio::Audio>& audio,
                const math::Vector2i& size)
            {
                auto out = std::shared_ptr<geom::TriangleMesh2>(new geom::TriangleMesh2);
                const auto& info = audio->getInfo();
                const size_t sampleCount = audio->getSampleCount();
                if (sampleCount > 0)
                {
                    switch (info.dataType)
                    {
                    case audio::DataType::F32:
                    {
                        const audio::F32_T* data = reinterpret_cast<const audio::F32_T*>(
                            audio->getData());
                        for (int x = 0; x < size.x; ++x)
                        {
                            const int x0 = std::min(
                                static_cast<size_t>((x + 0) / static_cast<double>(size.x - 1) * (sampleCount - 1)),
                                sampleCount - 1);
                            const int x1 = std::min(
                                static_cast<size_t>((x + 1) / static_cast<double>(size.x - 1) * (sampleCount - 1)),
                                sampleCount - 1);
                            //std::cout << x << ": " << x0 << " " << x1 << std::endl;
                            audio::F32_T min = 0.F;
                            audio::F32_T max = 0.F;
                            if (x0 < x1)
                            {
                                min = audio::F32Range.getMax();
                                max = audio::F32Range.getMin();
                                for (int i = x0; i < x1; ++i)
                                {
                                    const audio::F32_T v = *(data + i * info.channelCount);
                                    min = std::min(min, v);
                                    max = std::max(max, v);
                                }
                            }
                            const int h2 = size.y / 2;
                            const math::BBox2i bbox(
                                math::Vector2i(
                                    x,
                                    h2 - h2 * max),
                                math::Vector2i(
                                    x + 1,
                                    h2 - h2 * min));
                            if (bbox.isValid())
                            {
                                const size_t j = 1 + out->v.size();
                                out->v.push_back(math::Vector2f(bbox.x(), bbox.y()));
                                out->v.push_back(math::Vector2f(bbox.x() + bbox.w(), bbox.y()));
                                out->v.push_back(math::Vector2f(bbox.x() + bbox.w(), bbox.y() + bbox.h()));
                                out->v.push_back(math::Vector2f(bbox.x(), bbox.y() + bbox.h()));
                                out->triangles.push_back(geom::Triangle2({ j + 0, j + 1, j + 2 }));
                                out->triangles.push_back(geom::Triangle2({ j + 2, j + 3, j + 0 }));
                            }
                        }
                        break;
                    }
                    default: break;
                    }
                }
                return out;
            }
        }

        void AudioClipItem::tickEvent(
            bool parentsVisible,
            bool parentsEnabled,
            const ui::TickEvent& event)
        {
            IWidget::tickEvent(parentsVisible, parentsEnabled, event);
            TLRENDER_P();
            auto i = p.audioDataFutures.begin();
            while (i != p.audioDataFutures.end())
            {
                if (i->second.future.valid() &&
                    i->second.future.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                {
                    const auto audio = i->second.future.get();
                    const auto size = i->second.size;
                    Private::AudioData audioData;
                    audioData.audio = audio;
                    audioData.size = size;
                    if (audio.audio)
                    {
                        audioData.meshFuture = std::async(
                            std::launch::async,
                            [audio, size]
                            {
                                auto convert = audio::AudioConvert::create(
                                    audio.audio->getInfo(),
                                    audio::Info(1, audio::DataType::F32, audio.audio->getSampleRate()));
                                const auto convertedAudio = convert->convert(audio.audio);
                                return audioMesh(convertedAudio, size);
                            });
                    }
                    p.audioData[i->first] = std::move(audioData);
                    i = p.audioDataFutures.erase(i);
                    continue;
                }
                ++i;
            }

            const auto now = std::chrono::steady_clock::now();
            for (auto& audioData : p.audioData)
            {
                if (audioData.second.meshFuture.valid() &&
                    audioData.second.meshFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                {
                    const auto mesh = audioData.second.meshFuture.get();
                    audioData.second.mesh = mesh;
                    audioData.second.time = now;
                    _updates |= ui::Update::Draw;
                }
                const std::chrono::duration<float> diff = now - audioData.second.time;
                if (diff.count() < _options.thumbnailFade)
                {
                    _updates |= ui::Update::Draw;
                }
            }
        }

        void AudioClipItem::sizeHintEvent(const ui::SizeHintEvent& event)
        {
            IItem::sizeHintEvent(event);
            TLRENDER_P();

            p.size.margin = event.style->getSizeRole(ui::SizeRole::MarginSmall, event.displayScale);
            p.size.spacing = event.style->getSizeRole(ui::SizeRole::SpacingSmall, event.displayScale);
            p.size.border = event.style->getSizeRole(ui::SizeRole::Border, event.displayScale);

            auto fontInfo = event.style->getFontRole(p.labelFontRole, event.displayScale);
            if (fontInfo != p.size.labelFontInfo || p.size.textUpdate)
            {
                p.size.labelFontInfo = event.style->getFontRole(p.labelFontRole, event.displayScale);
                auto fontMetrics = event.getFontMetrics(p.labelFontRole);
                p.size.lineHeight = fontMetrics.lineHeight;
                p.size.labelSize = event.fontSystem->getSize(p.label, p.size.labelFontInfo);
            }
            fontInfo = event.style->getFontRole(p.durationFontRole, event.displayScale);
            if (fontInfo != p.size.durationFontInfo || p.size.textUpdate)
            {
                p.size.durationFontInfo = fontInfo;
                p.size.durationSize = event.fontSystem->getSize(p.durationLabel, p.size.durationFontInfo);
            }
            p.size.textUpdate = false;

            const int waveformWidth = _options.thumbnails ?
                (otime::RationalTime(1.0, 1.0).value() * _scale) :
                0;
            if (waveformWidth != p.size.waveformWidth)
            {
                p.size.waveformWidth = waveformWidth;
                _data.ioManager->cancelRequests();
                p.audioData.clear();
                _updates |= ui::Update::Draw;
            }

            _sizeHint = math::Vector2i(
                p.timeRange.duration().rescaled_to(1.0).value() * _scale,
                p.size.margin +
                p.size.lineHeight +
                p.size.margin);
            if (_options.thumbnails)
            {
                _sizeHint.y += p.size.spacing + _options.waveformHeight;
            }
        }

        void AudioClipItem::clipEvent(
            const math::BBox2i& clipRect,
            bool clipped,
            const ui::ClipEvent& event)
        {
            IItem::clipEvent(clipRect, clipped, event);
            TLRENDER_P();
            if (clipRect == p.size.clipRect)
                return;
            p.size.clipRect = clipRect;
            if (clipped)
            {
                p.draw.labelGlyphs.clear();
                p.draw.durationGlyphs.clear();
            }
            _data.ioManager->cancelRequests();
            _updates |= ui::Update::Draw;
        }

        void AudioClipItem::drawEvent(
            const math::BBox2i& drawRect,
            const ui::DrawEvent& event)
        {
            IItem::drawEvent(drawRect, event);
            TLRENDER_P();

            const math::BBox2i& g = _geometry;

            const math::BBox2i g2 = g.margin(-p.size.border);
            event.render->drawMesh(
                ui::rect(g2, p.size.margin),
                math::Vector2i(),
                _options.colors[ColorRole::AudioClip]);

            _drawInfo(drawRect, event);
            if (_options.thumbnails)
            {
                _drawWaveforms(drawRect, event);
            }
        }

        void AudioClipItem::_timeUnitsUpdate(timeline::TimeUnits value)
        {
            IItem::_timeUnitsUpdate(value);
            _textUpdate();
        }

        void AudioClipItem::_textUpdate()
        {
            TLRENDER_P();
            p.durationLabel = IItem::_durationLabel(p.timeRange.duration());
            p.size.textUpdate = true;
            p.draw.durationGlyphs.clear();
            _updates |= ui::Update::Size;
            _updates |= ui::Update::Draw;
        }

        void AudioClipItem::_drawInfo(
            const math::BBox2i& drawRect,
            const ui::DrawEvent& event)
        {
            TLRENDER_P();

            const math::BBox2i& g = _geometry;

            const math::BBox2i labelGeometry(
                g.min.x +
                p.size.margin,
                g.min.y +
                p.size.margin,
                p.size.labelSize.x,
                p.size.lineHeight);
            const math::BBox2i durationGeometry(
                g.max.x -
                p.size.margin -
                p.size.durationSize.x,
                g.min.y +
                p.size.margin,
                p.size.durationSize.x,
                p.size.lineHeight);
            const bool labelVisible = drawRect.intersects(labelGeometry);
            const bool durationVisible =
                drawRect.intersects(durationGeometry) &&
                !durationGeometry.intersects(labelGeometry);

            if (labelVisible)
            {
                if (!p.label.empty() && p.draw.labelGlyphs.empty())
                {
                    p.draw.labelGlyphs = event.fontSystem->getGlyphs(p.label, p.size.labelFontInfo);
                }
                const auto fontMetrics = event.getFontMetrics(p.labelFontRole);
                event.render->drawText(
                    p.draw.labelGlyphs,
                    math::Vector2i(
                        labelGeometry.min.x,
                        labelGeometry.min.y +
                        fontMetrics.ascender),
                    event.style->getColorRole(ui::ColorRole::Text));
            }

            if (durationVisible)
            {
                if (!p.durationLabel.empty() && p.draw.durationGlyphs.empty())
                {
                    p.draw.durationGlyphs = event.fontSystem->getGlyphs(p.durationLabel, p.size.durationFontInfo);
                }
                const auto fontMetrics = event.getFontMetrics(p.durationFontRole);
                event.render->drawText(
                    p.draw.durationGlyphs,
                    math::Vector2i(
                        durationGeometry.min.x,
                        durationGeometry.min.y +
                        p.size.lineHeight / 2 -
                        fontMetrics.lineHeight / 2 +
                        fontMetrics.ascender),
                    event.style->getColorRole(ui::ColorRole::Text));
            }
        }

        void AudioClipItem::_drawWaveforms(
            const math::BBox2i& drawRect,
            const ui::DrawEvent& event)
        {
            TLRENDER_P();

            const math::BBox2i& g = _geometry;
            const auto now = std::chrono::steady_clock::now();

            const math::BBox2i bbox(
                g.min.x +
                p.size.margin,
                g.min.y +
                p.size.margin +
                p.size.lineHeight +
                p.size.spacing,
                _sizeHint.x - p.size.margin * 2,
                _options.waveformHeight);
            event.render->drawRect(
                bbox,
                imaging::Color4f(0.F, 0.F, 0.F));
            const timeline::ClipRectEnabledState clipRectEnabledState(event.render);
            const timeline::ClipRectState clipRectState(event.render);
            event.render->setClipRectEnabled(true);
            event.render->setClipRect(bbox.intersect(clipRectState.getClipRect()));

            std::set<otime::RationalTime> audioDataDelete;
            for (const auto& audioData : p.audioData)
            {
                audioDataDelete.insert(audioData.first);
            }

            const math::BBox2i clipRect = _getClipRect(
                drawRect,
                _options.clipRectScale);
            if (g.intersects(clipRect))
            {
                if (p.ioInfoInit)
                {
                    p.ioInfoInit = false;
                    p.ioInfo = _data.ioManager->getInfo(p.path, p.memoryRead).get();
                    _updates |= ui::Update::Size;
                    _updates |= ui::Update::Draw;
                }
            }

            if (p.size.waveformWidth > 0)
            {
                const int w = _sizeHint.x - p.size.margin * 2;
                for (int x = 0; x < w; x += p.size.waveformWidth)
                {
                    math::BBox2i bbox(
                        g.min.x +
                        p.size.margin +
                        x,
                        g.min.y +
                        p.size.margin +
                        p.size.lineHeight +
                        p.size.spacing,
                        p.size.waveformWidth,
                        _options.waveformHeight);
                    if (bbox.intersects(clipRect))
                    {
                        const otime::RationalTime time = time::round(otime::RationalTime(
                            p.timeRange.start_time().value() +
                            (w > 0 ? (x / static_cast<double>(w)) : 0) *
                            p.timeRange.duration().value(),
                            p.timeRange.duration().rate()));
                        auto i = p.audioData.find(time);
                        if (i != p.audioData.end())
                        {
                            if (i->second.mesh)
                            {
                                const std::chrono::duration<float> diff = now - i->second.time;
                                const float a = std::min(diff.count() / _options.thumbnailFade, 1.F);
                                event.render->drawMesh(
                                    *i->second.mesh,
                                    bbox.min,
                                    imaging::Color4f(1.F, 1.F, 1.F, a));
                            }
                            audioDataDelete.erase(time);
                        }
                        else if (p.ioInfo.audio.isValid())
                        {
                            const auto j = p.audioDataFutures.find(time);
                            if (j == p.audioDataFutures.end())
                            {
                                const otime::TimeRange mediaRange = timeline::toAudioMediaTime(
                                    otime::TimeRange(time, otime::RationalTime(time.rate(), time.rate())),
                                    p.track,
                                    p.clip,
                                    p.ioInfo);
                                p.audioDataFutures[time].future = _data.ioManager->readAudio(
                                    p.path,
                                    p.memoryRead,
                                    mediaRange);
                                p.audioDataFutures[time].size = bbox.getSize();
                            }
                        }
                    }
                }
            }

            for (auto i : audioDataDelete)
            {
                const auto j = p.audioData.find(i);
                if (j != p.audioData.end())
                {
                    p.audioData.erase(j);
                }
            }
        }
    }
}
