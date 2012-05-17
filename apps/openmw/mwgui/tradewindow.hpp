#ifndef MWGUI_TRADEWINDOW_H
#define MWGUI_TRADEWINDOW_H

#include "container.hpp"
#include "window_base.hpp"

namespace MyGUI
{
  class Gui;
  class Widget;
}

namespace MWGui
{
    class WindowManager;
}


namespace MWGui
{
    class TradeWindow : public ContainerBase, public WindowBase
    {
        public:
            TradeWindow(WindowManager& parWindowManager);

            //virtual void Update();
            //virtual void notifyContentChanged();

        protected:
            MyGUI::Button* mFilterAll;
            MyGUI::Button* mFilterWeapon;
            MyGUI::Button* mFilterApparel;
            MyGUI::Button* mFilterMagic;
            MyGUI::Button* mFilterMisc;

            void onWindowResize(MyGUI::Window* _sender);
            void onFilterChanged(MyGUI::Widget* _sender);
    };
}

#endif
