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
            void setTakeButtonShow(bool show);

        protected:
            void onNextPageButtonClicked (MyGUI::Widget* _sender);
            void onPrevPageButtonClicked (MyGUI::Widget* _sender);
            void onCloseButtonClicked (MyGUI::Widget* _sender);
            void onTakeButtonClicked (MyGUI::Widget* _sender);

            void updatePages();
            void clearPages();

        private:
            MyGUI::Button* mCloseButton;
            MyGUI::Button* mTakeButton;
            MyGUI::Button* mNextPageButton;
            MyGUI::Button* mPrevPageButton;
            MyGUI::TextBox* mLeftPageNumber;
            MyGUI::TextBox* mRightPageNumber;
            MyGUI::Widget* mLeftPage;
            MyGUI::Widget* mRightPage;

            unsigned int mCurrentPage; // 0 is first page
            std::vector<MyGUI::Widget*> mPages;

            MWWorld::Ptr mBook;
    };

}

#endif

