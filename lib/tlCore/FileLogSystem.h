// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlCore/LogSystem.h>

namespace tl
{
    namespace file
    {
        //! File log system.
        class FileLogSystem : public system::ICoreSystem
        {
            TLRENDER_NON_COPYABLE(FileLogSystem);

        protected:
            void _init(
                const std::string& fileName,
                const std::shared_ptr<system::Context>&);

            FileLogSystem();

        public:
            virtual ~FileLogSystem();

            //! Create a new system.
            static std::shared_ptr<FileLogSystem> create(
                const std::string& fileName,
                const std::shared_ptr<system::Context>&);

        private:
            TLRENDER_PRIVATE();
        };
    }
}
