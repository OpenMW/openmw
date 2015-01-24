#ifndef MWGUI_SCROLLWINDOW_H
#define MWGUI_SCROLLWINDOW_H

#include "windowbase.hpp"

#include "../mwworld/ptr.hpp"

namespace Gui
{
    class ImageButton;
}

namespace MWGui
{
    class ScrollWindow : public WindowBase
    {
        public:
            ScrollWindow ();

            void open (MWWorld::Ptr scroll);
            virtual void exit();
            void setTakeButtonShow(bool show);
            void setInventoryAllowed(bool allowed);

        protected:
            void onCloseButtonClicked (MyGUI::Widget* _sender);
            void onTakeButtonClicked (MyGUI::Widget* _sender);

        private:
            Gui::ImageButton* mCloseButton;
            Gui::ImageButton* mTakeButton;
            MyGUI::ScrollView* mTextView;

            MWWorld::Ptr mScroll;

            bool mTakeButtonShow;
            bool mTakeButtonAllowed;

    };

}

#endif
