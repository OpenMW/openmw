#ifndef MWGUI_SPELLCREATION_H
#define MWGUI_SPELLCREATION_H

#include "window_base.hpp"
#include "referenceinterface.hpp"
#include "list.hpp"

namespace MWGui
{

    class EditEffectDialog : public WindowModal
    {
    public:
        EditEffectDialog(MWBase::WindowManager& parWindowManager);

        virtual void open();

        void setEffect (const ESM::MagicEffect* effect);

    protected:
        MyGUI::Button* mCancelButton;
        MyGUI::Button* mOkButton;
        MyGUI::Button* mDeleteButton;

        MyGUI::Button* mRangeButton;

        MyGUI::TextBox* mMagnitudeMinValue;
        MyGUI::TextBox* mMagnitudeMaxValue;
        MyGUI::TextBox* mDurationValue;
        MyGUI::TextBox* mAreaValue;

        MyGUI::ScrollBar* mMagnitudeMinSlider;
        MyGUI::ScrollBar* mMagnitudeMaxSlider;
        MyGUI::ScrollBar* mDurationSlider;
        MyGUI::ScrollBar* mAreaSlider;

        MyGUI::ScrollBar* mAreaText;

        MyGUI::ImageBox* mEffectImage;
        MyGUI::TextBox* mEffectName;

    protected:
        void onRangeButtonClicked (MyGUI::Widget* sender);
        void onDeleteButtonClicked (MyGUI::Widget* sender);
        void onOkButtonClicked (MyGUI::Widget* sender);
        void onCancelButtonClicked (MyGUI::Widget* sender);

    protected:
        int mRange;
    };

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
        void onAvailableEffectClicked (MyGUI::Widget* sender);


        MyGUI::EditBox* mNameEdit;
        MyGUI::TextBox* mMagickaCost;
        MyGUI::TextBox* mSuccessChance;
        Widgets::MWList* mAvailableEffectsList;
        MyGUI::ScrollView* mUsedEffectsView;
        MyGUI::Button* mBuyButton;
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPriceLabel;

        EditEffectDialog mAddEffectDialog;

    };

}

#endif
