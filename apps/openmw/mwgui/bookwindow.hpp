#ifndef MWGUI_BOOKWINDOW_H
#define MWGUI_BOOKWINDOW_H

#include "window_base.hpp"

#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class BookWindow : public WindowBase
    {
        public:
            BookWindow(WindowManager& parWindowManager);
            void open(MWWorld::Ptr book);
    };

}

#endif

