#include "container.hpp"

#include <algorithm>

namespace LuaUi
{
    void LuaContainer::updateChildren()
    {
        WidgetExtension::updateChildren();
        updateSizeToFit();
    }

    MyGUI::IntSize LuaContainer::childScalingSize() const
    {
        return MyGUI::IntSize();
    }

    MyGUI::IntSize LuaContainer::templateScalingSize() const
    {
        return mInnerSize;
    }

    void LuaContainer::updateSizeToFit()
    {
        MyGUI::IntSize innerSize = MyGUI::IntSize();
        for (const auto w : children())
        {
            MyGUI::IntCoord coord = w->calculateCoord();
            innerSize.width = std::max(innerSize.width, coord.left + coord.width);
            innerSize.height = std::max(innerSize.height, coord.top + coord.height);
        }
        MyGUI::IntSize outerSize = innerSize;
        for (const auto w : templateChildren())
        {
            MyGUI::IntCoord coord = w->calculateCoord();
            outerSize.width = std::max(outerSize.width, coord.left + coord.width);
            outerSize.height = std::max(outerSize.height, coord.top + coord.height);
        }
        mInnerSize = innerSize;
        mOuterSize = outerSize;
    }

    MyGUI::IntSize LuaContainer::calculateSize() const
    {
        return mOuterSize;
    }

    void LuaContainer::updateCoord()
    {
        updateSizeToFit();
        WidgetExtension::updateCoord();
    }
}
