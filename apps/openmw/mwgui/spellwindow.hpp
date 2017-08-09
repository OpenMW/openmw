#ifndef MWGUI_SPELLWINDOW_H
#define MWGUI_SPELLWINDOW_H

#include "windowpinnablebase.hpp"
#include "../mwworld/ptr.hpp"

#include "spellmodel.hpp"

namespace MWGui
{
    class SpellIcons;
    class SpellView;

    class SpellWindow : public WindowPinnableBase, public NoDrop
    {
    public:
        SpellWindow(DragAndDrop* drag);
        virtual ~SpellWindow();

        void updateSpells();

        void onFrame(float dt);

        /// Cycle to next/previous spell
        void cycle(bool next);

    protected:
        MyGUI::Widget* mEffectBox;

        std::string mSpellToDelete;

        void onEnchantedItemSelected(MWWorld::Ptr item, bool alreadyEquipped);
        void onSpellSelected(const std::string& spellId);
        void onModelIndexSelected(SpellModel::ModelIndex index);
        void onDeleteSpellAccept();
        void askDeleteSpell(const std::string& spellId);

        virtual void onPinToggled();
        virtual void onTitleDoubleClicked();
        virtual void open();

        SpellView* mSpellView;
        SpellIcons* mSpellIcons;

    private:
        float mUpdateTimer;
    };
}

#endif
