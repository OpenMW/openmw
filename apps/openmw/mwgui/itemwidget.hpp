#ifndef MWGUI_ITEM_WIDGET_H
#define MWGUI_ITEM_WIDGET_H
#include <openengine/gui/layout.hpp>
#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class ItemWidget: public MyGUI::ImageBox
    {
        MYGUI_RTTI_DERIVED( ItemWidget )
    public:

        MWWorld::Ptr mPtr;
        int mPos;
    };
}

#endif