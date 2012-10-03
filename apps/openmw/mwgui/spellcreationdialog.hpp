#ifndef MWGUI_SPELLCREATION_H
#define MWGUI_SPELLCREATION_H

#include "window_base.hpp"
#include "referenceinterface.hpp"
#include "list.hpp"
#include "widgets.hpp"

namespace MWGui
{

    class SelectSkillDialog;
    class SelectAttributeDialog;

    class EditEffectDialog : public WindowModal
    {
    public:
        EditEffectDialog(MWBase::WindowManager& parWindowManager);

        virtual void open();

        void setSkill(int skill);
        void setAttribute(int attribute);

        void newEffect (const ESM::MagicEffect* effect);
        void editEffect (ESM::ENAMstruct effect);

        typedef MyGUI::delegates::CMultiDelegate1<ESM::ENAMstruct> EventHandle_Effect;

        EventHandle_Effect eventEffectAdded;
        EventHandle_Effect eventEffectModified;
        EventHandle_Effect eventEffectRemoved;

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

        MyGUI::TextBox* mAreaText;

        MyGUI::ImageBox* mEffectImage;
        MyGUI::TextBox* mEffectName;

        bool mEditing;

    protected:
        void onRangeButtonClicked (MyGUI::Widget* sender);
        void onDeleteButtonClicked (MyGUI::Widget* sender);
        void onOkButtonClicked (MyGUI::Widget* sender);
        void onCancelButtonClicked (MyGUI::Widget* sender);

        void setMagicEffect(const ESM::MagicEffect* effect);

    protected:
        ESM::ENAMstruct mEffect;
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

        void onAttributeOrSkillCancel();
        void onSelectAttribute();
        void onSelectSkill();

        void onEffectAdded(ESM::ENAMstruct effect);
        void onEffectModified(ESM::ENAMstruct effect);
        void onEffectRemoved(ESM::ENAMstruct effect);

        void updateEffectsView();

        void onEditEffect(MyGUI::Widget* sender);

        MyGUI::EditBox* mNameEdit;
        MyGUI::TextBox* mMagickaCost;
        MyGUI::TextBox* mSuccessChance;
        Widgets::MWList* mAvailableEffectsList;
        MyGUI::ScrollView* mUsedEffectsView;
        MyGUI::Button* mBuyButton;
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPriceLabel;

        int mSelectedEffect;

        EditEffectDialog mAddEffectDialog;

        SelectAttributeDialog* mSelectAttributeDialog;
        SelectSkillDialog* mSelectSkillDialog;

        Widgets::MWEffectList* mUsedEffectsList;

        std::vector<ESM::ENAMstruct> mEffects;

    };

}

#endif
