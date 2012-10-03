#include "spellcreationdialog.hpp"

#include <components/esm_store/store.hpp>

#include "../mwbase/windowmanager.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "tooltips.hpp"
#include "widgets.hpp"
#include "class.hpp"

namespace
{

    bool sortMagicEffects (short id1, short id2)
    {
        return MWBase::Environment::get().getWorld ()->getStore ().gameSettings.find(ESM::MagicEffect::effectIdToString (id1))->getString()
                < MWBase::Environment::get().getWorld ()->getStore ().gameSettings.find(ESM::MagicEffect::effectIdToString  (id2))->getString();
    }
}

namespace MWGui
{

    EditEffectDialog::EditEffectDialog(MWBase::WindowManager &parWindowManager)
        : WindowModal("openmw_edit_effect.layout", parWindowManager)
        , mEditing(false)
    {
        getWidget(mCancelButton, "CancelButton");
        getWidget(mOkButton, "OkButton");
        getWidget(mDeleteButton, "DeleteButton");
        getWidget(mRangeButton, "RangeButton");
        getWidget(mMagnitudeMinValue, "MagnitudeMinValue");
        getWidget(mMagnitudeMaxValue, "MagnitudeMaxValue");
        getWidget(mDurationValue, "DurationValue");
        getWidget(mAreaValue, "AreaValue");
        getWidget(mMagnitudeMinSlider, "MagnitudeMinSlider");
        getWidget(mMagnitudeMaxSlider, "MagnitudeMaxSlider");
        getWidget(mDurationSlider, "DurationSlider");
        getWidget(mAreaSlider, "AreaSlider");
        getWidget(mEffectImage, "EffectImage");
        getWidget(mEffectName, "EffectName");
        getWidget(mAreaText, "AreaText");

        mRangeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditEffectDialog::onRangeButtonClicked);
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditEffectDialog::onOkButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditEffectDialog::onCancelButtonClicked);
        mDeleteButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditEffectDialog::onDeleteButtonClicked);
    }

    void EditEffectDialog::open()
    {
        WindowModal::open();
        center();

        mEffect.mRange = ESM::RT_Self;

        onRangeButtonClicked(mRangeButton);
    }

    void EditEffectDialog::newEffect (const ESM::MagicEffect *effect)
    {
        setMagicEffect(effect);
        mEditing = false;

        mDeleteButton->setVisible (false);
    }

    void EditEffectDialog::editEffect (ESM::ENAMstruct effect)
    {
        const ESM::MagicEffect* magicEffect =  MWBase::Environment::get().getWorld()->getStore().magicEffects.find(effect.mEffectID);

        setMagicEffect(magicEffect);

        mEffect = effect;
        mEditing = true;

        mDeleteButton->setVisible (true);
    }

    void EditEffectDialog::setMagicEffect (const ESM::MagicEffect *effect)
    {
        std::string icon = effect->mIcon;
        icon[icon.size()-3] = 'd';
        icon[icon.size()-2] = 'd';
        icon[icon.size()-1] = 's';
        icon = "icons\\" + icon;

        mEffectImage->setImageTexture (icon);

        mEffectName->setCaptionWithReplacing("#{"+ESM::MagicEffect::effectIdToString  (effect->mIndex)+"}");

        mEffect.mEffectID = effect->mIndex;
    }

    void EditEffectDialog::onRangeButtonClicked (MyGUI::Widget* sender)
    {
        mEffect.mRange = (mEffect.mRange+1)%3;

        if (mEffect.mRange == ESM::RT_Self)
            mRangeButton->setCaptionWithReplacing ("#{sRangeSelf}");
        else if (mEffect.mRange == ESM::RT_Target)
            mRangeButton->setCaptionWithReplacing ("#{sRangeTarget}");
        else if (mEffect.mRange == ESM::RT_Touch)
            mRangeButton->setCaptionWithReplacing ("#{sRangeTouch}");

        mAreaSlider->setVisible (mEffect.mRange != ESM::RT_Self);
        mAreaText->setVisible (mEffect.mRange != ESM::RT_Self);
    }

    void EditEffectDialog::onDeleteButtonClicked (MyGUI::Widget* sender)
    {
        setVisible(false);

        eventEffectRemoved(mEffect);
    }

    void EditEffectDialog::onOkButtonClicked (MyGUI::Widget* sender)
    {
        setVisible(false);

        if (mEditing)
            eventEffectModified(mEffect);
        else
            eventEffectAdded(mEffect);
    }

    void EditEffectDialog::onCancelButtonClicked (MyGUI::Widget* sender)
    {
        setVisible(false);
    }

    void EditEffectDialog::setSkill (int skill)
    {
        mEffect.mSkill = skill;
    }

    void EditEffectDialog::setAttribute (int attribute)
    {
        mEffect.mAttribute = attribute;
    }

    // ------------------------------------------------------------------------------------------------

    SpellCreationDialog::SpellCreationDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_spellcreation_dialog.layout", parWindowManager)
        , mAddEffectDialog(parWindowManager)
        , mSelectAttributeDialog(NULL)
        , mSelectSkillDialog(NULL)
    {
        getWidget(mNameEdit, "NameEdit");
        getWidget(mMagickaCost, "MagickaCost");
        getWidget(mSuccessChance, "SuccessChance");
        getWidget(mAvailableEffectsList, "AvailableEffects");
        getWidget(mUsedEffectsView, "UsedEffects");
        getWidget(mPriceLabel, "PriceLabel");
        getWidget(mBuyButton, "BuyButton");
        getWidget(mCancelButton, "CancelButton");

        mAddEffectDialog.setVisible(false);

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellCreationDialog::onCancelButtonClicked);
        mBuyButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellCreationDialog::onBuyButtonClicked);

        mAvailableEffectsList->eventWidgetSelected += MyGUI::newDelegate(this, &SpellCreationDialog::onAvailableEffectClicked);

        mAddEffectDialog.eventEffectAdded += MyGUI::newDelegate(this, &SpellCreationDialog::onEffectAdded);
        mAddEffectDialog.eventEffectModified += MyGUI::newDelegate(this, &SpellCreationDialog::onEffectModified);
        mAddEffectDialog.eventEffectRemoved += MyGUI::newDelegate(this, &SpellCreationDialog::onEffectRemoved);
    }


    void SpellCreationDialog::open()
    {
        center();
    }

    void SpellCreationDialog::onReferenceUnavailable ()
    {
        mWindowManager.removeGuiMode (GM_Dialogue);
        mWindowManager.removeGuiMode (GM_SpellCreation);
    }

    void SpellCreationDialog::startSpellMaking (MWWorld::Ptr actor)
    {
        mPtr = actor;

        // get the list of magic effects that are known to the player

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        std::vector<short> knownEffects;

        for (MWMechanics::Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell = MWBase::Environment::get().getWorld()->getStore().spells.find(*it);

            // only normal spells count
            if (spell->mData.mType != ESM::Spell::ST_Spell)
                continue;

            const std::vector<ESM::ENAMstruct>& list = spell->mEffects.mList;
            for (std::vector<ESM::ENAMstruct>::const_iterator it2 = list.begin(); it2 != list.end(); ++it2)
            {
                if (std::find(knownEffects.begin(), knownEffects.end(), it2->mEffectID) == knownEffects.end())
                    knownEffects.push_back(it2->mEffectID);
            }
        }

        std::sort(knownEffects.begin(), knownEffects.end(), sortMagicEffects);

        mAvailableEffectsList->clear ();

        for (std::vector<short>::const_iterator it = knownEffects.begin(); it != knownEffects.end(); ++it)
        {
            mAvailableEffectsList->addItem(MWBase::Environment::get().getWorld ()->getStore ().gameSettings.find(
                                               ESM::MagicEffect::effectIdToString  (*it))->getString());
        }
        mAvailableEffectsList->adjustSize ();

        for (std::vector<short>::const_iterator it = knownEffects.begin(); it != knownEffects.end(); ++it)
        {
            std::string name = MWBase::Environment::get().getWorld ()->getStore ().gameSettings.find(
                                               ESM::MagicEffect::effectIdToString  (*it))->getString();
            MyGUI::Widget* w = mAvailableEffectsList->getItemWidget(name);
            w->setUserData(*it);

            ToolTips::createMagicEffectToolTip (w, *it);
        }

    }

    void SpellCreationDialog::onCancelButtonClicked (MyGUI::Widget* sender)
    {
        mWindowManager.removeGuiMode (MWGui::GM_SpellCreation);
    }

    void SpellCreationDialog::onBuyButtonClicked (MyGUI::Widget* sender)
    {

    }

    void SpellCreationDialog::onSelectAttribute ()
    {
        mAddEffectDialog.setVisible(true);
        mAddEffectDialog.setAttribute (mSelectAttributeDialog->getAttributeId());
        mWindowManager.removeDialog (mSelectAttributeDialog);
        mSelectAttributeDialog = 0;
    }

    void SpellCreationDialog::onSelectSkill ()
    {
        mAddEffectDialog.setVisible(true);
        mAddEffectDialog.setSkill (mSelectSkillDialog->getSkillId ());
        mWindowManager.removeDialog (mSelectSkillDialog);
        mSelectSkillDialog = 0;
    }

    void SpellCreationDialog::onAttributeOrSkillCancel ()
    {
        if (mSelectSkillDialog)
            mWindowManager.removeDialog (mSelectSkillDialog);
        if (mSelectAttributeDialog)
            mWindowManager.removeDialog (mSelectAttributeDialog);

        mSelectSkillDialog = 0;
        mSelectAttributeDialog = 0;
    }

    void SpellCreationDialog::onAvailableEffectClicked (MyGUI::Widget* sender)
    {

        short effectId = *sender->getUserData<short>();
        const ESM::MagicEffect* effect = MWBase::Environment::get().getWorld()->getStore().magicEffects.find(effectId);

        mAddEffectDialog.newEffect (effect);

        if (effect->mData.mFlags & ESM::MagicEffect::TargetSkill)
        {
            delete mSelectSkillDialog;
            mSelectSkillDialog = new SelectSkillDialog(mWindowManager);
            mSelectSkillDialog->eventCancel += MyGUI::newDelegate(this, &SpellCreationDialog::onAttributeOrSkillCancel);
            mSelectSkillDialog->eventItemSelected += MyGUI::newDelegate(this, &SpellCreationDialog::onSelectSkill);
            mSelectSkillDialog->setVisible (true);
        }
        else if (effect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
        {
            delete mSelectAttributeDialog;
            mSelectAttributeDialog = new SelectAttributeDialog(mWindowManager);
            mSelectAttributeDialog->eventCancel += MyGUI::newDelegate(this, &SpellCreationDialog::onAttributeOrSkillCancel);
            mSelectAttributeDialog->eventItemSelected += MyGUI::newDelegate(this, &SpellCreationDialog::onSelectAttribute);
            mSelectAttributeDialog->setVisible (true);
        }
        else
        {
            mAddEffectDialog.setVisible(true);
        }
    }

    void SpellCreationDialog::onEffectModified (ESM::ENAMstruct effect)
    {
        mEffects[mSelectedEffect] = effect;

        updateEffectsView();
    }

    void SpellCreationDialog::onEffectRemoved (ESM::ENAMstruct effect)
    {
        mEffects.erase(mEffects.begin() + mSelectedEffect);
        updateEffectsView();
    }

    void SpellCreationDialog::updateEffectsView ()
    {
        MyGUI::EnumeratorWidgetPtr oldWidgets = mUsedEffectsView->getEnumerator ();
        MyGUI::Gui::getInstance ().destroyWidgets (oldWidgets);

        MyGUI::IntSize size(0,0);

        int i = 0;
        for (std::vector<ESM::ENAMstruct>::const_iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            Widgets::SpellEffectParams params;
            params.mEffectID = it->mEffectID;
            params.mSkill = it->mSkill;
            params.mAttribute = it->mAttribute;
            params.mDuration = it->mDuration;
            params.mMagnMin = it->mMagnMin;
            params.mMagnMax = it->mMagnMax;
            params.mRange = it->mRange;

            MyGUI::Button* button = mUsedEffectsView->createWidget<MyGUI::Button>("", MyGUI::IntCoord(0, size.height, 0, 24), MyGUI::Align::Default);
            button->setUserData(i);
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellCreationDialog::onEditEffect);
            button->setNeedMouseFocus (true);

            Widgets::MWSpellEffectPtr effect = button->createWidget<Widgets::MWSpellEffect>("MW_EffectImage", MyGUI::IntCoord(0,0,0,24), MyGUI::Align::Default);

            effect->setNeedMouseFocus (false);
            effect->setWindowManager (&mWindowManager);
            effect->setSpellEffect (params);

            effect->setSize(effect->getRequestedWidth (), 24);
            button->setSize(effect->getRequestedWidth (), 24);

            size.width = std::max(size.width, effect->getRequestedWidth ());
            size.height += 24;
            ++i;
        }

        mUsedEffectsView->setCanvasSize(size);
    }

    void SpellCreationDialog::onEffectAdded (ESM::ENAMstruct effect)
    {
        mEffects.push_back(effect);

        updateEffectsView();
    }

    void SpellCreationDialog::onEditEffect (MyGUI::Widget *sender)
    {
        int id = *sender->getUserData<int>();

        mSelectedEffect = id;

        mAddEffectDialog.editEffect (mEffects[id]);
        mAddEffectDialog.setVisible (true);
    }
}
