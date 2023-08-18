// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2023 Darby Johnston
// All rights reserved.

#include <tlTimelineUI/Edit.h>

namespace tl
{
    namespace timelineui
    {
        namespace
        {
            int getIndex(const otio::Composable* composable)
            {
                int out = -1;
                if (composable && composable->parent())
                {
                    const auto& children = composable->parent()->children();
                    for (int i = 0; i < children.size(); ++i)
                    {
                        if (composable == children[i].value)
                        {
                            out = i;
                            break;
                        }
                    }
                }
                return out;
            }
        }

        otio::SerializableObject::Retainer<otio::Timeline> insert(
            const otio::Timeline* timeline,
            const otio::Composable* composable,
            int trackIndex,
            int insertIndex)
        {
            const std::string s = timeline->to_json_string();
            otio::SerializableObject::Retainer<otio::Timeline> out(
                dynamic_cast<otio::Timeline*>(otio::Timeline::from_json_string(s)));

            const int oldIndex = getIndex(composable);
            const int oldTrackIndex = getIndex(composable->parent());
            if (oldIndex != -1 &&
                oldTrackIndex != -1 &&
                trackIndex >= 0 &&
                trackIndex < out->tracks()->children().size())
            {
                if (oldTrackIndex == trackIndex && oldIndex < insertIndex)
                {
                    --insertIndex;
                }
                if (auto track = otio::dynamic_retainer_cast<otio::Track>(
                    out->tracks()->children()[oldTrackIndex]))
                {
                    auto child = track->children()[oldIndex];
                    track->remove_child(oldIndex);

                    if (auto track = otio::dynamic_retainer_cast<otio::Track>(
                        out->tracks()->children()[trackIndex]))
                    {
                        track->insert_child(insertIndex, child);
                    }
                }
            }

            return out;
        }

        
        otio::SerializableObject::Retainer<otio::Timeline> slice(
            const otio::Timeline* timeline, const otio::Item* item,
            const otime::RationalTime& time)
        {
            const std::string s = timeline->to_json_string();
            otio::SerializableObject::Retainer<otio::Timeline> out(
                dynamic_cast<otio::Timeline*>(
                    otio::Timeline::from_json_string(s)));

            const auto tracks = out->tracks()->children();
            const int itemTrackIndex = getIndex(item->parent());
            const int itemIndex = getIndex(item);
            if (itemTrackIndex >= 0 && itemTrackIndex < tracks.size())
            {
                if (auto track = dynamic_cast<otio::Track*>(
                        tracks[itemTrackIndex].value))
                {
                    const auto children = track->children();
                    if (itemIndex >= 0 && itemIndex < children.size())
                    {
                        // Get item range
                        const otime::TimeRange range =
                            track->trimmed_range_of_child_at_index(itemIndex);

                        if (!range.contains(time))
                            return out;

                        // Calculate the first and second range for each slice.
                        const otime::TimeRange first_source_range(
                            item->trimmed_range().start_time(),
                            time - range.start_time());
                        const otime::TimeRange second_source_range(
                            first_source_range.start_time() +
                                first_source_range.duration(),
                            range.duration() - first_source_range.duration());

                        // Clone the original item.
                        auto first_item =
                            dynamic_cast<otio::Item*>(item->clone());
                        auto second_item =
                            dynamic_cast<otio::Item*>(item->clone());

                        // Remove the original child
                        track->remove_child(itemIndex);

                        // Insert the two items
                        first_item->set_source_range(first_source_range);
                        track->insert_child(itemIndex, first_item);
                        second_item->set_source_range(second_source_range);
                        track->insert_child(itemIndex + 1, second_item);
                    }
                }
            }
            return out;
        }

        otio::SerializableObject::Retainer<otio::Timeline>
        remove(const otio::Timeline* timeline, const otio::Item* item)
        {
            const std::string s = timeline->to_json_string();
            otio::SerializableObject::Retainer<otio::Timeline> out(
                dynamic_cast<otio::Timeline*>(
                    otio::Timeline::from_json_string(s)));

            const auto tracks = out->tracks()->children();
            const int itemTrackIndex = getIndex(item->parent());
            const int itemIndex = getIndex(item);
            if (itemTrackIndex >= 0 && itemTrackIndex < tracks.size())
            {
                if (auto track = dynamic_cast<otio::Track*>(
                        tracks[itemTrackIndex].value))
                {
                    const auto children = track->children();
                    if (itemIndex >= 0 && itemIndex < children.size())
                    {
                        // Remove the original child
                        track->remove_child(itemIndex);
                    }
                }
            }
            return out;
        }
    }
}
