#ifndef MWGUI_SPELLWINDOW_H
#define MWGUI_SPELLWINDOW_H

#include "window_pinnable_base.hpp"

namespace MWGui
{
    class SpellWindow : public WindowPinnableBase
    {
    public:
        SpellWindow(WindowManager& parWindowManager);

    protected:
        MyGUI::ScrollView* mSpellView;
        MyGUI::Widget* mEffectBox;

        virtual void onPinToggled();
    };
}

#endif
