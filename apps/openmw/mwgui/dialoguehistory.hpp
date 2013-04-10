#ifndef MWGUI_DIALOGE_HISTORY_H
#define MWGUI_DIALOGE_HISTORY_H
#include <openengine/gui/layout.hpp>

namespace MWGui
{
    class DialogueHistory : public MyGUI::EditBox
    {
        MYGUI_RTTI_DERIVED( DialogueHistory )
        public:
            Widget* getClient() { return mClient; }
            MyGUI::UString getColorAtPos(size_t _pos);
            MyGUI::UString getColorTextAt(size_t _pos);
            void addDialogHeading(const MyGUI::UString& parText);
            void addDialogText(const MyGUI::UString& parText);
    };
}
#endif

