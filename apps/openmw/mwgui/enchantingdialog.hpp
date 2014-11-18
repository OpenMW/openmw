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

        virtual void open();

        virtual void exit();

        void setSoulGem (const MWWorld::Ptr& gem);
        void setItem (const MWWorld::Ptr& item);

        void startEnchanting(MWWorld::Ptr actor);
        void startSelfEnchanting(MWWorld::Ptr soulgem);

        virtual void resetReference();

    protected:
        virtual void onReferenceUnavailable();
        virtual void notifyEffectsChanged ();

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

        ItemSelectionDialog* mItemSelectionDialog;

        MyGUI::Button* mCancelButton;
        ItemWidget* mItemBox;
        ItemWidget* mSoulBox;

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
