#ifndef MWGUI_SPELLCREATION_H
#define MWGUI_SPELLCREATION_H

#include <components/esm/loadmgef.hpp>
#include <components/esm/loadspel.hpp>

#include "windowbase.hpp"
#include "referenceinterface.hpp"

namespace Gui
{
    class MWList;
}

namespace MWGui
{

    class SelectSkillDialog;
    class SelectAttributeDialog;

    class EditEffectDialog : public WindowModal
    {
    public:
        EditEffectDialog();

        void onOpen() override;
        bool exit() override;

        void setConstantEffect(bool constant);

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
        ESM::ENAMstruct mOldEffect;

        const ESM::MagicEffect* mMagicEffect;

        bool mConstantEffect;
    };


    class EffectEditorBase
    {
    public:
        enum Type
        {
            Spellmaking,
            Enchanting
        };

        EffectEditorBase(Type type);
        virtual ~EffectEditorBase();

        void setConstantEffect(bool constant);

    protected:
        std::map<int, short> mButtonMapping; // maps button ID to effect ID

        Gui::MWList* mAvailableEffectsList;
        MyGUI::ScrollView* mUsedEffectsView;

        EditEffectDialog mAddEffectDialog;
        SelectAttributeDialog* mSelectAttributeDialog;
        SelectSkillDialog* mSelectSkillDialog;

        int mSelectedEffect;
        short mSelectedKnownEffectId;

        bool mConstantEffect;

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
        void setWidgets (Gui::MWList* availableEffectsList, MyGUI::ScrollView* usedEffectsView);

        virtual void notifyEffectsChanged () {}

    private:
        Type mType;
    };

    class SpellCreationDialog : public WindowBase, public ReferenceInterface, public EffectEditorBase
    {
    public:
        SpellCreationDialog();

        void onOpen() override;
        void clear() override { resetReference(); }

        void onFrame(float dt) override { checkReferenceAvailable(); }

        void setPtr(const MWWorld::Ptr& actor) override;

    protected:
        void onReferenceUnavailable() override;

        void onCancelButtonClicked (MyGUI::Widget* sender);
        void onBuyButtonClicked (MyGUI::Widget* sender);
        void onAccept(MyGUI::EditBox* sender);

        void notifyEffectsChanged() override;

        MyGUI::EditBox* mNameEdit;
        MyGUI::TextBox* mMagickaCost;
        MyGUI::TextBox* mSuccessChance;
        MyGUI::Button* mBuyButton;
        MyGUI::Button* mCancelButton;
        MyGUI::TextBox* mPriceLabel;

        ESM::Spell mSpell;

    };

}

#endif
