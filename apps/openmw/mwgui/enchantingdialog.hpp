#ifndef MWGUI_ENCHANTINGDIALOG_H
#define MWGUI_ENCHANTINGDIALOG_H

#include <memory>

#include "itemselection.hpp"
#include "spellcreationdialog.hpp"

#include "../mwmechanics/enchanting.hpp"

namespace MWGui
{

    class ItemWidget;

    class EnchantingDialog : public WindowBase, public ReferenceInterface, public EffectEditorBase
    {
    public:
        EnchantingDialog();
        virtual ~EnchantingDialog() = default;

        void onOpen() override;

        void onFrame(float dt) override { checkReferenceAvailable(); }
        void clear() override { resetReference(); }

        void setSoulGem(const MWWorld::Ptr& gem);
        void setItem(const MWWorld::Ptr& item);

        /// Actor Ptr: buy enchantment from this actor
        /// Soulgem Ptr: player self-enchant
        void setPtr(const MWWorld::Ptr& ptr) override;

        void resetReference() override;

        std::string_view getWindowIdForLua() const override { return "EnchantingDialog"; }

    protected:
        void onReferenceUnavailable() override;
        void notifyEffectsChanged() override;

        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onSelectItem(MyGUI::Widget* sender);
        void onSelectSoul(MyGUI::Widget* sender);

        void onItemSelected(MWWorld::Ptr item);
        void onItemCancel();
        void onSoulSelected(MWWorld::Ptr item);
        void onSoulCancel();
        void onBuyButtonClicked(MyGUI::Widget* sender);
        void updateLabels();
        void onTypeButtonClicked(MyGUI::Widget* sender);
        void onAccept(MyGUI::EditBox* sender);

        std::unique_ptr<ItemSelectionDialog> mItemSelectionDialog;

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
