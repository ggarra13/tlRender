// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlUI/FileBrowser.h>

namespace tl
{
    namespace play_gl
    {
        class App;

        //! Separate audio dialog.
        class SeparateAudioDialog : public ui::IDialog
        {
            TLRENDER_NON_COPYABLE(SeparateAudioDialog);

        protected:
            void _init(
                const std::string&,
                const std::shared_ptr<system::Context>&,
                const std::shared_ptr<IWidget>& parent);

            SeparateAudioDialog();

        public:
            virtual ~SeparateAudioDialog();

            static std::shared_ptr<SeparateAudioDialog> create(
                const std::string&,
                const std::shared_ptr<system::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the file callback.
            void setFileCallback(const std::function<void(
                const file::Path&,
                const file::Path&)>&);

            //! Get the options.
            const ui::FileBrowserOptions& getOptions() const;

            //! Set the options.
            void setOptions(const ui::FileBrowserOptions&);

        private:
            TLRENDER_PRIVATE();
        };
    }
}
