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

        MyGUI::Widget* mDurationBox;
        MyGUI::Widget* mMagnitudeBox;
        MyGUI::Widget* mAreaBox;

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

        void onMagnitudeMinChanged (MyGUI::ScrollBar* sender, size_t pos);
        void onMagnitudeMaxChanged (MyGUI::ScrollBar* sender, size_t pos);
        void onDurationChanged (MyGUI::ScrollBar* sender, size_t pos);
        void onAreaChanged (MyGUI::ScrollBar* sender, size_t pos);

        void setMagicEffect(const ESM::MagicEffect* effect);

        void updateBoxes();

    protected:
        ESM::ENAMstruct mEffect;

        const ESM::MagicEffect* mMagicEffect;
    };


    class EffectEditorBase
    {
    public:
        EffectEditorBase(MWBase::WindowManager& parWindowManager);


    protected:
        std::map<int, short> mButtonMapping; // maps button ID to effect ID

        Widgets::MWList* mAvailableEffectsList;
        MyGUI::ScrollView* mUsedEffectsView;

        EditEffectDialog mAddEffectDialog;
        SelectAttributeDialog* mSelectAttributeDialog;
        SelectSkillDialog* mSelectSkillDialog;

        int mSelectedEffect;

        std::vector<ESM::ENAMstruct> mEffects;

        void onEffectAdded(ESM::ENAMstruct effect);
        void onEffectModified(ESM::ENAMstruct effect);
        void onEffectRemoved(ESM::ENAMstruct effect);

        void onAvailableEffectClicked (MyGUI::Widget* sender);

        void onAttributeOrSkillCancel();
        void onSelectAttribute();
        void onSelectSkill();

        void onEditEffect(MyGUI::Widget* sender);

        void updateEffectsView();

        void startEditing();
        void setWidgets (Widgets::MWList* availableEffectsList, MyGUI::ScrollView* usedEffectsView);

        virtual void notifyEffectsChanged () {}
    };

    class SpellCreationDialog : public WindowBase, public ReferenceInterface, public EffectEditorBase
    {
    public:
        SpellCreationDialog(MWBase::WindowManager& parWindowManager);

        virtual void open();

        void startSpellMaking(MWWorld::Ptr actor);

    protected:
        virtual void onReferenceUnavailable ();

        void onCancelButtonClicked (MyGUI::Widget* sender);
        void onBuyButtonClicked (MyGUI::Widget* sender);

        virtual void notifyEffectsChanged ();

        MyGUI::EditBox* mNameEdit;
        MyGUI::TextBox* mMagickaCost;
        MyGUI::TextBox* mSuccessChance;
        MyGUI::Button* mBuyButton;
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPriceLabel;

        Widgets::MWEffectList* mUsedEffectsList;

        ESM::Spell mSpell;

    };

}

#endif
