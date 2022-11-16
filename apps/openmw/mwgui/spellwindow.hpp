#ifndef MWGUI_SPELLWINDOW_H
#define MWGUI_SPELLWINDOW_H

#include <memory>

#include <apps/openmw/mwgui/windowbase.hpp>

#include <components/esm/refid.hpp>

#include "spellicons.hpp"
#include "spellmodel.hpp"
#include "windowpinnablebase.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MyGUI
{
    class EditBox;
    class Widget;
}

namespace MWGui
{
    class DragAndDrop;
    class SpellView;

    class SpellWindow : public WindowPinnableBase, public NoDrop
    {
    public:
        SpellWindow(DragAndDrop* drag);

        void updateSpells();

        void onFrame(float dt) override;

        /// Cycle to next/previous spell
        void cycle(bool next);

    protected:
        MyGUI::Widget* mEffectBox;

        ESM::RefId mSpellToDelete;

        void onEnchantedItemSelected(MWWorld::Ptr item, bool alreadyEquipped);
        void onSpellSelected(const ESM::RefId& spellId);
        void onModelIndexSelected(SpellModel::ModelIndex index);
        void onFilterChanged(MyGUI::EditBox* sender);
        void onDeleteClicked(MyGUI::Widget* widget);
        void onDeleteSpellAccept();
        void askDeleteSpell(const ESM::RefId& spellId);

        void onPinToggled() override;
        void onTitleDoubleClicked() override;
        void onOpen() override;

        SpellView* mSpellView;
        std::unique_ptr<SpellIcons> mSpellIcons;
        MyGUI::EditBox* mFilterEdit;

    private:
        float mUpdateTimer;
    };
}

#endif
