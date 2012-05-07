#ifndef MWGUI_SCROLLWINDOW_H
#define MWGUI_SCROLLWINDOW_H

#include "window_base.hpp"

#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class ScrollWindow : public WindowBase
    {
        public:
            ScrollWindow (WindowManager& parWindowManager);
            void open (MWWorld::Ptr scroll);

        protected:
            void onCloseButtonClicked (MyGUI::Widget* _sender);
            void onTakeButtonClicked (MyGUI::Widget* _sender);

        private:
            MyGUI::Button* mCloseButton;
            MyGUI::Button* mTakeButton;
            MyGUI::ScrollView* mTextView;

            MWWorld::Ptr mScroll;
    };

}

#endif
