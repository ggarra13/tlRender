// SPDX-License-Identifier: BSD-3-Clause
// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlPlayGLApp/MainWindow.h>

#include <tlPlayGLApp/App.h>
#include <tlPlayGLApp/AudioActions.h>
#include <tlPlayGLApp/AudioMenu.h>
#include <tlPlayGLApp/AudioPopup.h>
#include <tlPlayGLApp/CompareActions.h>
#include <tlPlayGLApp/CompareMenu.h>
#include <tlPlayGLApp/CompareToolBar.h>
#include <tlPlayGLApp/FileActions.h>
#include <tlPlayGLApp/FileMenu.h>
#include <tlPlayGLApp/FileToolBar.h>
#include <tlPlayGLApp/FrameActions.h>
#include <tlPlayGLApp/FrameMenu.h>
#include <tlPlayGLApp/PlaybackActions.h>
#include <tlPlayGLApp/PlaybackMenu.h>
#include <tlPlayGLApp/RenderActions.h>
#include <tlPlayGLApp/RenderMenu.h>
#include <tlPlayGLApp/SpeedPopup.h>
#include <tlPlayGLApp/TimelineActions.h>
#include <tlPlayGLApp/TimelineMenu.h>
#include <tlPlayGLApp/ToolsActions.h>
#include <tlPlayGLApp/ToolsMenu.h>
#include <tlPlayGLApp/ToolsToolBar.h>
#include <tlPlayGLApp/ToolsWidget.h>
#include <tlPlayGLApp/ViewActions.h>
#include <tlPlayGLApp/ViewMenu.h>
#include <tlPlayGLApp/ViewToolBar.h>
#include <tlPlayGLApp/WindowActions.h>
#include <tlPlayGLApp/WindowMenu.h>
#include <tlPlayGLApp/WindowToolBar.h>

#include <tlPlay/AudioModel.h>
#include <tlPlay/ColorModel.h>
#include <tlPlay/Info.h>
#include <tlPlay/Settings.h>
#include <tlPlay/ViewportModel.h>

#include <tlTimelineUI/TimelineViewport.h>
#include <tlTimelineUI/TimelineWidget.h>

#include <tlUI/ButtonGroup.h>
#include <tlUI/ComboBox.h>
#include <tlUI/Divider.h>
#include <tlUI/DoubleEdit.h>
#include <tlUI/DoubleModel.h>
#include <tlUI/Label.h>
#include <tlUI/Menu.h>
#include <tlUI/MenuBar.h>
#include <tlUI/RowLayout.h>
#include <tlUI/Spacer.h>
#include <tlUI/Splitter.h>
#include <tlUI/TimeEdit.h>
#include <tlUI/TimeLabel.h>
#include <tlUI/ToolButton.h>

#if defined(TLRENDER_BMD)
#include <tlDevice/BMDOutputDevice.h>
#endif // TLRENDER_BMD

#include <tlTimeline/TimeUnits.h>

#include <tlCore/Timer.h>

namespace tl
{
    namespace play_gl
    {
        bool WindowOptions::operator == (const WindowOptions& other) const
        {
            return
                fileToolBar == other.fileToolBar &&
                compareToolBar == other.compareToolBar &&
                windowToolBar == other.windowToolBar &&
                viewToolBar == other.viewToolBar &&
                toolsToolBar == other.toolsToolBar &&
                timeline == other.timeline &&
                bottomToolBar == other.bottomToolBar &&
                statusToolBar == other.statusToolBar &&
                splitter == other.splitter &&
                splitter2 == other.splitter2;
        }

        bool WindowOptions::operator != (const WindowOptions& other) const
        {
            return !(*this == other);
        }

        struct MainWindow::Private
        {
            std::weak_ptr<App> app;
            std::shared_ptr<play::Settings> settings;
            std::shared_ptr<observer::Value<WindowOptions> > windowOptions;
            std::shared_ptr<timeline::TimeUnitsModel> timeUnitsModel;
            std::shared_ptr<ui::DoubleModel> speedModel;
            timelineui::ItemOptions itemOptions;
            std::vector<std::shared_ptr<timeline::Player> > players;

