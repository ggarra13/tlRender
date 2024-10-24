// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2024 Darby Johnston
// All rights reserved.

namespace tl
{
    namespace timeline
    {
        inline bool ToneMapOptions::operator == (const ToneMapOptions& other) const
        {
            return
                enabled == other.enabled &&
                hdrData == other.hdrData;
        }

        inline bool ToneMapOptions::operator != (const ToneMapOptions& other) const
        {
            return !(*this == other);
        }
    }
}
