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

        protected:
            void onNextPageButtonClicked (MyGUI::Widget* _sender);
            void onPrevPageButtonClicked (MyGUI::Widget* _sender);
            void onCloseButtonClicked (MyGUI::Widget* _sender);
            void onTakeButtonClicked (MyGUI::Widget* _sender);

        private:
            MyGUI::Button* mCloseButton;
            MyGUI::Button* mTakeButton;
            MyGUI::Button* mNextPageButton;
            MyGUI::Button* mPrevPageButton;

            MWWorld::Ptr mBook;
    };

}

#endif

