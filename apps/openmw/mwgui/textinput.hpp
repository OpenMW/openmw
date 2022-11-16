#ifndef MWGUI_TEXT_INPUT_H
#define MWGUI_TEXT_INPUT_H

#include <string>
#include <string_view>

#include "windowbase.hpp"

namespace MyGUI
{
    class EditBox;
    class Widget;
}

namespace MWGui
{
    class TextInputDialog : public WindowModal
    {
    public:
        TextInputDialog();

        std::string getTextInput() const;
        void setTextInput(const std::string& text);

        void setNextButtonShow(bool shown);
        void setTextLabel(std::string_view label);
        void onOpen() override;

        bool exit() override { return false; }

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_WindowBase eventDone;

    protected:
        void onOkClicked(MyGUI::Widget* _sender);
        void onTextAccepted(MyGUI::Edit* _sender);

    private:
        MyGUI::EditBox* mTextEdit;
    };
}
#endif
