// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlUI/IWidget.h>
#include <tlUI/IntModel.h>

namespace tl
{
    namespace ui
    {
        //! Integer number editor.
        class IntEdit : public IWidget
        {
            TLRENDER_NON_COPYABLE(IntEdit);

        protected:
            void _init(
                const std::shared_ptr<system::Context>&,
                const std::shared_ptr<IntModel>&,
                const std::shared_ptr<IWidget>& parent);

            IntEdit();

        public:
            virtual ~IntEdit();

            //! Create a new widget.
            static std::shared_ptr<IntEdit> create(
                const std::shared_ptr<system::Context>&,
                const std::shared_ptr<IntModel>& = nullptr,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Get the model.
            const std::shared_ptr<IntModel>& getModel() const;

            //! Set the number of digits to display.
            void setDigits(int);

            //! Set the font role.
            void setFontRole(FontRole);

            void setGeometry(const math::BBox2i&) override;
            void sizeHintEvent(const SizeHintEvent&) override;
            void keyPressEvent(KeyEvent&) override;
            void keyReleaseEvent(KeyEvent&) override;

        private:
            void _textUpdate();

            TLRENDER_PRIVATE();
        };
    }
}
