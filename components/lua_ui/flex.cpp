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
        int alignSize(int container, int content, Alignment alignment)
        {
            switch (alignment)
            {
                case Alignment::Start:
                    return 0;
                case Alignment::Center:
                    return (container - content) / 2;
                case Alignment::End:
                    return container - content;
            }
            return 0;
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

        MyGUI::IntSize childrenSize;
        int currentSecondaryAxisPosition = 0;
        size_t widgetIndex = 0;

        while (widgetIndex < flexChildren.size())
        {
            const size_t trackStart = widgetIndex;
            int primaryAxisSize = 0;
            int primaryAxisSizeRemaining = mAutoSized ? 0 : primary(flexSize);
            int secondaryAxisSize = 0;
            float totalPrimaryGrow = 0;

            while (widgetIndex < flexChildren.size())
            {
                auto* w = flexChildren[widgetIndex];
                w->clearForced();

                const MyGUI::IntSize size = w->calculateSize();
                const int childPrimary = primary(size);
                const int childSecondary = secondary(size);
                const bool notFirstOnTrack = (widgetIndex > trackStart);
                const int primaryToAdd = childPrimary + (notFirstOnTrack ? mGap : 0);

                if (!mAutoSized && mWrap && notFirstOnTrack && primaryAxisSizeRemaining < primaryToAdd)
                    break;

                primaryAxisSize += primaryToAdd;
                if (!mAutoSized)
                    primaryAxisSizeRemaining -= primaryToAdd;
                secondaryAxisSize = std::max(childSecondary, secondaryAxisSize);
                totalPrimaryGrow += getGrow(w);
                widgetIndex++;
            }
            primaryAxisSizeRemaining = std::max(0, primaryAxisSizeRemaining);

            if (mAutoSized)
            {
                primary(childrenSize) = std::max(primaryAxisSize, primary(childrenSize));
                secondary(childrenSize) += secondaryAxisSize;
            }

            const int primaryAxisChildrenShift
                = alignSize(primaryAxisSize, primaryAxisSize - primaryAxisSizeRemaining, mAlign);
            int currentPrimaryAxisPosition = primaryAxisChildrenShift;

            // Keeping a constant total here ensures widgets that use grow don't use the
            // changing 'primaryAxisSizeRemaining' (which shrinks as we take off grown amounts
            const int totalPrimaryAxisSizeRemaining = primaryAxisSizeRemaining;

            for (size_t j = trackStart; j < widgetIndex; ++j)
            {
                auto* w = flexChildren[j];
                MyGUI::IntPoint widgetPosition;
                MyGUI::IntSize widgetSize = w->calculateSize();

                // Note: Grow is on the primary axis and stretch is "grow" on the cross/secondary axis
                const float widgetGrowFactor = getGrow(w);
                const float stretch = std::clamp(w->externalValue("stretch", 0.0f), 0.0f, 1.0f);
                if (widgetGrowFactor > 0)
                {
                    const int pixelsToExpandBy = std::clamp(
                        static_cast<int>(
                            std::round((widgetGrowFactor / totalPrimaryGrow) * totalPrimaryAxisSizeRemaining)),
                        0, primaryAxisSizeRemaining);
                    primary(widgetSize) += pixelsToExpandBy;
                    primaryAxisSizeRemaining -= pixelsToExpandBy;
                }

                secondary(widgetSize) = std::max(secondary(widgetSize),
                    std::clamp(static_cast<int>(std::round(stretch * secondaryAxisSize)), 0, secondaryAxisSize));

                primary(widgetPosition) = currentPrimaryAxisPosition;
                currentPrimaryAxisPosition += primary(widgetSize) + mGap;
                secondary(widgetPosition)
                    = currentSecondaryAxisPosition + alignSize(secondaryAxisSize, secondary(widgetSize), mArrange);

                w->forceSize(widgetSize);
                w->forcePosition(widgetPosition);
                w->updateCoord();
            }

            currentSecondaryAxisPosition += secondaryAxisSize + mGap;
        }

        mChildrenSize = childrenSize;
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
            std::vector<std::string_view> props = { "horizontal", "autoSize", "arrange", "align", "gap", "wrap" };
            auto baseProps = WidgetExtension::allUsedProperties();
            props.insert(props.end(), baseProps.begin(), baseProps.end());
            return props;
        });
        return usedProps;
    }
}
