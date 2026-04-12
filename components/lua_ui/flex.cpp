#include "flex.hpp"

namespace LuaUi
{
    void LuaFlex::updateProperties()
    {
        mGap = propertyValue("gap", 0);
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
        float totalGrow = 0;

        const auto& flexChildren = children();

        // Collect the total size and grow factor of all child widgets
        MyGUI::IntSize childrenSize;
        for (auto* w : flexChildren)
        {
            w->clearForced();
            MyGUI::IntSize size = w->calculateSize();
            primary(childrenSize) += primary(size);
            secondary(childrenSize) = std::max(secondary(childrenSize), secondary(size));
            totalGrow += getGrow(w);
        }

        // We only apply gap if there is more than one child widget
        if (flexChildren.size() > 1)
            primary(childrenSize) += mGap * static_cast<int>(flexChildren.size() - 1);

        mChildrenSize = childrenSize;

        MyGUI::IntSize flexSize = calculateSize();
        int growSize = 0;
        float growFactor = 0;
        if (totalGrow > 0)
        {
            growSize = primary(flexSize) - primary(childrenSize);
            growFactor = growSize / totalGrow;
        }

        MyGUI::IntPoint childPosition;
        primary(childPosition) = alignSize(primary(flexSize) - growSize, primary(childrenSize), mAlign);
        for (size_t i = 0; i < flexChildren.size(); ++i)
        {
            auto* w = flexChildren[i];
            MyGUI::IntSize size = w->calculateSize();
            primary(size) += static_cast<int>(growFactor * getGrow(w));
            float stretch = std::clamp(w->externalValue("stretch", 0.0f), 0.0f, 1.0f);
            secondary(size) = std::max(secondary(size), static_cast<int>(stretch * secondary(flexSize)));
            secondary(childPosition) = alignSize(secondary(flexSize), secondary(size), mArrange);
            w->forcePosition(childPosition);
            w->forceSize(size);
            w->updateCoord();
            primary(childPosition) += primary(size);

            // Push out the next child by the gap
            if (i + 1 < flexChildren.size())
                primary(childPosition) += mGap;
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
