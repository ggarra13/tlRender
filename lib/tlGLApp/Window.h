// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlUI/IWindow.h>

namespace tl
{
    namespace gl
    {
        class GLFWWindow;
    }

    namespace timeline
    {
        struct LUTOptions;
        struct OCIOOptions;
    }

    namespace gl_app
    {
        //! Window.
        class Window : public ui::IWindow
        {
            TLRENDER_NON_COPYABLE(Window);

        protected:
            void _init(
                const std::string& name,
                const std::shared_ptr<system::Context>&);

            Window();

        public:
            virtual ~Window();

            //! Create a new window.
            static std::shared_ptr<Window> create(
                const std::string& name,
                const std::shared_ptr<system::Context>&);

            //! Observe the window size.
            std::shared_ptr<observer::IValue<math::Size2i> > observeWindowSize() const;

            //! Set the window size.
            void setWindowSize(const math::Size2i&);

            //! Observe whether the window is visible.
            std::shared_ptr<observer::IValue<bool> > observeVisible() const;

            //! Get which screen the window is on.
            int getScreen() const;

            //! Get whether the window is in full screen mode.
            bool isFullScreen() const;

            //! Observe whether the window is in full screen mode.
            std::shared_ptr<observer::IValue<bool> > observeFullScreen() const;

            //! Set whether the window is in full screen mode.
            void setFullScreen(bool, int screen = -1);

            //! Get whether the window is floating on top.
            bool isFloatOnTop() const;

            //! Observe whether the window is floating on top.
            std::shared_ptr<observer::IValue<bool> > observeFloatOnTop() const;

            //! Set whether the window is floating on top.
            void setFloatOnTop(bool);

            //! Observe when the window is closed.
            std::shared_ptr<observer::IValue<bool> > observeClose() const;

            //! Get the GLFW window.
            const std::shared_ptr<gl::GLFWWindow>& getGLFWWindow() const;

            void setGeometry(const math::Box2i&) override;
            void setVisible(bool) override;
            void tickEvent(
                bool parentsVisible,
                bool parentsEnabled,
                const ui::TickEvent&) override;

        protected:
            void _makeCurrent();
            void _doneCurrent();

            void _setOCIOOptions(const timeline::OCIOOptions&);
            void _setLUTOptions(const timeline::LUTOptions&);

        private:
            bool _getSizeUpdate(const std::shared_ptr<IWidget>&) const;
            void _sizeHintEvent(
                const std::shared_ptr<IWidget>&,
                const ui::SizeHintEvent&);

            bool _getDrawUpdate(const std::shared_ptr<IWidget>&) const;
            void _drawEvent(
                const std::shared_ptr<IWidget>&,
                const math::Box2i&,
                const ui::DrawEvent&);

            TLRENDER_PRIVATE();
        };
    }
}
