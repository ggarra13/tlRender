// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlTimelineUI/IBasicItem.h>

#include <opentimelineio/transition.h>

namespace tl
{
    namespace timelineui
    {
        //! Transition item.
        class TransitionItem : public IItem
        {
        protected:
            void _init(
                const otio::SerializableObject::Retainer<otio::Transition>&,
                double scale,
                const ItemOptions&,
                const std::shared_ptr<ItemData>&,
                const std::shared_ptr<system::Context>&,
                const std::shared_ptr<IWidget>& parent);

            TransitionItem();

        public:
            virtual ~TransitionItem();

            //! Create a new item.
            static std::shared_ptr<TransitionItem> create(
                const otio::SerializableObject::Retainer<otio::Transition>&,
                double scale,
                const ItemOptions&,
                const std::shared_ptr<ItemData>&,
                const std::shared_ptr<system::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            // void setScale(double) override;
            // void setOptions(const ItemOptions&) override;

            void sizeHintEvent(const ui::SizeHintEvent&) override;
            void clipEvent(const math::Box2i&, bool) override;
            void drawEvent(const math::Box2i&, const ui::DrawEvent&) override;
            
        private:

            void _timeUnitsUpdate();
            void _textUpdate();
            
            TLRENDER_PRIVATE();
        };
    }
}
