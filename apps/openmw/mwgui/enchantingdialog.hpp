#ifndef MWGUI_ENCHANTINGDIALOG_H
#define MWGUI_ENCHANTINGDIALOG_H

#include "window_base.hpp"
#include "referenceinterface.hpp"
#include "spellcreationdialog.hpp"

#include "../mwbase/windowmanager.hpp"

namespace MWGui
{

    class ItemSelectionDialog;

    class EnchantingDialog : public WindowBase, public ReferenceInterface, public EffectEditorBase
    {
    public:
        EnchantingDialog(MWBase::WindowManager& parWindowManager);
        virtual ~EnchantingDialog();

        virtual void open();
        void startEnchanting(MWWorld::Ptr actor);

    protected:
        virtual void onReferenceUnavailable();

        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onSelectItem (MyGUI::Widget* sender);
        void onSelectSoul (MyGUI::Widget* sender);
        void onRemoveItem (MyGUI::Widget* sender);
        void onRemoveSoul (MyGUI::Widget* sender);

        void onItemSelected(MWWorld::Ptr item);
        void onItemCancel();
        void onSoulSelected(MWWorld::Ptr item);
        void onSoulCancel();

        void updateLabels();

        ItemSelectionDialog* mItemSelectionDialog;

        MyGUI::Button* mCancelButton;
        MyGUI::ImageBox* mItemBox;
        MyGUI::ImageBox* mSoulBox;

        MyGUI::Button* mTypeButton;
        MyGUI::Button* mBuyButton;

        MyGUI::TextBox* mEnchantmentPoints;
        MyGUI::TextBox* mCastCost;
        MyGUI::TextBox* mCharge;
        MyGUI::TextBox* mPrice;

        MWWorld::Ptr mItem;
        MWWorld::Ptr mSoul;

        float mCurrentEnchantmentPoints;
    };

}

#endif
