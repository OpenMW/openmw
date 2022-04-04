#include "container.hpp"

#include <algorithm>

namespace LuaUi
{
    void LuaContainer::updateChildren()
    {
        WidgetExtension::updateChildren();
        for (auto w : children())
        {
            w->onCoordChange([this](WidgetExtension* child, MyGUI::IntCoord coord)
                { updateSizeToFit(); });
        }
        updateSizeToFit();
    }

    MyGUI::IntSize LuaContainer::childScalingSize()
    {
        return MyGUI::IntSize();
    }

    void LuaContainer::updateSizeToFit()
    {
        MyGUI::IntSize size;
        for (auto w : children())
        {
            MyGUI::IntCoord coord = w->widget()->getCoord();
            size.width = std::max(size.width, coord.left + coord.width);
            size.height = std::max(size.height, coord.top + coord.height);
        }
        forceSize(size);
        updateCoord();
    }
}
