#ifndef MWGUI_INVENTORYTABSSOVERLAY_H
#define MWGUI_INVENTORYTABSSOVERLAY_H

#include <MyGUI_Button.h>

#include "windowbase.hpp"

namespace MWGui
{
    class InventoryTabsOverlay : public WindowBase
    {
    public:
        InventoryTabsOverlay();

        void setTab(int index);

    private:
        std::vector<MyGUI::Button*> mTabs;

        void onTabClicked(MyGUI::Widget* sender);
    };
}

#endif
