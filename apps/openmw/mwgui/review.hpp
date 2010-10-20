#ifndef MWGUI_REVIEW_H
#define MWGUI_REVIEW_H

#include <openengine/gui/layout.hpp>

namespace MWWorld
{
    class Environment;
}

/*
This file contains the dialog for reviewing the generated character.
Layout is defined by resources/mygui/openmw_chargen_review_layout.xml.
*/

namespace MWGui
{
    using namespace MyGUI;

    class ReviewDialog : public OEngine::GUI::Layout
    {
    public:
        ReviewDialog(MWWorld::Environment& environment, MyGUI::IntSize gameWindowSize);

        // Events
        typedef delegates::CDelegate0 EventHandle_Void;

        /** Event : Back button clicked.\n
        signature : void method()\n
        */
        EventHandle_Void eventBack;

        /** Event : Dialog finished, OK button clicked.\n
        signature : void method()\n
        */
        EventHandle_Void eventDone;

    protected:
        void onOkClicked(MyGUI::Widget* _sender);
        void onBackClicked(MyGUI::Widget* _sender);

    private:
        MWWorld::Environment& environment;
    };
}
#endif
