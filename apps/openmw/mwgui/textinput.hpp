#ifndef MWGUI_TEXT_INPUT_H
#define MWGUI_TEXT_INPUT_H

#include "windowbase.hpp"

namespace MWGui
{
    class WindowManager;
}

namespace MWGui
{
    class TextInputDialog : public WindowModal
    {
    public:
        TextInputDialog();

        std::string getTextInput() const { return mTextEdit->getCaption(); }
        void setTextInput(const std::string &text) { mTextEdit->setCaption(text); }

        void setNextButtonShow(bool shown);
        void setTextLabel(const std::string &label);
        virtual void open();

    protected:
        void onOkClicked(MyGUI::Widget* _sender);
        void onTextAccepted(MyGUI::Edit* _sender);

    private:
        MyGUI::EditBox* mTextEdit;
    };
}
#endif
