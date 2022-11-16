#ifndef MWGUI_SpellBuyingWINDOW_H
#define MWGUI_SpellBuyingWINDOW_H

#include <map>

#include <components/esm/refid.hpp>

#include "referenceinterface.hpp"
#include "windowbase.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MyGUI
{
    class Button;
    class ScrollView;
    class TextBox;
    class Widget;
}

namespace ESM
{
    struct Spell;
}

namespace MWGui
{
    class SpellBuyingWindow : public ReferenceInterface, public WindowBase
    {
    public:
        SpellBuyingWindow();

        void setPtr(const MWWorld::Ptr& actor) override;
        void setPtr(const MWWorld::Ptr& actor, int startOffset);

        void onFrame(float dt) override { checkReferenceAvailable(); }
        void clear() override { resetReference(); }

        void onResChange(int, int) override { center(); }

    protected:
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPlayerGold;

        MyGUI::ScrollView* mSpellsView;

        std::map<MyGUI::Widget*, ESM::RefId> mSpellsWidgetMap;

        void onCancelButtonClicked(MyGUI::Widget* _sender);
        void onSpellButtonClick(MyGUI::Widget* _sender);
        void onMouseWheel(MyGUI::Widget* _sender, int _rel);
        void addSpell(const ESM::Spell& spell);
        void clearSpells();
        int mCurrentY;

        void updateLabels();

        void onReferenceUnavailable() override;

        bool playerHasSpell(const ESM::RefId& id);

    private:
        static bool sortSpells(const ESM::Spell* left, const ESM::Spell* right);
    };
}

#endif
