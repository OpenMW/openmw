#ifndef MWGUI_SCROLLWINDOW_H
#define MWGUI_SCROLLWINDOW_H

#include "window_base.hpp"

#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class ScrollWindow : public WindowBase
    {
        public:
            ScrollWindow(WindowManager& parWindowManager);
            void open(MWWorld::Ptr scroll);
    };

}

#endif
