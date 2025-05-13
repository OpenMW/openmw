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
        ScrollWindow();

        void setPtr(const MWWorld::Ptr& scroll) override;
        void setInventoryAllowed(bool allowed);

        void onClose() override;
        void onResChange(int, int) override { center(); }

        std::string_view getWindowIdForLua() const override { return "Scroll"; }

    protected:
        void onCloseButtonClicked(MyGUI::Widget* _sender);
        void onTakeButtonClicked(MyGUI::Widget* _sender);
        void setTakeButtonShow(bool show);
        void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char character);
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        bool onControllerThumbstickEvent(const SDL_ControllerAxisEvent& arg) override;

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
