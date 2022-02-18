// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2022 Darby Johnston
// All rights reserved.

#include <tlQWidget/TimelineSlider.h>

#include <tlQt/TimelineThumbnailProvider.h>

#include <tlCore/Math.h>
#include <tlCore/StringFormat.h>
#include <tlCore/TimelineUtil.h>

#include <QMouseEvent>
#include <QPainter>
#include <QStyle>

namespace tl
{
    namespace qwidget
    {
        namespace
        {
            const int stripeSize = 5;
            const int handleSize = 3;
        }

        struct TimelineSlider::Private
        {
            imaging::ColorConfig colorConfig;
            qt::TimelinePlayer* timelinePlayer = nullptr;
            bool thumbnails = true;
            qt::TimelineThumbnailProvider* thumbnailProvider = nullptr;
            std::map<otime::RationalTime, QImage> thumbnailImages;
            qt::TimeUnits units = qt::TimeUnits::Timecode;
            qt::TimeObject* timeObject = nullptr;
        };

        TimelineSlider::TimelineSlider(QWidget* parent) :
            QWidget(parent),
            _p(new Private)
        {
            setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
            setMinimumHeight(50);
        }
        
        TimelineSlider::~TimelineSlider()
        {}

        void TimelineSlider::setTimeObject(qt::TimeObject* timeObject)
        {
            TLRENDER_P();
            if (timeObject == p.timeObject)
                return;
            if (p.timeObject)
            {
                disconnect(
                    p.timeObject,
                    SIGNAL(unitsChanged(tl::qt::Time::Units)),
                    this,
                    SLOT(setUnits(tl::qt::Time::Units)));
            }
            p.timeObject = timeObject;
            if (p.timeObject)
            {
                p.units = p.timeObject->units();
                connect(
                    p.timeObject,
                    SIGNAL(unitsChanged(tl::qt::TimeUnits)),
                    SLOT(setUnits(tl::qt::TimeUnits)));
            }
            update();
        }

        void TimelineSlider::setColorConfig(const imaging::ColorConfig& colorConfig)
        {
            TLRENDER_P();
            if (colorConfig == p.colorConfig)
                return;
            p.colorConfig = colorConfig;
            if (p.thumbnailProvider)
            {
                p.thumbnailProvider->setColorConfig(p.colorConfig);
            }
            _thumbnailsUpdate();
        }

        void TimelineSlider::setTimelinePlayer(qt::TimelinePlayer* timelinePlayer)
        {
            TLRENDER_P();
            if (timelinePlayer == p.timelinePlayer)
                return;
            if (p.timelinePlayer)
            {
                disconnect(
                    p.timelinePlayer,
                    SIGNAL(currentTimeChanged(const otime::RationalTime&)),
                    this,
                    SLOT(update()));
            }
            p.timelinePlayer = timelinePlayer;
            if (p.timelinePlayer)
            {
                connect(
                    p.timelinePlayer,
                    SIGNAL(currentTimeChanged(const otime::RationalTime&)),
                    SLOT(update()));
                connect(
                    p.timelinePlayer,
                    SIGNAL(inOutRangeChanged(const otime::TimeRange&)),
                    SLOT(update()));
                connect(
                    p.timelinePlayer,
                    SIGNAL(cachedVideoFramesChanged(const std::vector<otime::TimeRange>&)),
                    SLOT(update()));
                connect(
                    p.timelinePlayer,
                    SIGNAL(cachedAudioFramesChanged(const std::vector<otime::TimeRange>&)),
                    SLOT(update()));
            }
            _thumbnailsUpdate();
        }

        bool TimelineSlider::hasThumbnails() const
        {
            return _p->thumbnails;
        }

        qt::TimeUnits TimelineSlider::units() const
        {
            return _p->units;
        }

        void TimelineSlider::setThumbnails(bool value)
        {
            TLRENDER_P();
            if (value == p.thumbnails)
                return;
            p.thumbnails = value;
            _thumbnailsUpdate();
            setMinimumHeight(p.thumbnails ? 50 : (stripeSize * 2 + handleSize * 2));
            updateGeometry();
        }

        void TimelineSlider::setUnits(qt::TimeUnits value)
        {
            TLRENDER_P();
            if (value == p.units)
                return;
            p.units = value;
            update();
        }

        void TimelineSlider::resizeEvent(QResizeEvent* event)
        {
            if (event->oldSize() != size())
            {
                _thumbnailsUpdate();
            }
        }

