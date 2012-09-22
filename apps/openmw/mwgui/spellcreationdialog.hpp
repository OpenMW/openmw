#ifndef MWGUI_SPELLCREATION_H
#define MWGUI_SPELLCREATION_H

#include "window_base.hpp"
#include "referenceinterface.hpp"
#include "list.hpp"

namespace MWGui
{

    class SpellCreationDialog : public WindowBase, public ReferenceInterface
    {
    public:
        SpellCreationDialog(MWBase::WindowManager& parWindowManager);

        virtual void open();

        void startSpellMaking(MWWorld::Ptr actor);

    protected:
        virtual void onReferenceUnavailable ();

        void onCancelButtonClicked (MyGUI::Widget* sender);
        void onBuyButtonClicked (MyGUI::Widget* sender);


        MyGUI::EditBox* mNameEdit;
        MyGUI::TextBox* mMagickaCost;
        MyGUI::TextBox* mSuccessChance;
        Widgets::MWList* mAvailableEffectsList;
        MyGUI::ScrollView* mUsedEffectsView;
        MyGUI::Button* mBuyButton;
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPriceLabel;

    };

}

#endif
