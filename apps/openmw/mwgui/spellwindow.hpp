#ifndef MWGUI_SPELLWINDOW_H
#define MWGUI_SPELLWINDOW_H

#include "windowpinnablebase.hpp"

namespace MWGui
{
    class SpellIcons;

    bool sortItems(const MWWorld::Ptr& left, const MWWorld::Ptr& right)
    {
        int cmp = left.getClass().getName(left).compare(
                    right.getClass().getName(right));
        return cmp < 0;
    }

    bool sortSpells(const std::string& left, const std::string& right)
    {
        const MWWorld::Store<ESM::Spell> &spells =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>();

        const ESM::Spell* a = spells.find(left);
        const ESM::Spell* b = spells.find(right);

        int cmp = a->mName.compare(b->mName);
        return cmp < 0;
    }

    class SpellWindow : public WindowPinnableBase, public NoDrop
    {
    public:
        SpellWindow(DragAndDrop* drag);
        virtual ~SpellWindow();

        void updateSpells();

        void onFrame(float dt) { NoDrop::onFrame(dt); }

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
