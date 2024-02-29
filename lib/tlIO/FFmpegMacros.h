// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#define LOG_INFO(x, module)                                             \
    if (auto logSystem = _logSystem.lock())                             \
    {                                                                   \
    logSystem->print("tl::io::ffmpeg::Plugin", x,                       \
                     log::Type::Message, module);                       \
    }
#define LOG_ERROR(x, module)                                        \
    if (auto logSystem = _logSystem.lock())                         \
    {                                                               \
        logSystem->print("tl::io::ffmpeg::Plugin", x,               \
                         log::Type::Error, module);                 \
    }
#define LOG_WARNING(x, module)                                      \
    if (auto logSystem = _logSystem.lock())                         \
    {                                                               \
        logSystem->print("tl::io::ffmpeg::Plugin", x,               \
                         log::Type::Warning, module);               \
    }
#define LOG_STATUS(x, module)                                       \
    if (auto logSystem = _logSystem.lock())                         \
    {                                                               \
        logSystem->print("tl::io::ffmpeg::Plugin", x,               \
                         log::Type::Status, module);                \
    }
