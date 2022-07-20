#include "container.hpp"

#include <algorithm>

namespace LuaUi
{
    void LuaContainer::updateChildren()
    {
        WidgetExtension::updateChildren();
        updateSizeToFit();
    }

    MyGUI::IntSize LuaContainer::childScalingSize()
    {
        return MyGUI::IntSize();
    }

    MyGUI::IntSize LuaContainer::templateScalingSize()
    {
        return mInnerSize;
    }

    void LuaContainer::updateSizeToFit()
    {
        MyGUI::IntSize innerSize = MyGUI::IntSize();
        for (auto w : children())
        {
            MyGUI::IntCoord coord = w->calculateCoord();
            innerSize.width = std::max(innerSize.width, coord.left + coord.width);
            innerSize.height = std::max(innerSize.height, coord.top + coord.height);
        }
        MyGUI::IntSize outerSize = innerSize;
        for (auto w : templateChildren())
        {
            MyGUI::IntCoord coord = w->calculateCoord();
            outerSize.width = std::max(outerSize.width, coord.left + coord.width);
            outerSize.height = std::max(outerSize.height, coord.top + coord.height);
        }
        mInnerSize = innerSize;
        mOuterSize = outerSize;
    }

    MyGUI::IntSize LuaContainer::calculateSize()
    {
        return mOuterSize;
    }

    void LuaContainer::updateCoord()
    {
        updateSizeToFit();
        WidgetExtension::updateCoord();
    }
}
