// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlTimelineUI/IOManager.h>

#include <tlUI/IWidget.h>

#include <tlTimeline/TimeUnits.h>
#include <tlTimeline/Timeline.h>

#include <opentimelineio/item.h>

namespace tl
{
    namespace timelineui
    {
        class IItem;

        //! Item data.
        struct ItemData
        {
            float speed = 0.0;
            std::string directory;
            timeline::Options options;
            std::shared_ptr<IOManager> ioManager;
            std::shared_ptr<timeline::ITimeUnitsModel> timeUnitsModel;
        };

        //! In/out points display options.
        enum class InOutDisplay
        {
            InsideRange,
            OutsideRange
        };
        
        //! Cache display options.
        enum class CacheDisplay
        {
            VideoAndAudio,
            VideoOnly
        };

        //! Waveform primitive type.
        enum class WaveformPrim
        {
            Mesh,
            Image
        };

        //! Item options.
        struct ItemOptions
        {
            InOutDisplay inOutDisplay = InOutDisplay::InsideRange;
            CacheDisplay cacheDisplay = CacheDisplay::VideoAndAudio;
            float clipRectScale = 2.F;
            bool thumbnails = true;
            int thumbnailHeight = 100;
            int waveformWidth = 200;
            int waveformHeight = 50;
            WaveformPrim waveformPrim = WaveformPrim::Mesh;
            float thumbnailFade = .2F;
            bool showTransitions = false;
            bool showMarkers = false;
            std::string regularFont = "NotoSans-Regular";
            std::string monoFont = "NotoMono-Regular";
            int fontSize = 12;

            bool operator == (const ItemOptions&) const;
            bool operator != (const ItemOptions&) const;
        };

        //! Marker.
        struct Marker
        {
            std::string name;
            image::Color4f color;
            otime::TimeRange range;
        };

        //! Get the markers from an item.
        std::vector<Marker> getMarkers(const otio::Item*);

        //! Convert a named marker color.
        image::Color4f getMarkerColor(const std::string&);

        //! Drag and drop data.
        class DragAndDropData : public ui::DragAndDropData
        {
        public:
            DragAndDropData(const std::shared_ptr<IItem>&);

            virtual ~DragAndDropData();

            const std::shared_ptr<IItem>& getItem() const;

        private:
            std::shared_ptr<IItem> _item;
        };

        //! Base class for items.
        class IItem : public ui::IWidget
        {
        protected:
            void _init(
                const std::string& objectName,
                const otio::SerializableObject::Retainer<otio::Composable>&,
                const otime::TimeRange&,
                const ItemData&,
                const std::shared_ptr<system::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            IItem();

        public:
            virtual ~IItem();

            //! Get the OTIO object.
            const otio::SerializableObject::Retainer<otio::Composable>& getComposable() const;
            
            //! Get the item time range.
            const otime::TimeRange& getTimeRange() const;

            //! Set the item scale.
            virtual void setScale(double);

            //! Set the item options.
            virtual void setOptions(const ItemOptions&);

            //! Get the selection color role.
            ui::ColorRole getSelectRole() const;

            //! Set the selection color role.
            void setSelectRole(ui::ColorRole);

        protected:
            static math::Box2i _getClipRect(
                const math::Box2i&,
                double scale);

            std::string _getDurationLabel(const otime::RationalTime&);

            otime::RationalTime _posToTime(float) const;
            int _timeToPos(const otime::RationalTime&) const;

            virtual void _timeUnitsUpdate();

            otio::SerializableObject::Retainer<otio::Composable> _composable;
            otime::TimeRange _timeRange = time::invalidTimeRange;
            ItemData _data;
            double _scale = 500.0;
            ItemOptions _options;

        private:
            TLRENDER_PRIVATE();
        };
    }
}
