#ifndef MWGUI_ENCHANTINGDIALOG_H
#define MWGUI_ENCHANTINGDIALOG_H

#include "spellcreationdialog.hpp"

#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/enchanting.hpp"

namespace MWGui
{

    class ItemSelectionDialog;

    class EnchantingDialog : public WindowBase, public ReferenceInterface, public EffectEditorBase
    {
    public:
        EnchantingDialog();
        virtual ~EnchantingDialog();

        virtual void open();
        void startEnchanting(MWWorld::Ptr actor);
        void startSelfEnchanting(MWWorld::Ptr soulgem);

    protected:
        virtual void onReferenceUnavailable();
        virtual void notifyEffectsChanged ();

        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onSelectItem (MyGUI::Widget* sender);
        void onSelectSoul (MyGUI::Widget* sender);
        void onRemoveItem (MyGUI::Widget* sender);
        void onRemoveSoul (MyGUI::Widget* sender);

        void onItemSelected(MWWorld::Ptr item);
        void onItemCancel();
        void onSoulSelected(MWWorld::Ptr item);
        void onSoulCancel();
        void onBuyButtonClicked(MyGUI::Widget* sender);
        void updateLabels();
        void onTypeButtonClicked(MyGUI::Widget* sender);

        ItemSelectionDialog* mItemSelectionDialog;

        MyGUI::Button* mCancelButton;
        MyGUI::ImageBox* mItemBox;
        MyGUI::ImageBox* mSoulBox;

        MyGUI::Button* mTypeButton;
        MyGUI::Button* mBuyButton;

        MyGUI::TextBox* mName;
        MyGUI::TextBox* mEnchantmentPoints;
        MyGUI::TextBox* mCastCost;
        MyGUI::TextBox* mCharge;
        MyGUI::TextBox* mPrice;
        MyGUI::TextBox* mPriceText;

        MWMechanics::Enchanting mEnchanting;
        ESM::EffectList mEffectList;
    };

}

#endif
