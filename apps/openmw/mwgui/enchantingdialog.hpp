#ifndef MWGUI_ENCHANTINGDIALOG_H
#define MWGUI_ENCHANTINGDIALOG_H

#include "spellcreationdialog.hpp"

#include "../mwbase/windowmanager.hpp"

#include "../mwmechanics/enchanting.hpp"

namespace MWGui
{

    class ItemSelectionDialog;
    class ItemWidget;

    class EnchantingDialog : public WindowBase, public ReferenceInterface, public EffectEditorBase
    {
    public:
        EnchantingDialog();
        virtual ~EnchantingDialog();

        void onOpen() override;

        void onFrame(float dt) override { checkReferenceAvailable(); }
        void clear() override { resetReference(); }

        void setSoulGem (const MWWorld::Ptr& gem);
        void setItem (const MWWorld::Ptr& item);

        /// Actor Ptr: buy enchantment from this actor
        /// Soulgem Ptr: player self-enchant
        void setPtr(const MWWorld::Ptr& ptr) override;

        void resetReference() override;

    protected:
        void onReferenceUnavailable() override;
        void notifyEffectsChanged() override;

        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onSelectItem (MyGUI::Widget* sender);
        void onSelectSoul (MyGUI::Widget* sender);

        void onItemSelected(MWWorld::Ptr item);
        void onItemCancel();
        void onSoulSelected(MWWorld::Ptr item);
        void onSoulCancel();
        void onBuyButtonClicked(MyGUI::Widget* sender);
        void updateLabels();
        void onTypeButtonClicked(MyGUI::Widget* sender);
        void onAccept(MyGUI::EditBox* sender);

        ItemSelectionDialog* mItemSelectionDialog;

        MyGUI::Widget* mChanceLayout;

        MyGUI::Button* mCancelButton;
        ItemWidget* mItemBox;
        ItemWidget* mSoulBox;

        MyGUI::Button* mTypeButton;
        MyGUI::Button* mBuyButton;

        MyGUI::EditBox* mName;
        MyGUI::TextBox* mEnchantmentPoints;
        MyGUI::TextBox* mCastCost;
        MyGUI::TextBox* mCharge;
        MyGUI::TextBox* mSuccessChance;
        MyGUI::TextBox* mPrice;
        MyGUI::TextBox* mPriceText;

        MWMechanics::Enchanting mEnchanting;
        ESM::EffectList mEffectList;
    };

}

#endif