        void TimelineSlider::paintEvent(QPaintEvent*)
        {
            TLRENDER_P();
            QPainter painter(this);
            const QPalette& palette = this->palette();
            QRect rect = this->rect();
            painter.fillRect(rect, palette.color(QPalette::ColorRole::Base));
            if (p.timelinePlayer)
            {
                QRect rect2 = rect.adjusted(0, handleSize, 0, -handleSize);
                int x0 = 0;
                int y0 = 0;
                int x1 = 0;
                int y1 = 0;
                int h = 0;

                // Draw thumbnails.
                x0 = _timeToPos(p.timelinePlayer->currentTime());
                y0 = rect2.y();
                for (const auto& i : p.thumbnailImages)
                {
                    painter.drawImage(QPoint(_timeToPos(i.first), y0), i.second);
                }

                // Draw in/out points.
                const auto& inOutRange = p.timelinePlayer->inOutRange();
                x0 = _timeToPos(inOutRange.start_time());
                x1 = _timeToPos(inOutRange.end_time_inclusive());
                y1 = y0 + rect2.height();
                h = stripeSize * 2;
                painter.fillRect(
                    QRect(x0, y1 - h, x1 - x0, h),
                    palette.color(QPalette::ColorRole::Button));

                // Draw cached frames.
                auto color = QColor(40, 190, 40);
                auto cachedFrames = p.timelinePlayer->cachedVideoFrames();
                h = stripeSize;
                for (const auto& i : cachedFrames)
                {
                    x0 = _timeToPos(i.start_time());
                    x1 = _timeToPos(i.end_time_inclusive());
                    painter.fillRect(QRect(x0, y1 - h * 2, x1 - x0, h), color);
                }
                color = QColor(190, 190, 40);
                cachedFrames = p.timelinePlayer->cachedAudioFrames();
                for (const auto& i : cachedFrames)
                {
                    x0 = _timeToPos(i.start_time());
                    x1 = _timeToPos(i.end_time_inclusive());
                    painter.fillRect(QRect(x0, y1 - h, x1 - x0, h), color);
                }

                // Draw the current time.
                x0 = _timeToPos(p.timelinePlayer->currentTime());
                y0 = 0;
                painter.fillRect(
                    QRect(x0 - handleSize / 2, y0, handleSize, rect.height()),
                    palette.color(QPalette::ColorRole::Text));
            }
        }

        void TimelineSlider::mousePressEvent(QMouseEvent* event)
        {
            TLRENDER_P();
            if (p.timelinePlayer)
            {
                const auto& duration = p.timelinePlayer->duration();
                p.timelinePlayer->seek(_posToTime(event->x()));
            }
        }

        void TimelineSlider::mouseReleaseEvent(QMouseEvent*)
        {}

        void TimelineSlider::mouseMoveEvent(QMouseEvent* event)
        {
            TLRENDER_P();
            if (p.timelinePlayer)
            {
                const auto& duration = p.timelinePlayer->duration();
                p.timelinePlayer->seek(_posToTime(event->x()));
            }
        }

        void TimelineSlider::_thumbnailsCallback(const QList<QPair<otime::RationalTime, QImage> >& thumbnails)
        {
            TLRENDER_P();
            if (p.thumbnails)
            {
                for (const auto& i : thumbnails)
                {
                    p.thumbnailImages[i.first] = i.second;
                }
                update();
            }
        }

        otime::RationalTime TimelineSlider::_posToTime(int value) const
        {
            TLRENDER_P();
            otime::RationalTime out = time::invalidTime;
            if (p.timelinePlayer)
            {
                const auto& globalStartTime = p.timelinePlayer->globalStartTime();
                const auto& duration = p.timelinePlayer->duration();
                out = otime::RationalTime(
                    floor(math::clamp(value, 0, width()) / static_cast<double>(width()) * (duration.value() - 1) + globalStartTime.value()),
                    duration.rate());
            }
            return out;
        }

        int TimelineSlider::_timeToPos(const otime::RationalTime& value) const
        {
            TLRENDER_P();
            int out = 0;
            if (p.timelinePlayer)
            {
                const auto& globalStartTime = p.timelinePlayer->globalStartTime();
                const auto& duration = p.timelinePlayer->duration();
                out = (value.value() - globalStartTime.value()) / (duration.value() - 1) * width();
            }
            return out;
        }

        void TimelineSlider::_thumbnailsUpdate()
        {
            TLRENDER_P();
            if (p.thumbnailProvider)
            {
                p.thumbnailProvider->cancelRequests();
            }
            p.thumbnailImages.clear();
            if (p.timelinePlayer && p.thumbnails)
            {
                if (!p.thumbnailProvider)
                {
                    if (auto context = p.timelinePlayer->context().lock())
                    {
                        timeline::Options options;
                        options.videoRequestCount = 1;
                        options.audioRequestCount = 1;
                        options.requestTimeout = std::chrono::milliseconds(100);
                        options.avioOptions["SequenceIO/ThreadCount"] = string::Format("{0}").arg(1);
                        options.avioOptions["ffmpeg/ThreadCount"] = string::Format("{0}").arg(1);
                        auto timeline = timeline::Timeline::create(p.timelinePlayer->timeline()->getPath().get(), context, options);
                        p.thumbnailProvider = new qt::TimelineThumbnailProvider(timeline, context, this);
                        p.thumbnailProvider->setColorConfig(p.colorConfig);
                        connect(
                            p.thumbnailProvider,
                            SIGNAL(thumbails(const QList<QPair<otime::RationalTime, QImage> >&)),
                            SLOT(_thumbnailsCallback(const QList<QPair<otime::RationalTime, QImage> >&)));
                    }
                }

                const auto& duration = p.timelinePlayer->duration();
                const auto& info = p.timelinePlayer->avInfo();
                const auto rect = this->rect().adjusted(0, 0, 0, -(stripeSize * 2 + handleSize * 2));
                const int width = rect.width();
                const int height = rect.height();
                const int thumbnailWidth = !info.video.empty() ?
                    static_cast<int>(height * info.video[0].size.getAspect()) :
                    0;
                const int thumbnailHeight = height;
                if (thumbnailWidth > 0)
                {
                    QList<otime::RationalTime> requests;
                    int x = rect.x();
                    while (x < width)
                    {
                        requests.push_back(_posToTime(x));
                        x += thumbnailWidth;
                    }
                    p.thumbnailProvider->request(requests, QSize(thumbnailWidth, thumbnailHeight));
                }
            }
            else if (p.thumbnailProvider)
            {
                delete p.thumbnailProvider;
                p.thumbnailProvider = nullptr;
            }
            update();
        }
    }
}
