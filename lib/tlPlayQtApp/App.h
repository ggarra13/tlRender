// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlApp/IApp.h>

#include <tlTimeline/TimeUnits.h>
#include <tlTimeline/IRender.h>

#include <tlIO/IO.h>

#include <QApplication>
#include <QSharedPointer>
#include <QVector>

namespace tl
{
    namespace play
    {
        struct FilesModelItem;

        class AudioModel;
        class ColorModel;
        class FilesModel;
        class Settings;
        class ViewportModel;
    }

    namespace qt
    {
        class OutputDevice;
        class TimeObject;
        class TimelinePlayer;
    }

    namespace ui
    {
        class RecentFilesModel;
    }

    //! "tlplay-qt" application.
    namespace play_qt
    {
        class DevicesModel;
        class FilesAModel;
        class FilesBModel;
        class MainWindow;

        //! Application.
        class App : public QApplication, public app::IApp
        {
            Q_OBJECT

        public:
            App(
                int& argc,
                char** argv,
                const std::shared_ptr<system::Context>&);

            virtual ~App();

            //! Get the time units model.
            const std::shared_ptr<timeline::TimeUnitsModel>& timeUnitsModel() const;

            //! Get the time object.
            qt::TimeObject* timeObject() const;

            //! Get the settings.
            const std::shared_ptr<play::Settings>& settings() const;

            //! Get the files model.
            const std::shared_ptr<play::FilesModel>& filesModel() const;

            //! Get the timeline players.
            const QVector<QSharedPointer<qt::TimelinePlayer> >& players() const;

            //! Get the recent files model.
            const std::shared_ptr<ui::RecentFilesModel>& recentFilesModel() const;

            //! Get the viewport model.
            const std::shared_ptr<play::ViewportModel>& viewportModel() const;

            //! Get the color model.
            const std::shared_ptr<play::ColorModel>& colorModel() const;

            //! Get the output device.
            qt::OutputDevice* outputDevice() const;

            //! Get the devices model.
            const std::shared_ptr<DevicesModel>& devicesModel() const;

            //! Get the audio model.
            const std::shared_ptr<play::AudioModel>& audioModel() const;

            //! Get the main window.
            MainWindow* mainWindow() const;

        public Q_SLOTS:
            //! Open a file.
            void open(const QString&, const QString& = QString());

            //! Open a file dialog.
            void openDialog();

            //! Open a file with separate audio dialog.
            void openSeparateAudioDialog();

        Q_SIGNALS:
            //! This signal is emitted when the active players are changed.
            void activePlayersChanged(const QVector<QSharedPointer<qt::TimelinePlayer> >&);

        private Q_SLOTS:
            void _filesCallback(const std::vector<std::shared_ptr<tl::play::FilesModelItem> >&);
            void _activeCallback(const std::vector<std::shared_ptr<tl::play::FilesModelItem> >&);

        private:
            void _fileLogInit(const std::string&);
            void _settingsInit(const std::string&);
            void _modelsInit();
            void _observersInit();
            void _inputFilesInit();
            void _mainWindowInit();

            io::Options _ioOptions() const;
            QVector<QSharedPointer<qt::TimelinePlayer> > _activePlayers() const;
            otime::RationalTime _cacheReadAhead() const;
            otime::RationalTime _cacheReadBehind() const;

            void _settingsUpdate(const std::string&);
            void _cacheUpdate();
            void _audioUpdate();

            TLRENDER_PRIVATE();
        };
    }
}
