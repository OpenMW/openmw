#include "flex.hpp"

namespace LuaUi
{
    void LuaFlex::updateProperties()
    {
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
    }

    void LuaFlex::updateChildren()
    {
        float totalGrow = 0;
        MyGUI::IntSize childrenSize;
        for (auto* w: children())
        {
            w->clearForced();
            MyGUI::IntSize size = w->calculateSize();
            setPrimary(childrenSize, getPrimary(childrenSize) + getPrimary(size));
            setSecondary(childrenSize, std::max(getSecondary(childrenSize), getSecondary(size)));
            totalGrow += std::max(0.0f, w->externalValue("grow", 0.0f));
        }
        mChildrenSize = childrenSize;

        MyGUI::IntSize flexSize = calculateSize();
        int growSize = 0;
        float growFactor = 0;
        if (totalGrow > 0)
        {
            growSize = getPrimary(flexSize) - getPrimary(childrenSize);
            growFactor = growSize / totalGrow;
        }
        setPrimary(flexSize, getPrimary(flexSize) - growSize);

        MyGUI::IntPoint childPosition;
        setPrimary(childPosition, alignSize(getPrimary(flexSize), getPrimary(childrenSize), mAlign));
        setSecondary(childPosition, alignSize(getSecondary(flexSize), getSecondary(childrenSize), mArrange));
        for (auto* w : children())
        {
            w->forcePosition(childPosition);
            MyGUI::IntSize size = w->widget()->getSize();
            float grow = std::max(0.0f, w->externalValue("grow", 0.0f));
            setPrimary(size, getPrimary(size) + static_cast<int>(growFactor * grow));
            w->forceSize(size);
            setPrimary(childPosition, getPrimary(childPosition) + getPrimary(size));
        }
        WidgetExtension::updateProperties();
    }

    MyGUI::IntSize LuaFlex::calculateSize()
    {
        MyGUI::IntSize size = WidgetExtension::calculateSize();
        if (mAutoSized) {
            setPrimary(size, getPrimary(mChildrenSize));
            setSecondary(size, std::max(getSecondary(size), getSecondary(mChildrenSize)));
        }
        return size;
    }
}
