// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlPlayGLApp/IToolWidget.h>

namespace tl
{
    namespace play_gl
    {
        class App;

        //! Color tool widget.
        class ColorToolWidget : public IToolWidget
        {
            TLRENDER_NON_COPYABLE(ColorToolWidget);

        protected:
            void _init(
                const std::shared_ptr<App>&,
                const std::shared_ptr<system::Context>&,
                const std::shared_ptr<IWidget>& parent);

            ColorToolWidget();

        public:
            virtual ~ColorToolWidget();

            static std::shared_ptr<ColorToolWidget> create(
                const std::shared_ptr<App>&,
                const std::shared_ptr<system::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            TLRENDER_PRIVATE();
        };
    }
}
