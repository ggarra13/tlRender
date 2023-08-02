// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#pragma once

#include <tlTimeline/CompareOptions.h>

#include <tlCore/Path.h>
#include <tlCore/ListObserver.h>
#include <tlCore/ValueObserver.h>

namespace tl
{
    namespace system
    {
        class Context;
    }

    namespace play
    {
        //! Audio model.
        class AudioModel : public std::enable_shared_from_this<AudioModel>
        {
            TLRENDER_NON_COPYABLE(AudioModel);

        protected:
            void _init(const std::shared_ptr<system::Context>&);

            AudioModel();

        public:
            ~AudioModel();

            //! Create a new model.
            static std::shared_ptr<AudioModel> create(const std::shared_ptr<system::Context>&);

            //! Get the volume.
            float getVolume() const;

            //! Observe the volume.
            std::shared_ptr<observer::IValue<float> > observeVolume() const;

            //! Set the volume.
            void setVolume(float);

            //! Increase the volume.
            void volumeUp();

            //! Decrease the volume.
            void volumeDown();

            //! Get whether the audio is muted.
            bool isMuted() const;

            //! Observe whether the audio is muted.
            std::shared_ptr<observer::IValue<bool> > observeMute() const;

            //! Set whether the audio is muted.
            void setMute(bool);

        private:
            TLRENDER_PRIVATE();
        };
    }
}