            std::shared_ptr<timelineui::TimelineViewport> timelineViewport;
            std::shared_ptr<timelineui::TimelineWidget> timelineWidget;
            std::shared_ptr<FileActions> fileActions;
            std::shared_ptr<CompareActions> compareActions;
            std::shared_ptr<WindowActions> windowActions;
            std::shared_ptr<ViewActions> viewActions;
            std::shared_ptr<RenderActions> renderActions;
            std::shared_ptr<PlaybackActions> playbackActions;
            std::shared_ptr<FrameActions> frameActions;
            std::shared_ptr<TimelineActions> timelineActions;
            std::shared_ptr<AudioActions> audioActions;
            std::shared_ptr<ToolsActions> toolsActions;
            std::shared_ptr<FileMenu> fileMenu;
            std::shared_ptr<CompareMenu> compareMenu;
            std::shared_ptr<WindowMenu> windowMenu;
            std::shared_ptr<ViewMenu> viewMenu;
            std::shared_ptr<RenderMenu> renderMenu;
            std::shared_ptr<PlaybackMenu> playbackMenu;
            std::shared_ptr<FrameMenu> frameMenu;
            std::shared_ptr<TimelineMenu> timelineMenu;
            std::shared_ptr<AudioMenu> audioMenu;
            std::shared_ptr<ToolsMenu> toolsMenu;
            std::shared_ptr<ui::MenuBar> menuBar;
            std::shared_ptr<FileToolBar> fileToolBar;
            std::shared_ptr<CompareToolBar> compareToolBar;
            std::shared_ptr<WindowToolBar> windowToolBar;
            std::shared_ptr<ViewToolBar> viewToolBar;
            std::shared_ptr<ToolsToolBar> toolsToolBar;
            std::shared_ptr<ui::ButtonGroup> playbackButtonGroup;
            std::shared_ptr<ui::ButtonGroup> frameButtonGroup;
            std::shared_ptr<ui::TimeEdit> currentTimeEdit;
            std::shared_ptr<ui::TimeLabel> durationLabel;
            std::shared_ptr<ui::ComboBox> timeUnitsComboBox;
            std::shared_ptr<ui::DoubleEdit> speedEdit;
            std::shared_ptr<ui::ToolButton> speedButton;
            std::shared_ptr<SpeedPopup> speedPopup;
            std::shared_ptr<ui::ToolButton> audioButton;
            std::shared_ptr<AudioPopup> audioPopup;
            std::shared_ptr<ui::ToolButton> muteButton;
            std::shared_ptr<ui::Label> statusLabel;
            std::shared_ptr<time::Timer> statusTimer;
            std::shared_ptr<ui::Label> infoLabel;
            std::shared_ptr<ToolsWidget> toolsWidget;
            std::map<std::string, std::shared_ptr<ui::Divider> > dividers;
            std::shared_ptr<ui::Splitter> splitter;
            std::shared_ptr<ui::Splitter> splitter2;
            std::shared_ptr<ui::HorizontalLayout> bottomLayout;
            std::shared_ptr<ui::HorizontalLayout> statusLayout;
            std::shared_ptr<ui::VerticalLayout> layout;

            std::shared_ptr<observer::ListObserver<std::shared_ptr<timeline::Player> > > playersObserver;
            std::shared_ptr<observer::ValueObserver<double> > speedObserver;
            std::shared_ptr<observer::ValueObserver<double> > speedObserver2;
            std::shared_ptr<observer::ValueObserver<timeline::Playback> > playbackObserver;
            std::shared_ptr<observer::ValueObserver<otime::RationalTime> > currentTimeObserver;
            std::shared_ptr<observer::ValueObserver<timeline::BackgroundOptions> > backgroundOptionsObserver;
            std::shared_ptr<observer::ValueObserver<timeline::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<observer::ValueObserver<timeline::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<observer::ValueObserver<timeline::ImageOptions> > imageOptionsObserver;
            std::shared_ptr<observer::ValueObserver<timeline::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<observer::ValueObserver<timeline::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<observer::ValueObserver<bool> > muteObserver;
            std::shared_ptr<observer::ListObserver<log::Item> > logObserver;
        };

