#ifndef MWGUI_DIALOGE_HISTORY_H
#define MWGUI_DIALOGE_HISTORY_H
#include <openengine/gui/layout.hpp>

namespace MWGui
{
    using namespace MyGUI;
    class DialogeHistory : public MyGUI::EditBox
    {
        MYGUI_RTTI_DERIVED( DialogeHistory )
        public:
            Widget* getClient() { return mClient; }
            UString getColorAtPos(size_t _pos);
            UString getColorTextAt(size_t _pos);
            void addDialogHeading(const UString& parText);
            void addDialogText(const UString& parText);
    };
}
#endif

