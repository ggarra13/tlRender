// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

#define LOG_INFO(x)                                                 \
    if (auto logSystem = _logSystem.lock())                         \
    {                                                               \
        logSystem->print("tl::io::ffmpeg::Plugin", x);              \
    }
#define LOG_ERROR(x)                                                \
    if (auto logSystem = _logSystem.lock())                         \
    {                                                               \
        logSystem->print("tl::io::ffmpeg::Plugin", x,               \
                         log::Type::Error);                         \
    }
#define LOG_WARNING(x)                                              \
    if (auto logSystem = _logSystem.lock())                         \
    {                                                               \
        logSystem->print("tl::io::ffmpeg::Plugin", x,               \
                         log::Type::Warning);                       \
    }
#define LOG_STATUS(x)                                               \
    if (auto logSystem = _logSystem.lock())                         \
    {                                                               \
        logSystem->print("tl::io::ffmpeg::Plugin", x,               \
                         log::Type::Status);                        \
    }