        void MainWindow::_init(
            const std::shared_ptr<App>& app,
            const std::shared_ptr<system::Context>& context)
        {
            Window::_init("tlplay-gl", context, nullptr);
            TLRENDER_P();

            setBackgroundRole(ui::ColorRole::Window);

            p.app = app;

            p.settings = app->getSettings();
            p.settings->setDefaultValue("Window/Options", WindowOptions());
            p.settings->setDefaultValue("Timeline/Editable", true);
            p.settings->setDefaultValue("Timeline/EditAssociatedClips",
                timelineui::ItemOptions().editAssociatedClips);
            p.settings->setDefaultValue("Timeline/FrameView", true);
            p.settings->setDefaultValue("Timeline/StopOnScrub", true);
            p.settings->setDefaultValue("Timeline/Thumbnails",
                timelineui::ItemOptions().thumbnails);
            p.settings->setDefaultValue("Timeline/ThumbnailsSize",
                timelineui::ItemOptions().thumbnailHeight);
            p.settings->setDefaultValue("Timeline/Transitions",
                timelineui::ItemOptions().showTransitions);
            p.settings->setDefaultValue("Timeline/Markers",
                timelineui::ItemOptions().showMarkers);

            p.windowOptions = observer::Value<WindowOptions>::create(
                p.settings->getValue<WindowOptions>("Window/Options"));

            p.timeUnitsModel = timeline::TimeUnitsModel::create(context);

            p.speedModel = ui::DoubleModel::create(context);
            p.speedModel->setRange(math::DoubleRange(0.0, 1000000.0));
            p.speedModel->setStep(1.F);
            p.speedModel->setLargeStep(10.F);

            p.timelineViewport = timelineui::TimelineViewport::create(context);

            p.timelineWidget = timelineui::TimelineWidget::create(p.timeUnitsModel, context);
            p.timelineWidget->setEditable(p.settings->getValue<bool>("Timeline/Editable"));
            p.timelineWidget->setFrameView(p.settings->getValue<bool>("Timeline/FrameView"));
            p.timelineWidget->setScrollBarsVisible(false);
            p.timelineWidget->setStopOnScrub(p.settings->getValue<bool>("Timeline/StopOnScrub"));
            timelineui::ItemOptions itemOptions;
            itemOptions.editAssociatedClips = p.settings->getValue<bool>("Timeline/EditAssociatedClips");
            itemOptions.thumbnails = p.settings->getValue<bool>("Timeline/Thumbnails");
            itemOptions.thumbnailHeight = p.settings->getValue<int>("Timeline/ThumbnailsSize");
            itemOptions.showTransitions = p.settings->getValue<bool>("Timeline/Transitions");
            itemOptions.showMarkers = p.settings->getValue<bool>("Timeline/Markers");
            p.timelineWidget->setItemOptions(itemOptions);

            p.fileActions = FileActions::create(app, context);
            p.compareActions = CompareActions::create(app, context);
            p.windowActions = WindowActions::create(
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                app,
                context);
            p.viewActions = ViewActions::create(
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                app,
                context);
            p.renderActions = RenderActions::create(app, context);
            p.playbackActions = PlaybackActions::create(app, context);
            p.frameActions = FrameActions::create(
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                app,
                context);
            p.timelineActions = TimelineActions::create(
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                app,
                context);
            p.audioActions = AudioActions::create(app, context);
            p.toolsActions = ToolsActions::create(app, context);

            p.fileMenu = FileMenu::create(
                p.fileActions->getActions(),
                app,
                context);
            p.compareMenu = CompareMenu::create(
                p.compareActions->getActions(),
                app,
                context);
            p.windowMenu = WindowMenu::create(
                p.windowActions->getActions(),
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                app,
                context);
            p.viewMenu = ViewMenu::create(
                p.viewActions->getActions(),
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                app,
                context);
            p.renderMenu = RenderMenu::create(
                p.renderActions->getActions(),
                app,
                context);
            p.playbackMenu = PlaybackMenu::create(
                p.playbackActions->getActions(),
                app,
                context);
            p.frameMenu = FrameMenu::create(
                p.frameActions->getActions(),
                app,
                context);
            p.timelineMenu = TimelineMenu::create(
                p.timelineActions->getActions(),
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                app,
                context);
            p.audioMenu = AudioMenu::create(
                p.audioActions->getActions(),
                app,
                context);
            p.toolsMenu = ToolsMenu::create(
                p.toolsActions->getActions(),
                app,
                context);
            p.menuBar = ui::MenuBar::create(context);
            p.menuBar->addMenu("File", p.fileMenu);
            p.menuBar->addMenu("Compare", p.compareMenu);
            p.menuBar->addMenu("Window", p.windowMenu);
            p.menuBar->addMenu("View", p.viewMenu);
            p.menuBar->addMenu("Render", p.renderMenu);
            p.menuBar->addMenu("Playback", p.playbackMenu);
            p.menuBar->addMenu("Frame", p.frameMenu);
            p.menuBar->addMenu("Timeline", p.timelineMenu);
            p.menuBar->addMenu("Audio", p.audioMenu);
            p.menuBar->addMenu("Tools", p.toolsMenu);

            p.fileToolBar = FileToolBar::create(
                p.fileActions->getActions(),
                app,
                context);
            p.compareToolBar = CompareToolBar::create(
                p.compareActions->getActions(),
                app,
                context);
            p.windowToolBar = WindowToolBar::create(
                p.windowActions->getActions(),
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                app,
                context);
            p.viewToolBar = ViewToolBar::create(
                p.viewActions->getActions(),
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                app,
                context);
            p.toolsToolBar = ToolsToolBar::create(
                p.toolsActions->getActions(),
                app,
                context);

            auto playbackActions = p.playbackActions->getActions();
            auto stopButton = ui::ToolButton::create(context);
            stopButton->setIcon(playbackActions["Stop"]->icon);
            stopButton->setToolTip(playbackActions["Stop"]->toolTip);
            auto forwardButton = ui::ToolButton::create(context);
            forwardButton->setIcon(playbackActions["Forward"]->icon);
            forwardButton->setToolTip(playbackActions["Forward"]->toolTip);
            auto reverseButton = ui::ToolButton::create(context);
            reverseButton->setIcon(playbackActions["Reverse"]->icon);
            reverseButton->setToolTip(playbackActions["Reverse"]->toolTip);
            p.playbackButtonGroup = ui::ButtonGroup::create(ui::ButtonGroupType::Radio, context);
            p.playbackButtonGroup->addButton(stopButton);
            p.playbackButtonGroup->addButton(forwardButton);
            p.playbackButtonGroup->addButton(reverseButton);

            auto frameActions = p.frameActions->getActions();
            auto timeStartButton = ui::ToolButton::create(context);
            timeStartButton->setIcon(frameActions["Start"]->icon);
            timeStartButton->setToolTip(frameActions["Start"]->toolTip);
            auto timeEndButton = ui::ToolButton::create(context);
            timeEndButton->setIcon(frameActions["End"]->icon);
            timeEndButton->setToolTip(frameActions["End"]->toolTip);
            auto framePrevButton = ui::ToolButton::create(context);
            framePrevButton->setIcon(frameActions["Prev"]->icon);
            framePrevButton->setToolTip(frameActions["Prev"]->toolTip);
            framePrevButton->setRepeatClick(true);
            auto frameNextButton = ui::ToolButton::create(context);
            frameNextButton->setIcon(frameActions["Next"]->icon);
            frameNextButton->setToolTip(frameActions["Next"]->toolTip);
            frameNextButton->setRepeatClick(true);
            p.frameButtonGroup = ui::ButtonGroup::create(ui::ButtonGroupType::Click, context);
            p.frameButtonGroup->addButton(timeStartButton);
            p.frameButtonGroup->addButton(framePrevButton);
            p.frameButtonGroup->addButton(frameNextButton);
            p.frameButtonGroup->addButton(timeEndButton);

            p.currentTimeEdit = ui::TimeEdit::create(p.timeUnitsModel, context);
            p.currentTimeEdit->setToolTip("Current time");

            p.durationLabel = ui::TimeLabel::create(p.timeUnitsModel, context);
            p.durationLabel->setFontRole(ui::FontRole::Mono);
            p.durationLabel->setMarginRole(ui::SizeRole::MarginInside);
            p.durationLabel->setToolTip("Duration");

            p.timeUnitsComboBox = ui::ComboBox::create(context);
            p.timeUnitsComboBox->setItems(timeline::getTimeUnitsLabels());
            p.timeUnitsComboBox->setCurrentIndex(
                static_cast<int>(p.timeUnitsModel->getTimeUnits()));
            p.timeUnitsComboBox->setToolTip("Time units");

            p.speedEdit = ui::DoubleEdit::create(context, p.speedModel);
            p.speedEdit->setToolTip("Current speed");
            p.speedButton = ui::ToolButton::create("FPS", context);
            p.speedButton->setIcon("MenuArrow");
            p.speedButton->setToolTip("Speed menu");

            p.audioButton = ui::ToolButton::create(context);
            p.audioButton->setIcon("Volume");
            p.audioButton->setToolTip("Audio settings");
            p.muteButton = ui::ToolButton::create(context);
            p.muteButton->setCheckable(true);
            p.muteButton->setIcon("Mute");
            p.muteButton->setToolTip("Mute the audio");

            p.statusLabel = ui::Label::create(context);
            p.statusLabel->setHStretch(ui::Stretch::Expanding);
            p.statusLabel->setMarginRole(ui::SizeRole::MarginInside);
            p.statusTimer = time::Timer::create(context);

            p.infoLabel = ui::Label::create(context);
            p.infoLabel->setHAlign(ui::HAlign::Right);
            p.infoLabel->setMarginRole(ui::SizeRole::MarginInside);

            p.toolsWidget = ToolsWidget::create(app, context);
            p.toolsWidget->hide();

            p.layout = ui::VerticalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ui::SizeRole::None);
            p.menuBar->setParent(p.layout);
            p.dividers["MenuBar"] = ui::Divider::create(ui::Orientation::Vertical, context, p.layout);
            auto hLayout = ui::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ui::SizeRole::None);
            p.fileToolBar->setParent(hLayout);
            p.dividers["File"] = ui::Divider::create(ui::Orientation::Horizontal, context, hLayout);
            p.compareToolBar->setParent(hLayout);
            p.dividers["Compare"] = ui::Divider::create(ui::Orientation::Horizontal, context, hLayout);
            p.windowToolBar->setParent(hLayout);
            p.dividers["Window"] = ui::Divider::create(ui::Orientation::Horizontal, context, hLayout);
            p.viewToolBar->setParent(hLayout);
            p.dividers["View"] = ui::Divider::create(ui::Orientation::Horizontal, context, hLayout);
            p.toolsToolBar->setParent(hLayout);
            p.dividers["ToolBar"] = ui::Divider::create(ui::Orientation::Vertical, context, p.layout);
            p.splitter = ui::Splitter::create(ui::Orientation::Vertical, context, p.layout);
            p.splitter->setSpacingRole(ui::SizeRole::None);
            p.splitter2 = ui::Splitter::create(ui::Orientation::Horizontal, context, p.splitter);
            p.splitter2->setSpacingRole(ui::SizeRole::None);
            p.timelineViewport->setParent(p.splitter2);
            p.toolsWidget->setParent(p.splitter2);
            p.timelineWidget->setParent(p.splitter);
            p.dividers["Bottom"] = ui::Divider::create(ui::Orientation::Vertical, context, p.layout);
            p.bottomLayout = ui::HorizontalLayout::create(context, p.layout);
            p.bottomLayout->setMarginRole(ui::SizeRole::MarginInside);
            p.bottomLayout->setSpacingRole(ui::SizeRole::SpacingSmall);
            hLayout = ui::HorizontalLayout::create(context, p.bottomLayout);
            hLayout->setSpacingRole(ui::SizeRole::None);
            reverseButton->setParent(hLayout);
            stopButton->setParent(hLayout);
            forwardButton->setParent(hLayout);
            timeStartButton->setParent(hLayout);
            framePrevButton->setParent(hLayout);
            frameNextButton->setParent(hLayout);
            timeEndButton->setParent(hLayout);
            p.currentTimeEdit->setParent(p.bottomLayout);
            p.durationLabel->setParent(p.bottomLayout);
            p.timeUnitsComboBox->setParent(p.bottomLayout);
            hLayout = ui::HorizontalLayout::create(context, p.bottomLayout);
            hLayout->setSpacingRole(ui::SizeRole::SpacingTool);
            p.speedEdit->setParent(hLayout);
            p.speedButton->setParent(hLayout);
            auto spacer = ui::Spacer::create(ui::Orientation::Horizontal, context);
            spacer->setHStretch(ui::Stretch::Expanding);
            spacer->setParent(p.bottomLayout);
            p.audioButton->setParent(p.bottomLayout);
            p.muteButton->setParent(p.bottomLayout);
            p.dividers["Status"] = ui::Divider::create(ui::Orientation::Vertical, context, p.layout);
            p.statusLayout = ui::HorizontalLayout::create(context, p.layout);
            p.statusLayout->setSpacingRole(ui::SizeRole::None);
            p.statusLabel->setParent(p.statusLayout);
            ui::Divider::create(ui::Orientation::Horizontal, context, p.statusLayout);
            p.infoLabel->setParent(p.statusLayout);

