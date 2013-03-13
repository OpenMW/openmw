#include "spellcreationdialog.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/windowmanager.hpp"

#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spellsuccess.hpp"


#include "tooltips.hpp"
#include "widgets.hpp"
#include "class.hpp"
#include "inventorywindow.hpp"
#include "tradewindow.hpp"

namespace
{

    bool sortMagicEffects (short id1, short id2)
    {
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        return gmst.find(ESM::MagicEffect::effectIdToString (id1))->getString()
                < gmst.find(ESM::MagicEffect::effectIdToString  (id2))->getString();
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
        getWidget(mDurationBox, "DurationBox");
        getWidget(mAreaBox, "AreaBox");
        getWidget(mMagnitudeBox, "MagnitudeBox");

        mRangeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditEffectDialog::onRangeButtonClicked);
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditEffectDialog::onOkButtonClicked);
        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditEffectDialog::onCancelButtonClicked);
        mDeleteButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditEffectDialog::onDeleteButtonClicked);

        mMagnitudeMinSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &EditEffectDialog::onMagnitudeMinChanged);
        mMagnitudeMaxSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &EditEffectDialog::onMagnitudeMaxChanged);
        mDurationSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &EditEffectDialog::onDurationChanged);
        mAreaSlider->eventScrollChangePosition += MyGUI::newDelegate(this, &EditEffectDialog::onAreaChanged);
    }

    void EditEffectDialog::open()
    {
        WindowModal::open();
        center();
    }

    void EditEffectDialog::newEffect (const ESM::MagicEffect *effect)
    {
        setMagicEffect(effect);
        mEditing = false;

        mDeleteButton->setVisible (false);

        mEffect.mRange = ESM::RT_Self;

        onRangeButtonClicked(mRangeButton);

        mMagnitudeMinSlider->setScrollPosition (0);
        mMagnitudeMaxSlider->setScrollPosition (0);
        mAreaSlider->setScrollPosition (0);
        mDurationSlider->setScrollPosition (0);

        mDurationValue->setCaption("1");
        mMagnitudeMinValue->setCaption("1");
        mMagnitudeMaxValue->setCaption("- 1");
        mAreaValue->setCaption("0");

        mEffect.mMagnMin = 1;
        mEffect.mMagnMax = 1;
        mEffect.mDuration = 1;
        mEffect.mArea = 0;
    }

    void EditEffectDialog::editEffect (ESM::ENAMstruct effect)
    {
        const ESM::MagicEffect* magicEffect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effect.mEffectID);

        setMagicEffect(magicEffect);

        mEffect = effect;
        mEditing = true;

        mDeleteButton->setVisible (true);

        mMagnitudeMinSlider->setScrollPosition (effect.mMagnMin-1);
        mMagnitudeMaxSlider->setScrollPosition (effect.mMagnMax-1);
        mAreaSlider->setScrollPosition (effect.mArea);
        mDurationSlider->setScrollPosition (effect.mDuration-1);

        onMagnitudeMinChanged (mMagnitudeMinSlider, effect.mMagnMin-1);
        onMagnitudeMaxChanged (mMagnitudeMinSlider, effect.mMagnMax-1);
        onAreaChanged (mAreaSlider, effect.mArea);
        onDurationChanged (mDurationSlider, effect.mDuration-1);
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

        mMagicEffect = effect;

        updateBoxes();
    }

    void EditEffectDialog::updateBoxes()
    {
        static int startY = mMagnitudeBox->getPosition().top;
        int curY = startY;

        mMagnitudeBox->setVisible (false);
        mDurationBox->setVisible (false);
        mAreaBox->setVisible (false);

        if (!(mMagicEffect->mData.mFlags & ESM::MagicEffect::NoMagnitude))
        {
            mMagnitudeBox->setPosition(mMagnitudeBox->getPosition().left, curY);
            mMagnitudeBox->setVisible (true);
            curY += mMagnitudeBox->getSize().height;
        }
        if (!(mMagicEffect->mData.mFlags & ESM::MagicEffect::NoDuration))
        {
            mDurationBox->setPosition(mDurationBox->getPosition().left, curY);
            mDurationBox->setVisible (true);
            curY += mDurationBox->getSize().height;
        }
        if (mEffect.mRange == ESM::RT_Target)
        {
            mAreaBox->setPosition(mAreaBox->getPosition().left, curY);
            mAreaBox->setVisible (true);
            curY += mAreaBox->getSize().height;
        }
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

        // cycle through range types until we find something that's allowed
        if (mEffect.mRange == ESM::RT_Target && !(mMagicEffect->mData.mFlags & ESM::MagicEffect::CastTarget))
            onRangeButtonClicked(sender);
        if (mEffect.mRange == ESM::RT_Self && !(mMagicEffect->mData.mFlags & ESM::MagicEffect::CastSelf))
            onRangeButtonClicked(sender);
        if (mEffect.mRange == ESM::RT_Touch && !(mMagicEffect->mData.mFlags & ESM::MagicEffect::CastTouch))
            onRangeButtonClicked(sender);

        updateBoxes();
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

    void EditEffectDialog::onMagnitudeMinChanged (MyGUI::ScrollBar* sender, size_t pos)
    {
        mMagnitudeMinValue->setCaption(boost::lexical_cast<std::string>(pos+1));
        mEffect.mMagnMin = pos+1;

        // trigger the check again (see below)
        onMagnitudeMaxChanged(mMagnitudeMaxSlider, mMagnitudeMaxSlider->getScrollPosition ());
    }

    void EditEffectDialog::onMagnitudeMaxChanged (MyGUI::ScrollBar* sender, size_t pos)
    {
        // make sure the max value is actually larger or equal than the min value
        size_t magnMin = std::abs(mEffect.mMagnMin); // should never be < 0, this is just here to avoid the compiler warning
        if (pos+1 < magnMin)
        {
            pos = mEffect.mMagnMin-1;
            sender->setScrollPosition (pos);
        }

        mEffect.mMagnMax = pos+1;

        mMagnitudeMaxValue->setCaption("- " + boost::lexical_cast<std::string>(pos+1));
    }

    void EditEffectDialog::onDurationChanged (MyGUI::ScrollBar* sender, size_t pos)
    {
        mDurationValue->setCaption(boost::lexical_cast<std::string>(pos+1));
        mEffect.mDuration = pos+1;
    }

    void EditEffectDialog::onAreaChanged (MyGUI::ScrollBar* sender, size_t pos)
    {
        mAreaValue->setCaption(boost::lexical_cast<std::string>(pos));
        mEffect.mArea = pos;
    }

    // ------------------------------------------------------------------------------------------------

    SpellCreationDialog::SpellCreationDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_spellcreation_dialog.layout", parWindowManager)
        , EffectEditorBase(parWindowManager)
    {
        getWidget(mNameEdit, "NameEdit");
        getWidget(mMagickaCost, "MagickaCost");
        getWidget(mSuccessChance, "SuccessChance");
        getWidget(mAvailableEffectsList, "AvailableEffects");
        getWidget(mUsedEffectsView, "UsedEffects");
        getWidget(mPriceLabel, "PriceLabel");
        getWidget(mBuyButton, "BuyButton");
        getWidget(mCancelButton, "CancelButton");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellCreationDialog::onCancelButtonClicked);
        mBuyButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellCreationDialog::onBuyButtonClicked);

        setWidgets(mAvailableEffectsList, mUsedEffectsView);
    }

    void SpellCreationDialog::startSpellMaking (MWWorld::Ptr actor)
    {
        mPtr = actor;
        mNameEdit->setCaption("");

        startEditing();
    }

    void SpellCreationDialog::onCancelButtonClicked (MyGUI::Widget* sender)
    {
        mWindowManager.removeGuiMode (MWGui::GM_SpellCreation);
    }

    void SpellCreationDialog::onBuyButtonClicked (MyGUI::Widget* sender)
    {
        if (mEffects.size() <= 0)
        {
            mWindowManager.messageBox ("#{sNotifyMessage30}", std::vector<std::string>());
            return;
        }

        if (mNameEdit->getCaption () == "")
        {
            mWindowManager.messageBox ("#{sNotifyMessage10}", std::vector<std::string>());
            return;
        }

        if (mMagickaCost->getCaption() == "0")
        {
            mWindowManager.messageBox ("#{sEnchantmentMenu8}", std::vector<std::string>());
            return;
        }

        if (boost::lexical_cast<int>(mPriceLabel->getCaption()) > mWindowManager.getInventoryWindow()->getPlayerGold())
        {
            mWindowManager.messageBox ("#{sNotifyMessage18}", std::vector<std::string>());
            return;
        }

        mSpell.mName = mNameEdit->getCaption();

        mWindowManager.getTradeWindow()->addOrRemoveGold(-boost::lexical_cast<int>(mPriceLabel->getCaption()));

        MWBase::Environment::get().getSoundManager()->playSound ("Item Gold Up", 1.0, 1.0);

        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->createRecord(mSpell);

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();
        spells.add (spell->mId);

        MWBase::Environment::get().getSoundManager()->playSound ("Item Gold Up", 1.0, 1.0);

        mWindowManager.removeGuiMode (GM_SpellCreation);
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

    void SpellCreationDialog::notifyEffectsChanged ()
    {
        float y = 0;

        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        for (std::vector<ESM::ENAMstruct>::const_iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            float x = 0.5 * it->mMagnMin + it->mMagnMax;

            const ESM::MagicEffect* effect =
                store.get<ESM::MagicEffect>().find(it->mEffectID);

            x *= 0.1 * effect->mData.mBaseCost;
            x *= 1 + it->mDuration;
            x += 0.05 * std::max(1, it->mArea) * effect->mData.mBaseCost;

            float fEffectCostMult =
                store.get<ESM::GameSetting>().find("fEffectCostMult")->getFloat();

            y += x * fEffectCostMult;
            y = std::max(1.f,y);

            if (effect->mData.mFlags & ESM::MagicEffect::CastTarget)
                y *= 1.5;
        }

        mSpell.mData.mCost = int(y);

        ESM::EffectList effectList;
        effectList.mList = mEffects;
        mSpell.mEffects = effectList;
        mSpell.mData.mType = ESM::Spell::ST_Spell;

        mMagickaCost->setCaption(boost::lexical_cast<std::string>(int(y)));

        float fSpellMakingValueMult =
            store.get<ESM::GameSetting>().find("fSpellMakingValueMult")->getFloat();

        int price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr,int(y) * fSpellMakingValueMult,true);

        mPriceLabel->setCaption(boost::lexical_cast<std::string>(int(price)));

        float chance = MWMechanics::getSpellSuccessChance(&mSpell, MWBase::Environment::get().getWorld()->getPlayer().getPlayer());
        mSuccessChance->setCaption(boost::lexical_cast<std::string>(int(chance)));
    }

    // ------------------------------------------------------------------------------------------------


    EffectEditorBase::EffectEditorBase(MWBase::WindowManager& parWindowManager)
        : mAddEffectDialog(parWindowManager)
        , mSelectAttributeDialog(NULL)
        , mSelectSkillDialog(NULL)
    {
        mAddEffectDialog.eventEffectAdded += MyGUI::newDelegate(this, &EffectEditorBase::onEffectAdded);
        mAddEffectDialog.eventEffectModified += MyGUI::newDelegate(this, &EffectEditorBase::onEffectModified);
        mAddEffectDialog.eventEffectRemoved += MyGUI::newDelegate(this, &EffectEditorBase::onEffectRemoved);

        mAddEffectDialog.setVisible (false);
    }

    void EffectEditorBase::startEditing ()
    {
        // get the list of magic effects that are known to the player

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWMechanics::CreatureStats& stats = MWWorld::Class::get(player).getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        std::vector<short> knownEffects;

        for (MWMechanics::Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find (it->first);

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

        int i=0;
        for (std::vector<short>::const_iterator it = knownEffects.begin(); it != knownEffects.end(); ++it)
        {
            mAvailableEffectsList->addItem(MWBase::Environment::get().getWorld ()->getStore ().get<ESM::GameSetting>().find(
                                               ESM::MagicEffect::effectIdToString  (*it))->getString());
            mButtonMapping[i] = *it;
            ++i;
        }
        mAvailableEffectsList->adjustSize ();

        for (std::vector<short>::const_iterator it = knownEffects.begin(); it != knownEffects.end(); ++it)
        {
            std::string name = MWBase::Environment::get().getWorld ()->getStore ().get<ESM::GameSetting>().find(
                                               ESM::MagicEffect::effectIdToString  (*it))->getString();
            MyGUI::Widget* w = mAvailableEffectsList->getItemWidget(name);

            ToolTips::createMagicEffectToolTip (w, *it);
        }

        mEffects.clear();
        updateEffectsView ();
    }

    void EffectEditorBase::setWidgets (Widgets::MWList *availableEffectsList, MyGUI::ScrollView *usedEffectsView)
    {
        mAvailableEffectsList = availableEffectsList;
        mUsedEffectsView = usedEffectsView;

        mAvailableEffectsList->eventWidgetSelected += MyGUI::newDelegate(this, &EffectEditorBase::onAvailableEffectClicked);
    }

    void EffectEditorBase::onSelectAttribute ()
    {
        mAddEffectDialog.setVisible(true);
        mAddEffectDialog.setAttribute (mSelectAttributeDialog->getAttributeId());
        MWBase::Environment::get().getWindowManager ()->removeDialog (mSelectAttributeDialog);
        mSelectAttributeDialog = 0;
    }

    void EffectEditorBase::onSelectSkill ()
    {
        mAddEffectDialog.setVisible(true);
        mAddEffectDialog.setSkill (mSelectSkillDialog->getSkillId ());
        MWBase::Environment::get().getWindowManager ()->removeDialog (mSelectSkillDialog);
        mSelectSkillDialog = 0;
    }

    void EffectEditorBase::onAttributeOrSkillCancel ()
    {
        if (mSelectSkillDialog)
            MWBase::Environment::get().getWindowManager ()->removeDialog (mSelectSkillDialog);
        if (mSelectAttributeDialog)
            MWBase::Environment::get().getWindowManager ()->removeDialog (mSelectAttributeDialog);

        mSelectSkillDialog = 0;
        mSelectAttributeDialog = 0;
    }

    void EffectEditorBase::onAvailableEffectClicked (MyGUI::Widget* sender)
    {
        if (mEffects.size() >= 8)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage28}", std::vector<std::string>());
            return;
        }

        int buttonId = *sender->getUserData<int>();
        short effectId = mButtonMapping[buttonId];

        for (std::vector<ESM::ENAMstruct>::const_iterator it = mEffects.begin(); it != mEffects.end(); ++it)
        {
            if (it->mEffectID == effectId)
            {
                MWBase::Environment::get().getWindowManager()->messageBox ("#{sOnetypeEffectMessage}", std::vector<std::string>());
                return;
            }
        }

        const ESM::MagicEffect* effect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectId);

        mAddEffectDialog.newEffect (effect);

        if (effect->mData.mFlags & ESM::MagicEffect::TargetSkill)
        {
            delete mSelectSkillDialog;
            mSelectSkillDialog = new SelectSkillDialog(*MWBase::Environment::get().getWindowManager ());
            mSelectSkillDialog->eventCancel += MyGUI::newDelegate(this, &SpellCreationDialog::onAttributeOrSkillCancel);
            mSelectSkillDialog->eventItemSelected += MyGUI::newDelegate(this, &SpellCreationDialog::onSelectSkill);
            mSelectSkillDialog->setVisible (true);
        }
        else if (effect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
        {
            delete mSelectAttributeDialog;
            mSelectAttributeDialog = new SelectAttributeDialog(*MWBase::Environment::get().getWindowManager ());
            mSelectAttributeDialog->eventCancel += MyGUI::newDelegate(this, &SpellCreationDialog::onAttributeOrSkillCancel);
            mSelectAttributeDialog->eventItemSelected += MyGUI::newDelegate(this, &SpellCreationDialog::onSelectAttribute);
            mSelectAttributeDialog->setVisible (true);
        }
        else
        {
            mAddEffectDialog.setVisible(true);
        }
    }

    void EffectEditorBase::onEffectModified (ESM::ENAMstruct effect)
    {
        mEffects[mSelectedEffect] = effect;

        updateEffectsView();
    }

    void EffectEditorBase::onEffectRemoved (ESM::ENAMstruct effect)
    {
        mEffects.erase(mEffects.begin() + mSelectedEffect);
        updateEffectsView();
    }

    void EffectEditorBase::updateEffectsView ()
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
            params.mArea = it->mArea;

            MyGUI::Button* button = mUsedEffectsView->createWidget<MyGUI::Button>("", MyGUI::IntCoord(0, size.height, 0, 24), MyGUI::Align::Default);
            button->setUserData(i);
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellCreationDialog::onEditEffect);
            button->setNeedMouseFocus (true);

            Widgets::MWSpellEffectPtr effect = button->createWidget<Widgets::MWSpellEffect>("MW_EffectImage", MyGUI::IntCoord(0,0,0,24), MyGUI::Align::Default);

            effect->setNeedMouseFocus (false);
            effect->setWindowManager (MWBase::Environment::get().getWindowManager ());
            effect->setSpellEffect (params);

            effect->setSize(effect->getRequestedWidth (), 24);
            button->setSize(effect->getRequestedWidth (), 24);

            size.width = std::max(size.width, effect->getRequestedWidth ());
            size.height += 24;
            ++i;
        }

        mUsedEffectsView->setCanvasSize(size);

        notifyEffectsChanged();
    }

    void EffectEditorBase::onEffectAdded (ESM::ENAMstruct effect)
    {
        mEffects.push_back(effect);

        updateEffectsView();
    }

    void EffectEditorBase::onEditEffect (MyGUI::Widget *sender)
    {
        int id = *sender->getUserData<int>();

        mSelectedEffect = id;

        mAddEffectDialog.editEffect (mEffects[id]);
        mAddEffectDialog.setVisible (true);
    }
}
