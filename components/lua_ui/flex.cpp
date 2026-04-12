#include "flex.hpp"

namespace LuaUi
{
    void LuaFlex::updateProperties()
    {
        mGap = propertyValue("gap", 0);
        mWrap = propertyValue("wrap", false);
        mHorizontal = propertyValue("horizontal", false);
        mAutoSized = propertyValue("autoSize", true);
        mAlign = propertyValue("align", Alignment::Start);
        mArrange = propertyValue("arrange", Alignment::Start);
        WidgetExtension::updateProperties();
    }

    namespace
    {
        // Flex widgets are made up of tracks based on if mWrap is true
        // In most circumstances, flex only has one track - it's primary axis track.
        // We keep track of the "remaining size" on the track to know how much space we have left for widgets that can
        // grow/stretch
        // We don't have a "remaining" for the secondary axis because the secondary axis never shares
        // space with other widgets and is always the max size of the largest widget on that track and its
        // secondary axis size.
        struct FlexTrack
        {
            int primaryAxisSize = 0;
            int primaryAxisSizeRemaining = 0;
            int secondaryAxisSize = 0;
            float totalPrimaryGrow = 0;
            std::vector<WidgetExtension*> widgets;
        };

        int alignSize(int container, int content, Alignment alignment)
        {
            int alignedPosition = 0;
            {
                switch (alignment)
                {
                    case Alignment::Start:
                        alignedPosition = 0;
                        break;
                    case Alignment::Center:
                        alignedPosition = (container - content) / 2;
                        break;
                    case Alignment::End:
                        alignedPosition = container - content;
                        break;
                }
            }
            return alignedPosition;
        }

        float getGrow(WidgetExtension* w)
        {
            return std::max(0.0f, w->externalValue("grow", 0.0f));
        }
    }

