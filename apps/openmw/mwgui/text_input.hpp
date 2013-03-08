#ifndef MWGUI_TEXT_INPUT_H
#define MWGUI_TEXT_INPUT_H

#include "window_base.hpp"

namespace MWGui
{
    class WindowManager;
}

/*
 */

namespace MWGui
{
    class TextInputDialog : public WindowModal
    {
    public:
        TextInputDialog(MWBase::WindowManager& parWindowManager);

        std::string getTextInput() const { return mTextEdit ? mTextEdit->getOnlyText() : ""; }
        void setTextInput(const std::string &text) { if (mTextEdit) mTextEdit->setOnlyText(text); }

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
