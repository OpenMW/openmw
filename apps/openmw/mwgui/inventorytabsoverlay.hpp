#ifndef MWGUI_INVENTORYTABSSOVERLAY_H
#define MWGUI_INVENTORYTABSSOVERLAY_H

#include "windowbase.hpp"

namespace MyGUI
{
    class Button;
}

namespace MWGui
{
    class InventoryTabsOverlay : public WindowBase
    {
    public:
        InventoryTabsOverlay();

        int getHeight();
        void setTab(size_t index);

    private:
        std::vector<MyGUI::Button*> mTabs;

        void onTabClicked(MyGUI::Widget* sender);
    };
}

#endif
