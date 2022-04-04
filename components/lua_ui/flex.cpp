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
        MyGUI::IntPoint alignSize(const MyGUI::IntSize& container, const MyGUI::IntSize& content, Alignment alignment)
        {
            MyGUI::IntPoint alignedPosition;
            {
                MyGUI::IntSize alignSize = container;
                switch (alignment)
                {
                case Alignment::Start:
                    alignedPosition = MyGUI::IntPoint(0, 0);
                    break;
                case Alignment::Center:
                    alignSize -= content;
                    alignedPosition = { alignSize.width / 2, alignSize.height / 2 };
                    break;
                case Alignment::End:
                    alignSize -= content;
                    alignedPosition = { alignSize.width, alignSize.height };
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
            childrenSize += w->calculateSize();
            totalGrow += w->externalValue("grow", 0.0f);
        }
        mChildrenSize = childrenSize;

        MyGUI::IntSize flexSize = calculateSize();
        MyGUI::IntSize growSize;
        MyGUI::FloatSize growFactor;
        if (totalGrow > 0)
        {
            growSize = flexSize - childrenSize;
            growFactor = { growSize.width / totalGrow, growSize.height / totalGrow };
        }
        if (mHorizontal)
            flexSize.width -= growSize.width;
        else
            flexSize.height-= growSize.height;

        MyGUI::IntPoint alignedPosition = alignSize(flexSize, childrenSize, mAlign);
        MyGUI::IntPoint arrangedPosition = alignSize(flexSize, childrenSize, mArrange);
        MyGUI::IntPoint childPosition;
        if (mHorizontal)
            childPosition = { alignedPosition.left, arrangedPosition.top };
        else
            childPosition = { arrangedPosition.left, alignedPosition.top };
        for (auto* w : children())
        {
            w->forcePosition(childPosition);
            float grow = w->externalValue("grow", 0);
            MyGUI::IntSize growth(growFactor.width * grow, growFactor.height * grow);
            if (mHorizontal)
            {
                int width = w->widget()->getWidth();
                width += growth.width;
                w->forceSize({width, w->widget()->getHeight()});
                childPosition.left += width;
            }
            else
            {
                int height = w->widget()->getHeight();
                height += growth.height;
                w->forceSize({ w->widget()->getWidth(), height });
                childPosition.top += height;
            }
        }
        WidgetExtension::updateProperties();
    }

    MyGUI::IntSize LuaFlex::calculateSize()
    {
        MyGUI::IntSize size = WidgetExtension::calculateSize();
        if (mAutoSized) {
            if (mHorizontal)
                size.width = mChildrenSize.width;
            else
                size.height = mChildrenSize.height;
        }
        return size;
    }
}
