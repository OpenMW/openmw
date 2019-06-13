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
    class ScrollWindow : public BookWindowBase
    {
        public:
            ScrollWindow ();

            void setPtr (const MWWorld::Ptr& scroll);
            void setInventoryAllowed(bool allowed);

            void onResChange(int, int) { center(); }

        protected:
            void onCloseButtonClicked (MyGUI::Widget* _sender);
            void onTakeButtonClicked (MyGUI::Widget* _sender);
            void setTakeButtonShow(bool show);
            void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);

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
