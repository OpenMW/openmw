#ifndef MWGUI_SPELLWINDOW_H
#define MWGUI_SPELLWINDOW_H

#include "window_pinnable_base.hpp"

namespace MWGui
{
    class SpellWindow : public WindowPinnableBase
    {
    public:
        SpellWindow(WindowManager& parWindowManager);

        void updateSpells();

    protected:
        MyGUI::ScrollView* mSpellView;
        MyGUI::Widget* mEffectBox;

        int mHeight;
        int mWidth;

        void addGroup(const std::string& label, const std::string& label2);

        int estimateHeight(int numSpells) const;

        virtual void onPinToggled();
        void onWindowResize(MyGUI::Window* _sender);
        void onEnchantedItemSelected(MyGUI::Widget* _sender);
        void onSpellSelected(MyGUI::Widget* _sender);
        virtual void open();
    };
}

#endif
