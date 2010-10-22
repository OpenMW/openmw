#ifndef MWGUI_TEXT_INPUT_H
#define MWGUI_TEXT_INPUT_H

#include <openengine/gui/layout.hpp>

namespace MWWorld
{
    class Environment;
}

/*
 */

namespace MWGui
{
    using namespace MyGUI;

    class TextInputDialog : public OEngine::GUI::Layout
    {
    public:
        TextInputDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize);

        std::string getTextInput() const { return textEdit ? textEdit->getOnlyText() : ""; }
        void setTextInput(const std::string &text) { if (textEdit) textEdit->setOnlyText(text); }

        void setNextButtonShow(bool shown);
        void setTextLabel(const std::string &label);
        void open();

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;

        /** Event : Dialog finished, OK button clicked.\n
            signature : void method()\n
        */
        EventHandle_Void eventDone;

    protected:
        void onOkClicked(MyGUI::Widget* _sender);
        void onTextAccepted(MyGUI::Edit* _sender);

    private:
        MWWorld::Environment& environment;

        MyGUI::EditPtr textEdit;
    };
}
#endif
