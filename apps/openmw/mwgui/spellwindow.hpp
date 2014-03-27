#ifndef MWGUI_SPELLWINDOW_H
#define MWGUI_SPELLWINDOW_H

#include "windowpinnablebase.hpp"
#include "../mwworld/ptr.hpp"

namespace MWGui
{
    class SpellIcons;

    bool sortItems(const MWWorld::Ptr& left, const MWWorld::Ptr& right);

    bool sortSpells(const std::string& left, const std::string& right);

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
