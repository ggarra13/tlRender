// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlUI/IWidget.h>

namespace tl
{
    namespace ui
    {
        void IWidget::_init(
            const std::string& name,
            const std::shared_ptr<system::Context>& context,
            const std::shared_ptr<IWidget>& parent)
        {
            _context = context;
            _name = name;
            if (parent)
            {
                parent->_children.push_back(
                    std::static_pointer_cast<IWidget>(shared_from_this()));
            }
            _parent = parent;
        }

        IWidget::IWidget()
        {}

        IWidget::~IWidget()
        {}

        void IWidget::setName(const std::string& value)
        {
            _name = value;
        }

        void IWidget::setBackgroundRole(ColorRole value)
        {
            if (value == _backgroundRole)
                return;
            _backgroundRole = value;
            _updates |= Update::Draw;
        }

        void IWidget::setParent(const std::shared_ptr<IWidget>& value)
        {
            if (auto parent = _parent.lock())
            {
                auto i = std::find(
                    parent->_children.begin(),
                    parent->_children.end(),
                    shared_from_this());
                if (i != parent->_children.end())
                {
                    ChildEvent event;
                    event.child = *i;
                    parent->_children.erase(i);
                    parent->childRemovedEvent(event);
                    parent->_updates |= Update::Size;
                    parent->_updates |= Update::Draw;
                }
            }
            _parent = value;
            if (value)
            {
                value->_children.push_back(
                    std::static_pointer_cast<IWidget>(shared_from_this()));
                ChildEvent event;
                event.child = shared_from_this();
                value->childAddedEvent(event);
                value->_updates |= Update::Size;
                value->_updates |= Update::Draw;
            }
        }

        std::shared_ptr<IWidget> IWidget::getTopLevel() const
        {
            std::shared_ptr<IWidget> out;
            auto parent = _parent.lock();
            while (parent)
            {
                out = parent;
                parent = parent->_parent.lock();
            }
            return out;
        }

        void IWidget::setEventLoop(const std::weak_ptr<EventLoop>& value)
        {
            _eventLoop = value;
        }

        void IWidget::setHStretch(Stretch value)
        {
            if (value == _hStretch)
                return;
            _hStretch = value;
            _updates |= Update::Size;
            _updates |= Update::Draw;
        }

        void IWidget::setVStretch(Stretch value)
        {
            if (value == _vStretch)
                return;
            _vStretch = value;
            _updates |= Update::Size;
            _updates |= Update::Draw;
        }

        void IWidget::setHAlign(HAlign value)
        {
            if (value == _hAlign)
                return;
            _hAlign = value;
            _updates |= Update::Size;
            _updates |= Update::Draw;
        }

        void IWidget::setVAlign(VAlign value)
        {
            if (value == _vAlign)
                return;
            _vAlign = value;
            _updates |= Update::Size;
            _updates |= Update::Draw;
        }

        void IWidget::setGeometry(const math::BBox2i& value)
        {
            if (value == _geometry)
                return;
            _geometry = value;
            _updates |= Update::Size;
            _updates |= Update::Draw;
        }

        void IWidget::setVisible(bool value)
        {
            if (value == _visible)
                return;
            _visible = value;
            _updates |= Update::Size;
            _updates |= Update::Draw;
        }

        bool IWidget::acceptsKeyFocus() const
        {
            return false;
        }

        void IWidget::childAddedEvent(const ChildEvent&)
        {}

        void IWidget::childRemovedEvent(const ChildEvent&)
        {}

        void IWidget::tickEvent(const TickEvent&)
        {}

        void IWidget::sizeEvent(const SizeEvent&)
        {
            _updates &= ~static_cast<int>(Update::Size);
        }

        void IWidget::drawEvent(const DrawEvent& event)
        {
            _updates &= ~static_cast<int>(Update::Draw);
            if (_backgroundRole != ColorRole::None)
            {
                event.render->drawRect(
                    _geometry,
                    event.style->getColorRole(_backgroundRole));
            }
        }

        void IWidget::enterEvent()
        {}

        void IWidget::leaveEvent()
        {}

        void IWidget::mouseMoveEvent(MouseMoveEvent&)
        {}

        void IWidget::mousePressEvent(MouseClickEvent&)
        {}

        void IWidget::mouseReleaseEvent(MouseClickEvent&)
        {}

        void IWidget::keyPressEvent(KeyEvent&)
        {}

        void IWidget::keyReleaseEvent(KeyEvent&)
        {}
    }
}
