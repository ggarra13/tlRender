// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlPlayGLApp/FileMenu.h>

#include <tlPlayGLApp/App.h>

namespace tl
{
    namespace play_gl
    {
        struct FileMenu::Private
        {
            std::weak_ptr<App> app;
            std::shared_ptr<timeline::Player> player;

            std::shared_ptr<Menu> recentMenu;
            std::shared_ptr<Menu> currentMenu;

            std::shared_ptr<observer::ListObserver<std::shared_ptr<timeline::Player> > > playerObserver;
        };

        void FileMenu::_init(
            const std::shared_ptr<App>& app,
            const std::shared_ptr<system::Context>& context)
        {
            Menu::_init(context);
            TLRENDER_P();

            p.app = app;

            auto appWeak = std::weak_ptr<App>(app);
            auto item = std::make_shared<ui::MenuItem>(
                "Open",
                "FileOpen",
                ui::Key::O,
                static_cast<int>(ui::KeyModifier::Control),
                [this, appWeak]
                {
                    close();
                    if (auto app = appWeak.lock())
                    {
                        app->openDialog();
                    }
                });
            addItem(item);

            item = std::make_shared<ui::MenuItem>(
                "Open With Separate Audio",
                "FileOpenSeparateAudio",
                ui::Key::O,
                static_cast<int>(ui::KeyModifier::Shift) |
                static_cast<int>(ui::KeyModifier::Control),
                [this]
                {
                    close();
                });
            addItem(item);
            setItemEnabled(item, false);

            item = std::make_shared<ui::MenuItem>(
                "Close",
                "FileClose",
                ui::Key::E,
                static_cast<int>(ui::KeyModifier::Control),
                [this, appWeak]
                {
                    close();
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->close();
                    }
                });
            addItem(item);

            item = std::make_shared<ui::MenuItem>(
                "Close All",
                "FileCloseAll",
                ui::Key::E,
                static_cast<int>(ui::KeyModifier::Shift) |
                static_cast<int>(ui::KeyModifier::Control),
                [this, appWeak]
                {
                    close();
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->closeAll();
                    }
                });
            addItem(item);

            item = std::make_shared<ui::MenuItem>(
                "Reload",
                [this, appWeak]
                {
                    close();
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->reload();
                    }
                });
            addItem(item);

            p.recentMenu = addSubMenu("Recent");
            for (size_t i = 0; i < 10; ++i)
            {
                item = std::make_shared<ui::MenuItem>(
                    "File Name",
                    [this]
                    {
                        close();
                    });
                p.recentMenu->addItem(item);
                p.recentMenu->setItemEnabled(item, false);
            }

            addDivider();

            p.currentMenu = addSubMenu("Current");
            for (size_t i = 0; i < 10; ++i)
            {
                item = std::make_shared<ui::MenuItem>(
                    "File Name",
                    [this]
                    {
                        close();
                    });
                p.currentMenu->addItem(item);
                p.currentMenu->setItemEnabled(item, false);
            }

            item = std::make_shared<ui::MenuItem>(
                "Next",
                "Next",
                ui::Key::PageDown,
                static_cast<int>(ui::KeyModifier::Control),
                [this, appWeak]
                {
                    close();
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->next();
                    }
                });
            addItem(item);

            item = std::make_shared<ui::MenuItem>(
                "Previous",
                "Prev",
                ui::Key::PageUp,
                static_cast<int>(ui::KeyModifier::Control),
                [this, appWeak]
                {
                    close();
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->prev();
                    }
                });
            addItem(item);

            addDivider();

            item = std::make_shared<ui::MenuItem>(
                "Next Layer",
                ui::Key::Equal,
                static_cast<int>(ui::KeyModifier::Control),
                [this, appWeak]
                {
                    close();
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->nextLayer();
                    }
                });
            addItem(item);

            item = std::make_shared<ui::MenuItem>(
                "Previous Layer",
                "Prev",
                ui::Key::Minus,
                static_cast<int>(ui::KeyModifier::Control),
                [this, appWeak]
                {
                    close();
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->prevLayer();
                    }
                });
            addItem(item);

            addDivider();

            item = std::make_shared<ui::MenuItem>(
                "Exit",
                ui::Key::Q,
                static_cast<int>(ui::KeyModifier::Control),
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        app->exit();
                    }
                });
            addItem(item);

            p.playerObserver = observer::ListObserver<std::shared_ptr<timeline::Player> >::create(
                app->observeActivePlayers(),
                [this](const std::vector<std::shared_ptr<timeline::Player> >& value)
                {
                    _p->player = !value.empty() ? value[0] : nullptr;
                });
        }

        FileMenu::FileMenu() :
            _p(new Private)
        {}

        FileMenu::~FileMenu()
        {}

        std::shared_ptr<FileMenu> FileMenu::create(
            const std::shared_ptr<App>& app,
            const std::shared_ptr<system::Context>& context)
        {
            auto out = std::shared_ptr<FileMenu>(new FileMenu);
            out->_init(app, context);
            return out;
        }

        void FileMenu::close()
        {
            Menu::close();
            TLRENDER_P();
            p.recentMenu->close();
            p.currentMenu->close();
        }
    }
}
