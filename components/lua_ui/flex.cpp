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

        float getGrow(WidgetExtension* w)
        {
            return std::max(0.0f, w->externalValue("grow", 0.0f));
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
            primary(childrenSize) += primary(size);
            secondary(childrenSize) = std::max(secondary(childrenSize), secondary(size));
            totalGrow += getGrow(w);
        }
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
        secondary(childPosition) = alignSize(secondary(flexSize), secondary(childrenSize), mArrange);
        for (auto* w : children())
        {
            w->forcePosition(childPosition);
            MyGUI::IntSize size = w->widget()->getSize();
            primary(size) += static_cast<int>(growFactor * getGrow(w));
            w->forceSize(size);
            primary(childPosition) += primary(size);
        }
        WidgetExtension::updateProperties();
    }

    MyGUI::IntSize LuaFlex::calculateSize()
    {
        MyGUI::IntSize size = WidgetExtension::calculateSize();
        if (mAutoSized) {
            primary(size) = primary(mChildrenSize);
            secondary(size) = std::max(secondary(size), secondary(mChildrenSize));
        }
        return size;
    }
}