    void LuaFlex::updateChildren()
    {
        const auto& flexChildren = children();
        MyGUI::IntSize flexSize = calculateSize();

        std::vector<FlexTrack> tracks;
        tracks.emplace_back();

        // Set up the first track with the correct size remaining
        if (!mAutoSized)
        {
            tracks.back().primaryAxisSizeRemaining = primary(flexSize);
        }

        MyGUI::IntSize childrenSize;
        for (size_t i = 0; i < flexChildren.size(); ++i)
        {
            auto* w = flexChildren[i];
            auto* currentTrack = &tracks.back();

            w->clearForced();

            MyGUI::IntSize size = w->calculateSize();
            const int childPrimary = primary(size);
            const int childSecondary = secondary(size);

            if (!mAutoSized && mWrap)
            {
                const bool hasWidgetsInTrack = !currentTrack->widgets.empty();
                const int neededPrimary = childPrimary + (hasWidgetsInTrack ? mGap : 0);

                if (currentTrack->primaryAxisSizeRemaining - neededPrimary < 0 && hasWidgetsInTrack)
                {
                    // Move to the next track
                    currentTrack = &tracks.emplace_back();
                    currentTrack->primaryAxisSizeRemaining = primary(flexSize);
                }

                const bool hasWidgetsInActiveTrack = !currentTrack->widgets.empty();
                const int primaryToAdd = childPrimary + (hasWidgetsInActiveTrack ? mGap : 0);
                currentTrack->primaryAxisSize += primaryToAdd;
                currentTrack->primaryAxisSizeRemaining -= primaryToAdd;
                currentTrack->secondaryAxisSize = std::max(childSecondary, currentTrack->secondaryAxisSize);
                currentTrack->totalPrimaryGrow += getGrow(w);
            }
            else
            {
                // Auto size is enabled or wrap is disabled. In both cases, we don't need to handle multiple tracks
                // There is also no "remaining" size on the primary axis to keep up with, as when auto size is enabled
                // we'll adjust the flex widget width/height to match its contents' extents.
                const int primaryToAdd = childPrimary + (!currentTrack->widgets.empty() ? mGap : 0);
                currentTrack->primaryAxisSize += primaryToAdd;
                currentTrack->secondaryAxisSize = std::max(childSecondary, currentTrack->secondaryAxisSize);
                currentTrack->totalPrimaryGrow += getGrow(w);
            }

            // Add the iterated widget to the currently-iterated track
            currentTrack->widgets.push_back(w);
        }

        // The total children size is primary axis size and the sum of all secondary axis sizes
        // Note: When calculating the primary axis size we should remove any extra gap that is at the end of the axis
        // (e.g. when wrap is enabled and we have multiple tracks) because the last element on the track doesn't have a
        // gap after it, but we added a gap in the iteration above for every element including the last one on the track
        // However, for the secondary axis size we need to include gap for however many tracks there are, minus one.
        // This is because gap for secondary axes is not added until we position the children in the iteration below,
        // and when we position the children we add gap for every track, excluding the last one. So we need to add gap
        // for the secondary axis here to account for the fact that in the iteration below we will be adding gap for
        // every track.
        for (const auto& track : tracks)
        {
            const int trackCount = static_cast<int>(tracks.size());
            const int secondaryGapTotal = std::max(0, trackCount - 1) * mGap;
            if (mHorizontal)
            {
                childrenSize.width = std::max(track.primaryAxisSize, mChildrenSize.width) - mGap;
                childrenSize.height += track.secondaryAxisSize + secondaryGapTotal;
            }
            else
            {
                childrenSize.height += std::max(track.primaryAxisSize, mChildrenSize.height) - mGap;
                childrenSize.width = track.secondaryAxisSize + secondaryGapTotal;
            }
        }

        mChildrenSize = childrenSize;

        // Now, we'll iterate over each track and expand any widgets with grow > 0 to fill the remaining primary track
        // space
        int currentSecondaryAxisPosition = 0;
        for (size_t i = 0; i < tracks.size(); ++i)
        {
            auto& track = tracks[i];
            // For the primary axis, we'll first get an offset based on the set alignment property
            int primaryAxisChildrenShift
                = alignSize(track.primaryAxisSize, track.primaryAxisSize - track.primaryAxisSizeRemaining, mAlign);

            int secondaryAxisChildrenShift = currentSecondaryAxisPosition;

            // This represents the next pixel location to place the next child in the iteration below
            int currentPrimaryAxisPosition = primaryAxisChildrenShift;
            for (size_t j = 0; j < track.widgets.size(); ++j)
            {
                auto* w = track.widgets[j];
                bool isLastWidgetOnTrack = j == track.widgets.size() - 1;
                MyGUI::IntPoint widgetPosition;
                MyGUI::IntSize widgetSize = w->calculateSize();

                // Note: Grow is on the primary axis and stretch is "grow" on the cross/secondary axis
                float widgetGrowFactor = getGrow(w);
                float stretch = std::clamp(w->externalValue("stretch", 0.0f), 0.0f, 1.0f);
                if (widgetGrowFactor > 0 && track.primaryAxisSizeRemaining > 0)
                {
                    // Size the child element on this track

                    // Will not rounding to the nearest integer here cause 1-pixel-off secenarios?
                    int pixelsToExpandBy = static_cast<int>(
                        (widgetGrowFactor / track.totalPrimaryGrow) * track.primaryAxisSizeRemaining);
                    primary(widgetSize) += pixelsToExpandBy;
                    track.primaryAxisSizeRemaining -= pixelsToExpandBy;
                }

                secondary(widgetSize)
                    = std::max(secondary(widgetSize), static_cast<int>(stretch * track.secondaryAxisSize));

                // Position the child element on this track
                primary(widgetPosition) += currentPrimaryAxisPosition;
                currentPrimaryAxisPosition += primary(widgetSize) + mGap * (!isLastWidgetOnTrack ? 1 : 0);
                secondary(widgetPosition)
                    += secondaryAxisChildrenShift + alignSize(track.secondaryAxisSize, secondary(widgetSize), mArrange);

                w->forceSize(widgetSize);
                w->forcePosition(widgetPosition);
                w->updateCoord();
            }

            currentSecondaryAxisPosition += track.secondaryAxisSize;
            if (i + 1 < tracks.size())
                currentSecondaryAxisPosition += mGap;
        }

        WidgetExtension::updateChildren();
    }

    MyGUI::IntSize LuaFlex::childScalingSize() const
    {
        // Call the base method to prevent relativeSize feedback loop
        MyGUI::IntSize size = WidgetExtension::calculateSize();
        if (mAutoSized)
            primary(size) = 0;
        return size;
    }

    MyGUI::IntSize LuaFlex::calculateSize() const
    {
        MyGUI::IntSize size = WidgetExtension::calculateSize();
        if (mAutoSized)
        {
            primary(size) = std::max(primary(size), primary(mChildrenSize));
            secondary(size) = std::max(secondary(size), secondary(mChildrenSize));
        }
        return size;
    }

    void LuaFlex::updateCoord()
    {
        updateChildren();
        WidgetExtension::updateCoord();
    }

    const std::vector<std::string_view>& LuaFlex::allUsedProperties() const
    {
        static std::vector<std::string_view> usedProps = std::invoke([this] {
            std::vector<std::string_view> props = { "horizontal", "autoSize", "arrange", "align", "gap" };
            auto baseProps = WidgetExtension::allUsedProperties();
            props.insert(props.end(), baseProps.begin(), baseProps.end());
            return props;
        });
        return usedProps;
    }
}
