// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#pragma once

#include <tlQt/TimelinePlayer.h>

#include <QMainWindow>

namespace tl
{
    namespace qtwidget
    {
        class TimelineWidget;
        class TimelineViewport;
    }

    namespace play_qt
    {
        class App;

        //! Main window.
        class MainWindow : public QMainWindow
        {
            Q_OBJECT

        public:
            MainWindow(App*, QWidget* parent = nullptr);

            virtual ~MainWindow();

            //! Get the timeline widget;
            qtwidget::TimelineWidget* timelineWidget() const;

            //! Get the timeline viewport;
            qtwidget::TimelineViewport* timelineViewport() const;

        protected:
            void dragEnterEvent(QDragEnterEvent*) override;
            void dragMoveEvent(QDragMoveEvent*) override;
            void dragLeaveEvent(QDragLeaveEvent*) override;
            void dropEvent(QDropEvent*) override;

        private Q_SLOTS:
            void _speedCallback(double);
            void _playbackCallback(tl::timeline::Playback);
            void _currentTimeCallback(const otime::RationalTime&);
            void _volumeCallback(int);

        private:
            void _playerUpdate(const QSharedPointer<qt::TimelinePlayer>&);
            void _widgetUpdate();

            TLRENDER_PRIVATE();
        };
    }
}
