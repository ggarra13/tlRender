// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021 Darby Johnston
// All rights reserved.

#pragma once

#include <tlrTestLib/ITest.h>

namespace tlr
{
    namespace CoreTest
    {
        class JPEGTest : public Test::ITest
        {
        protected:
            JPEGTest();

        public:
            static std::shared_ptr<JPEGTest> create();

            void run() override;
        };
    }
}
