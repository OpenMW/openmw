#ifndef MWGUI_SCROLLWINDOW_H
#define MWGUI_SCROLLWINDOW_H

#include "window_base.hpp"
#include "imagebutton.hpp"

#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class ScrollWindow : public WindowBase
    {
        public:
            ScrollWindow (MWBase::WindowManager& parWindowManager);

            void open (MWWorld::Ptr scroll);
            void setTakeButtonShow(bool show);

        protected:
            void onCloseButtonClicked (MyGUI::Widget* _sender);
            void onTakeButtonClicked (MyGUI::Widget* _sender);

        private:
            MWGui::ImageButton* mCloseButton;
            MWGui::ImageButton* mTakeButton;
            MyGUI::ScrollView* mTextView;

            MWWorld::Ptr mScroll;
    };

}

#endif
