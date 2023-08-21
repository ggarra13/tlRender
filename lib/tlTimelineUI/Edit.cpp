// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <opentimelineio/editAlgorithm.h>

#include <tlTimelineUI/Edit.h>

namespace tl
{
    namespace timelineui
    {
        namespace
        {
            int getIndex(const otio::Composable* composable)
            {
                auto parent = composable->parent();
                return parent->index_of_child(composable);
            }
        }

        otio::SerializableObject::Retainer<otio::Timeline> insert(
            const otio::Timeline* timeline,
            const otio::Composable* composable,
            int trackIndex,
            int insertIndex)
        {
            const int oldIndex = getIndex(composable);
            const int oldTrackIndex = getIndex(composable->parent());

            // Sanity checks
            if (oldIndex == -1 ||
                oldTrackIndex == -1 ||
                insertIndex < 0 ||
                trackIndex < 0 ||
                trackIndex > timeline->tracks()->children().size() ||
                (oldIndex == insertIndex && oldTrackIndex == trackIndex))
                return timeline;

            const auto& tracks = timeline->tracks()->children();

            // We should not make the timeline or composable cont*
            // as we modify the timeline in place.
            // This has the benefit that all pointers do not get invalidated.
            auto constItem = dynamic_cast<const otio::Item*>(composable);
            auto insert_item = const_cast<otio::Item*>(constItem);

            // Original track
            auto track = otio::dynamic_retainer_cast<otio::Track>(tracks[oldTrackIndex]);            

            // Remove item from original track
            track->remove_child(oldIndex);

            // Switch to insert track
            track = otio::dynamic_retainer_cast<otio::Track>(tracks[trackIndex]);
            // Time to insert into insert track
            otime::RationalTime time = time::invalidTime;
            
            const auto& children = track->children();

            // Find insert index's time to insert into
            if (insertIndex < children.size())
            {
                auto item = otio::dynamic_retainer_cast<otio::Item>(
                    children[insertIndex]);
                time = item->trimmed_range_in_parent().value().start_time();
            }

            // If time is not valid, it means we are inserting at the end
            if (!time::isValid(time))
            {
                time = track->trimmed_range().end_time_exclusive();
            }
            
            otio::ErrorStatus errorStatus;
            otio::algo::insert(
                insert_item, track, time, true, nullptr, &errorStatus);
            
            // Note: timeline may have grown in size if the item was moved from
            //       one video track to another.  tlRender needs to take care
            //       of that.
            return timeline;
        }        
    }
}
