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

    typedef delegates::CDelegate0 EventHandle_Void;

    class TextInputDialog : public OEngine::GUI::Layout
    {
    public:
        TextInputDialog(MWWorld::Environment& environment, const std::string &label, bool showNext, MyGUI::IntSize size);

        std::string getTextInput() const { return textEdit ? textEdit->getOnlyText() : ""; }
        void setTextInput(const std::string &text) { if (textEdit) textEdit->setOnlyText(text); }

        // Events

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