            _windowOptionsUpdate();
            _infoUpdate();

            auto appWeak = std::weak_ptr<App>(app);
            p.timelineViewport->setCompareCallback(
                [appWeak](const timeline::CompareOptions& value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getFilesModel()->setCompareOptions(value);
                    }
                });
            p.timelineViewport->setViewPosAndZoomCallback(
                [this](const math::Vector2i& pos, double zoom)
                {
                    _devicesViewUpdate(
                        pos,
                        zoom,
                        _p->timelineViewport->hasFrameView());
                });
            p.timelineViewport->setFrameViewCallback(
                [this](bool value)
                {
                    _devicesViewUpdate(
                        _p->timelineViewport->getViewPos(),
                        _p->timelineViewport->getViewZoom(),
                        value);
                });

            p.currentTimeEdit->setCallback(
                [this](const otime::RationalTime& value)
                {
                    if (!_p->players.empty() && _p->players[0])
                    {
                        _p->players[0]->setPlayback(timeline::Playback::Stop);
                        _p->players[0]->seek(value);
                        _p->currentTimeEdit->setValue(_p->players[0]->getCurrentTime());
                    }
                });

            p.timeUnitsComboBox->setIndexCallback(
                [this](int value)
                {
                    _p->timeUnitsModel->setTimeUnits(
                        static_cast<timeline::TimeUnits>(value));
                });

            p.playbackButtonGroup->setCheckedCallback(
                [this](int index, bool value)
                {
                    if (!_p->players.empty() && _p->players[0])
                    {
                        _p->players[0]->setPlayback(static_cast<timeline::Playback>(index));
                    }
                });

            p.frameButtonGroup->setClickedCallback(
                [this](int index)
                {
                    if (!_p->players.empty() && _p->players[0])
                    {
                        switch (index)
                        {
                        case 0:
                            _p->players[0]->timeAction(timeline::TimeAction::Start);
                            break;
                        case 1:
                            _p->players[0]->timeAction(timeline::TimeAction::FramePrev);
                            break;
                        case 2:
                            _p->players[0]->timeAction(timeline::TimeAction::FrameNext);
                            break;
                        case 3:
                            _p->players[0]->timeAction(timeline::TimeAction::End);
                            break;
                        }
                    }
                });

            p.speedButton->setPressedCallback(
                [this]
                {
                    _showSpeedPopup();
                });

            p.audioButton->setPressedCallback(
                [this]
                {
                    _showAudioPopup();
                });
            p.muteButton->setCheckedCallback(
                [appWeak](bool value)
                {
                    if (auto app = appWeak.lock())
                    {
                        app->getAudioModel()->setMute(value);
                    }
                });

            p.playersObserver = observer::ListObserver<std::shared_ptr<timeline::Player> >::create(
                app->observeActivePlayers(),
                [this](const std::vector<std::shared_ptr<timeline::Player> >& value)
                {
                    _playersUpdate(value);
                });

            p.speedObserver2 = observer::ValueObserver<double>::create(
                p.speedModel->observeValue(),
                [this](double value)
                {
                    if (!_p->players.empty() && _p->players[0])
                    {
                        _p->players[0]->setSpeed(value);
                    }
                });

            p.backgroundOptionsObserver = observer::ValueObserver<timeline::BackgroundOptions>::create(
                app->getViewportModel()->observeBackgroundOptions(),
                [this](const timeline::BackgroundOptions& value)
                {
                    _p->timelineViewport->setBackgroundOptions(value);
                });

            p.ocioOptionsObserver = observer::ValueObserver<timeline::OCIOOptions>::create(
                app->getColorModel()->observeOCIOOptions(),
                [this](const timeline::OCIOOptions& value)
                {
                    _p->timelineViewport->setOCIOOptions(value);
                });

            p.lutOptionsObserver = observer::ValueObserver<timeline::LUTOptions>::create(
                app->getColorModel()->observeLUTOptions(),
                [this](const timeline::LUTOptions& value)
                {
                    _p->timelineViewport->setLUTOptions(value);
                });

            p.imageOptionsObserver = observer::ValueObserver<timeline::ImageOptions>::create(
                app->getColorModel()->observeImageOptions(),
                [this](const timeline::ImageOptions& value)
                {
                    std::vector<timeline::ImageOptions> imageOptions;
                    for (const auto& player : _p->players)
                    {
                        imageOptions.push_back(value);
                    }
                    _p->timelineViewport->setImageOptions(imageOptions);
                });

            p.displayOptionsObserver = observer::ValueObserver<timeline::DisplayOptions>::create(
                app->getColorModel()->observeDisplayOptions(),
                [this](const timeline::DisplayOptions& value)
                {
                    std::vector<timeline::DisplayOptions> displayOptions;
                    for (const auto& player : _p->players)
                    {
                        displayOptions.push_back(value);
                    }
                    _p->timelineViewport->setDisplayOptions(displayOptions);
                });

            p.compareOptionsObserver = observer::ValueObserver<timeline::CompareOptions>::create(
                app->getFilesModel()->observeCompareOptions(),
                [this](const timeline::CompareOptions& value)
                {
                    _p->timelineViewport->setCompareOptions(value);
                });

            p.muteObserver = observer::ValueObserver<bool>::create(
                app->getAudioModel()->observeMute(),
                [this](bool value)
                {
                    _p->muteButton->setChecked(value);
                });

            p.logObserver = observer::ListObserver<log::Item>::create(
                context->getLogSystem()->observeLog(),
                [this](const std::vector<log::Item>& value)
                {
                    _statusUpdate(value);
                });
        }

        MainWindow::MainWindow() :
            _p(new Private)
        {}

        MainWindow::~MainWindow()
        {
            TLRENDER_P();
            math::Size2i windowSize = _geometry.getSize();
#if defined(__APPLE__)
            //! \bug The window size needs to be scaled on macOS?
            windowSize = windowSize / _displayScale;
#endif // __APPLE__
            p.settings->setValue("Window/Size", windowSize);
            p.settings->setValue("Window/Options", p.windowOptions->get());
            p.settings->setValue("Timeline/Editable",
                p.timelineWidget->isEditable());
            const auto& timelineItemOptions = p.timelineWidget->getItemOptions();
            p.settings->setValue("Timeline/EditAssociatedClips",
                timelineItemOptions.editAssociatedClips);
            p.settings->setValue("Timeline/FrameView",
                p.timelineWidget->hasFrameView());
            p.settings->setValue("Timeline/StopOnScrub",
                p.timelineWidget->hasStopOnScrub());
            p.settings->setValue("Timeline/Thumbnails",
                timelineItemOptions.thumbnails);
            p.settings->setValue("Timeline/ThumbnailsSize",
                timelineItemOptions.thumbnailHeight);
            p.settings->setValue("Timeline/Transitions",
                timelineItemOptions.showTransitions);
            p.settings->setValue("Timeline/Markers",
                timelineItemOptions.showMarkers);
            _makeCurrent();
            p.timelineViewport->setParent(nullptr);
            p.timelineWidget->setParent(nullptr);
        }

        std::shared_ptr<MainWindow> MainWindow::create(
            const std::shared_ptr<App>& app,
            const std::shared_ptr<system::Context>& context)
        {
            auto out = std::shared_ptr<MainWindow>(new MainWindow);
            out->_init(app, context);
            return out;
        }

        const std::shared_ptr<timelineui::TimelineViewport>& MainWindow::getTimelineViewport() const
        {
            return _p->timelineViewport;
        }

        const std::shared_ptr<timelineui::TimelineWidget>& MainWindow::getTimelineWidget() const
        {
            return _p->timelineWidget;
        }

        void MainWindow::focusCurrentFrame()
        {
            _p->currentTimeEdit->takeKeyFocus();
        }

        const WindowOptions& MainWindow::getWindowOptions() const
        {
            return _p->windowOptions->get();
        }

        std::shared_ptr<observer::IValue<WindowOptions> > MainWindow::observeWindowOptions() const
        {
            return _p->windowOptions;
        }

        void MainWindow::setWindowOptions(const WindowOptions& value)
        {
            if (_p->windowOptions->setIfChanged(value))
            {
                _windowOptionsUpdate();
            }
        }

        void MainWindow::setGeometry(const math::Box2i& value)
        {
            Window::setGeometry(value);
            _p->layout->setGeometry(value);
        }

        void MainWindow::keyPressEvent(ui::KeyEvent& event)
        {
            TLRENDER_P();
            event.accept = p.menuBar->shortcut(event.key, event.modifiers);
        }

        void MainWindow::keyReleaseEvent(ui::KeyEvent& event)
        {
            event.accept = true;
        }

        void MainWindow::_drop(const std::vector<std::string>& value)
        {
            TLRENDER_P();
            if (auto app = p.app.lock())
            {
                for (const auto& i : value)
                {
                    app->open(file::Path(i));
                }
            }
        }

        void MainWindow::_playersUpdate(const std::vector<std::shared_ptr<timeline::Player> >& value)
        {
            TLRENDER_P();

            p.speedObserver.reset();
            p.playbackObserver.reset();
            p.currentTimeObserver.reset();

            p.players = value;

            p.timelineViewport->setPlayers(p.players);
            p.timelineWidget->setPlayer(
                !p.players.empty() ?
                p.players[0] :
                nullptr);
            p.durationLabel->setValue(
                (!p.players.empty() && p.players[0]) ?
                p.players[0]->getTimeRange().duration() :
                time::invalidTime);
            _infoUpdate();

            if (!p.players.empty() && p.players[0])
            {
                p.speedObserver = observer::ValueObserver<double>::create(
                    p.players[0]->observeSpeed(),
                    [this](double value)
                    {
                        _p->speedModel->setValue(value);
                    });

                p.playbackObserver = observer::ValueObserver<timeline::Playback>::create(
                    p.players[0]->observePlayback(),
                    [this](timeline::Playback value)
                    {
                        _p->playbackButtonGroup->setChecked(static_cast<int>(value), true);
                    });

                p.currentTimeObserver = observer::ValueObserver<otime::RationalTime>::create(
                    p.players[0]->observeCurrentTime(),
                    [this](const otime::RationalTime& value)
                    {
                        _p->currentTimeEdit->setValue(value);
                    });
            }
            else
            {
                p.speedModel->setValue(0.0);
                p.playbackButtonGroup->setChecked(0, true);
                p.currentTimeEdit->setValue(time::invalidTime);
            }
        }

        void MainWindow::_showSpeedPopup()
        {
            TLRENDER_P();
            if (auto context = _context.lock())
            {
                if (auto window = std::dynamic_pointer_cast<IWindow>(shared_from_this()))
                {
                    if (!p.speedPopup)
                    {
                        const double defaultSpeed =
                            !p.players.empty() && p.players[0] ?
                            p.players[0]->getDefaultSpeed() :
                            0.0;
                        p.speedPopup = SpeedPopup::create(defaultSpeed, context);
                        p.speedPopup->open(window, p.speedButton->getGeometry());
                        auto weak = std::weak_ptr<MainWindow>(std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
                        p.speedPopup->setCallback(
                            [weak](double value)
                            {
                                if (auto widget = weak.lock())
                                {
                                    if (!widget->_p->players.empty() &&
                                        widget->_p->players[0])
                                    {
                                        widget->_p->players[0]->setSpeed(value);
                                    }
                                    widget->_p->speedPopup->close();
                                }
                            });
                        p.speedPopup->setCloseCallback(
                            [weak]
                            {
                                if (auto widget = weak.lock())
                                {
                                    widget->_p->speedPopup.reset();
                                }
                            });
                    }
                    else
                    {
                        p.speedPopup->close();
                        p.speedPopup.reset();
                    }
                }
            }
        }

        void MainWindow::_showAudioPopup()
        {
            TLRENDER_P();
            if (auto context = _context.lock())
            {
                if (auto app = p.app.lock())
                {
                    if (auto window = std::dynamic_pointer_cast<IWindow>(shared_from_this()))
                    {
                        if (!p.audioPopup)
                        {
                            p.audioPopup = AudioPopup::create(app, context);
                            p.audioPopup->open(window, p.audioButton->getGeometry());
                            auto weak = std::weak_ptr<MainWindow>(std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
                            p.audioPopup->setCloseCallback(
                                [weak]
                                {
                                    if (auto widget = weak.lock())
                                    {
                                        widget->_p->audioPopup.reset();
                                    }
                                });
                        }
                        else
                        {
                            p.audioPopup->close();
                            p.audioPopup.reset();
                        }
                    }
                }
            }
        }

        void MainWindow::_windowOptionsUpdate()
        {
            TLRENDER_P();
            const auto& windowOptions = p.windowOptions->get();

            p.fileToolBar->setVisible(windowOptions.fileToolBar);
            p.dividers["File"]->setVisible(windowOptions.fileToolBar);

            p.compareToolBar->setVisible(windowOptions.compareToolBar);
            p.dividers["Compare"]->setVisible(windowOptions.compareToolBar);

            p.windowToolBar->setVisible(windowOptions.windowToolBar);
            p.dividers["Window"]->setVisible(windowOptions.windowToolBar);

            p.viewToolBar->setVisible(windowOptions.viewToolBar);
            p.dividers["View"]->setVisible(windowOptions.viewToolBar);

            p.toolsToolBar->setVisible(windowOptions.toolsToolBar);

            p.dividers["ToolBar"]->setVisible(
                windowOptions.fileToolBar ||
                windowOptions.compareToolBar ||
                windowOptions.windowToolBar ||
                windowOptions.viewToolBar ||
                windowOptions.toolsToolBar);

            p.timelineWidget->setVisible(windowOptions.timeline);

            p.bottomLayout->setVisible(windowOptions.bottomToolBar);
            p.dividers["Bottom"]->setVisible(windowOptions.bottomToolBar);

            p.statusLayout->setVisible(windowOptions.statusToolBar);
            p.dividers["Status"]->setVisible(windowOptions.statusToolBar);

            p.splitter->setSplit(windowOptions.splitter);
            p.splitter2->setSplit(windowOptions.splitter2);
        }

        void MainWindow::_statusUpdate(const std::vector<log::Item>& value)
        {
            TLRENDER_P();
            for (const auto& i : value)
            {
                switch (i.type)
                {
                case log::Type::Error:
                    p.statusLabel->setText(log::toString(i));
                    p.statusTimer->start(
                        std::chrono::seconds(5),
                        [this]
                        {
                            _p->statusLabel->setText(std::string());
                        });
                        break;
                default: break;
                }
            }
        }

        void MainWindow::_infoUpdate()
        {
            TLRENDER_P();
            std::string text;
            std::string toolTip;
            if (!p.players.empty() && p.players[0])
            {
                const file::Path& path = p.players[0]->getPath();
                const io::Info& info = p.players[0]->getIOInfo();
                text = play::infoLabel(path, info);
                toolTip = play::infoToolTip(path, info);
            }
            p.infoLabel->setText(text);
            p.infoLabel->setToolTip(toolTip);
        }

        void MainWindow::_devicesViewUpdate(const math::Vector2i& pos, double zoom, bool frame)
        {
            TLRENDER_P();
#if defined(TLRENDER_BMD)
            if (auto app = p.app.lock())
            {
                const math::Box2i& g = _p->timelineViewport->getGeometry();
                auto bmdOutputDevice = app->getBMDOutputDevice();
                const math::Size2i& bmdSize = bmdOutputDevice->getSize();
                math::Vector2i bmdPos;
                double bmdZoom = 1.0;
                if (g.isValid() && bmdSize.isValid())
                {
                    bmdPos.x = pos.x / static_cast<float>(g.w()) * bmdSize.w;
                    bmdPos.y = pos.y / static_cast<float>(g.h()) * bmdSize.h;
                    bmdZoom = zoom / static_cast<double>(g.w()) * bmdSize.w;
                }
                bmdOutputDevice->setView(
                    bmdPos,
                    bmdZoom,
                    _p->timelineViewport->hasFrameView());
            }
#endif // TLRENDER_BMD
        }

        void to_json(nlohmann::json& json, const WindowOptions& in)
        {
            json = nlohmann::json
            {
                { "fileToolBar", in.fileToolBar },
                { "compareToolBar", in.compareToolBar },
                { "windowToolBar", in.windowToolBar },
                { "viewToolBar", in.viewToolBar },
                { "toolsToolBar", in.toolsToolBar },
                { "timeline", in.timeline },
                { "bottomToolBar", in.bottomToolBar },
                { "statusToolBar", in.statusToolBar },
                { "splitter", in.splitter },
                { "splitter2", in.splitter2 }
            };
        }

        void from_json(const nlohmann::json& json, WindowOptions& out)
        {
            json.at("fileToolBar").get_to(out.fileToolBar);
            json.at("compareToolBar").get_to(out.compareToolBar);
            json.at("windowToolBar").get_to(out.windowToolBar);
            json.at("viewToolBar").get_to(out.viewToolBar);
            json.at("toolsToolBar").get_to(out.toolsToolBar);
            json.at("timeline").get_to(out.timeline);
            json.at("bottomToolBar").get_to(out.bottomToolBar);
            json.at("statusToolBar").get_to(out.statusToolBar);
            json.at("splitter").get_to(out.splitter);
            json.at("splitter2").get_to(out.splitter2);
        }
    }
}
