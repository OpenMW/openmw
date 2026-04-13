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
                const bool hasWidgets = (widgetIndex > trackStart);
                const int primaryToAdd = childPrimary + (hasWidgets ? mGap : 0);

                if (!mAutoSized && mWrap && hasWidgets && primaryAxisSizeRemaining < primaryToAdd)
                    break;

                primaryAxisSize += primaryToAdd;
                if (!mAutoSized && mWrap)
                    primaryAxisSizeRemaining -= primaryToAdd;
                secondaryAxisSize = std::max(childSecondary, secondaryAxisSize);
                totalPrimaryGrow += getGrow(w);
                widgetIndex++;
            }

            if (mAutoSized)
            {
                if (mHorizontal)
                {
                    childrenSize.width = std::max(primaryAxisSize, childrenSize.width);
                    childrenSize.height += secondaryAxisSize;
                }
                else
                {
                    childrenSize.height += primaryAxisSize;
                    childrenSize.width = std::max(secondaryAxisSize, childrenSize.width);
                }
            }

            const int primaryAxisChildrenShift
                = alignSize(primaryAxisSize, primaryAxisSize - primaryAxisSizeRemaining, mAlign);
            int currentPrimaryAxisPosition = primaryAxisChildrenShift;

            for (size_t j = trackStart; j < widgetIndex; ++j)
            {
                auto* w = flexChildren[j];
                const bool isLastWidgetOnTrack = (j == widgetIndex - 1);
                MyGUI::IntPoint widgetPosition;
                MyGUI::IntSize widgetSize = w->calculateSize();

                // Note: Grow is on the primary axis and stretch is "grow" on the cross/secondary axis
                const float widgetGrowFactor = getGrow(w);
                const float stretch = std::clamp(w->externalValue("stretch", 0.0f), 0.0f, 1.0f);
                if (widgetGrowFactor > 0 && primaryAxisSizeRemaining > 0)
                {
                    // Will not rounding to the nearest integer here cause 1-pixel-off scenarios?
                    const int pixelsToExpandBy
                        = static_cast<int>((widgetGrowFactor / totalPrimaryGrow) * primaryAxisSizeRemaining);
                    primary(widgetSize) += pixelsToExpandBy;
                    primaryAxisSizeRemaining -= pixelsToExpandBy;
                }

                secondary(widgetSize) = std::max(secondary(widgetSize), static_cast<int>(stretch * secondaryAxisSize));

                primary(widgetPosition) = currentPrimaryAxisPosition;
                currentPrimaryAxisPosition += primary(widgetSize) + (isLastWidgetOnTrack ? 0 : mGap);
                secondary(widgetPosition)
                    = currentSecondaryAxisPosition + alignSize(secondaryAxisSize, secondary(widgetSize), mArrange);

                w->forceSize(widgetSize);
                w->forcePosition(widgetPosition);
                w->updateCoord();
            }

            currentSecondaryAxisPosition += secondaryAxisSize;
            if (widgetIndex < flexChildren.size())
                currentSecondaryAxisPosition += mGap;
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
