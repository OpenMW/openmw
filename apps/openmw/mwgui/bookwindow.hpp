#ifndef MWGUI_BOOKWINDOW_H
#define MWGUI_BOOKWINDOW_H

#include "window_base.hpp"

#include "../mwworld/ptr.hpp"

#include "imagebutton.hpp"

namespace MWGui
{
    class BookWindow : public WindowBase
    {
        public:
            BookWindow(MWBase::WindowManager& parWindowManager);

            void open(MWWorld::Ptr book);
            void setTakeButtonShow(bool show);

        protected:
            void onNextPageButtonClicked (MyGUI::Widget* sender);
            void onPrevPageButtonClicked (MyGUI::Widget* sender);
            void onCloseButtonClicked (MyGUI::Widget* sender);
            void onTakeButtonClicked (MyGUI::Widget* sender);

            void updatePages();
            void clearPages();

        private:
            MWGui::ImageButton* mCloseButton;
            MWGui::ImageButton* mTakeButton;
            MWGui::ImageButton* mNextPageButton;
            MWGui::ImageButton* mPrevPageButton;
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
