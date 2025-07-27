#ifndef MWGUI_COUNTDIALOG_H
#define MWGUI_COUNTDIALOG_H

#include "windowbase.hpp"

namespace Gui
{
    class NumericEditBox;
    class ScrollBar;
}

namespace MWGui
{
    class CountDialog : public WindowModal
    {
    public:
        CountDialog();
        void openCountDialog(const std::string& item, const std::string& message, const int maxCount);
        void setCount(int count);

        /** Event : Ok button was clicked.\n
            signature : void method(MyGUI::Widget* sender, std::size_t count)\n
        */
        MyGUI::delegates::MultiDelegate<MyGUI::Widget*, std::size_t> eventOkClicked;

    private:
        Gui::ScrollBar* mSlider;
        Gui::NumericEditBox* mItemEdit;
        MyGUI::TextBox* mItemText;
        MyGUI::TextBox* mLabelText;
        MyGUI::Button* mOkButton;
        MyGUI::Button* mCancelButton;

        void onCancelButtonClicked(MyGUI::Widget* _sender);
        void onOkButtonClicked(MyGUI::Widget* _sender);
        void onEditValueChanged(int value);
        void onSliderMoved(MyGUI::ScrollBar* _sender, size_t _position);
        void onEnterKeyPressed(MyGUI::EditBox* _sender);
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
    };

}

#endif
