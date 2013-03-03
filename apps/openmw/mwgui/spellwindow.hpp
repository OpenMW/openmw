#ifndef MWGUI_SPELLWINDOW_H
#define MWGUI_SPELLWINDOW_H

#include "window_pinnable_base.hpp"

namespace MWGui
{
    class SpellIcons;

    class SpellWindow : public WindowPinnableBase
    {
    public:
        SpellWindow(MWBase::WindowManager& parWindowManager);
        virtual ~SpellWindow();

        void updateSpells();

    protected:
        MyGUI::ScrollView* mSpellView;
        MyGUI::Widget* mEffectBox;

        int mHeight;
        int mWidth;

        std::string mSpellToDelete;

        void addGroup(const std::string& label, const std::string& label2);

        int estimateHeight(int numSpells) const;

        void onWindowResize(MyGUI::Window* _sender);
        void onEnchantedItemSelected(MyGUI::Widget* _sender);
        void onSpellSelected(MyGUI::Widget* _sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);
        void onDeleteSpellAccept();

        virtual void onPinToggled();
        virtual void open();

        SpellIcons* mSpellIcons;
    };
}

#endif
