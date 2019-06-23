#include "spellcreationdialog.hpp"

#include <MyGUI_ImageBox.h>
#include <MyGUI_Gui.h>

#include <components/esm/records.hpp>
#include <components/widgets/list.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "tooltips.hpp"
#include "class.hpp"
#include "widgets.hpp"

namespace
{

    bool sortMagicEffects (short id1, short id2)
    {
        const MWWorld::Store<ESM::GameSetting> &gmst =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::GameSetting>();

        return gmst.find(ESM::MagicEffect::effectIdToString (id1))->mValue.getString()
                < gmst.find(ESM::MagicEffect::effectIdToString  (id2))->mValue.getString();
    }

    void init(ESM::ENAMstruct& effect)
    {
        effect.mArea = 0;
        effect.mDuration = 0;
        effect.mEffectID = -1;
        effect.mMagnMax = 0;
        effect.mMagnMin = 0;
        effect.mRange = 0;
        effect.mSkill = -1;
        effect.mAttribute = -1;
    }
}

namespace MWGui
{

    EditEffectDialog::EditEffectDialog()
        : WindowModal("openmw_edit_effect.layout")
        , mEditing(false)
        , mMagicEffect(nullptr)
        , mConstantEffect(false)
    {
        init(mEffect);
        init(mOldEffect);

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

    void EditEffectDialog::setConstantEffect(bool constant)
    {
        mConstantEffect = constant;
    }

    void EditEffectDialog::onOpen()
    {
        WindowModal::onOpen();
        center();
    }

    bool EditEffectDialog::exit()
    {
        if(mEditing)
            eventEffectModified(mOldEffect);
        else
            eventEffectRemoved(mEffect);
        return true;
    }

    void EditEffectDialog::newEffect (const ESM::MagicEffect *effect)
    {
        bool allowSelf = (effect->mData.mFlags & ESM::MagicEffect::CastSelf) != 0;
        bool allowTouch = (effect->mData.mFlags & ESM::MagicEffect::CastTouch) && !mConstantEffect;
        bool allowTarget = (effect->mData.mFlags & ESM::MagicEffect::CastTarget) && !mConstantEffect;

        if (!allowSelf && !allowTouch && !allowTarget)
            return; // TODO: Show an error message popup?

        setMagicEffect(effect);
        mEditing = false;

        mDeleteButton->setVisible (false);

        mEffect.mRange = ESM::RT_Self;
        if (!allowSelf)
            mEffect.mRange = ESM::RT_Touch;
        if (!allowTouch)
            mEffect.mRange = ESM::RT_Target;
        mEffect.mMagnMin = 1;
        mEffect.mMagnMax = 1;
        mEffect.mDuration = 1;
        mEffect.mArea = 0;
        mEffect.mSkill = -1;
        mEffect.mAttribute = -1;
        eventEffectAdded(mEffect);

        onRangeButtonClicked(mRangeButton);

        mMagnitudeMinSlider->setScrollPosition (0);
        mMagnitudeMaxSlider->setScrollPosition (0);
        mAreaSlider->setScrollPosition (0);
        mDurationSlider->setScrollPosition (0);

        mDurationValue->setCaption("1");
        mMagnitudeMinValue->setCaption("1");
        const std::string to = MWBase::Environment::get().getWindowManager()->getGameSettingString("sTo", "-");

        mMagnitudeMaxValue->setCaption(to + " 1");
        mAreaValue->setCaption("0");

        setVisible(true);
    }

    void EditEffectDialog::editEffect (ESM::ENAMstruct effect)
    {
        const ESM::MagicEffect* magicEffect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effect.mEffectID);

        setMagicEffect(magicEffect);
        mOldEffect = effect;
        mEffect = effect;
        mEditing = true;

        mDeleteButton->setVisible (true);

        mMagnitudeMinSlider->setScrollPosition (effect.mMagnMin-1);
        mMagnitudeMaxSlider->setScrollPosition (effect.mMagnMax-1);
        mAreaSlider->setScrollPosition (effect.mArea);
        mDurationSlider->setScrollPosition (effect.mDuration-1);

        if (mEffect.mRange == ESM::RT_Self)
            mRangeButton->setCaptionWithReplacing ("#{sRangeSelf}");
        else if (mEffect.mRange == ESM::RT_Target)
            mRangeButton->setCaptionWithReplacing ("#{sRangeTarget}");
        else if (mEffect.mRange == ESM::RT_Touch)
            mRangeButton->setCaptionWithReplacing ("#{sRangeTouch}");

        onMagnitudeMinChanged (mMagnitudeMinSlider, effect.mMagnMin-1);
        onMagnitudeMaxChanged (mMagnitudeMinSlider, effect.mMagnMax-1);
        onAreaChanged (mAreaSlider, effect.mArea);
        onDurationChanged (mDurationSlider, effect.mDuration-1);
        eventEffectModified(mEffect);

        updateBoxes();
    }

    void EditEffectDialog::setMagicEffect (const ESM::MagicEffect *effect)
    {
        mEffectImage->setImageTexture(MWBase::Environment::get().getWindowManager()->correctIconPath(effect->mIcon));

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
        if (!(mMagicEffect->mData.mFlags & ESM::MagicEffect::NoDuration)&&mConstantEffect==false)
        {
            mDurationBox->setPosition(mDurationBox->getPosition().left, curY);
            mDurationBox->setVisible (true);
            curY += mDurationBox->getSize().height;
        }
        if (mEffect.mRange != ESM::RT_Self)
        {
            mAreaBox->setPosition(mAreaBox->getPosition().left, curY);
            mAreaBox->setVisible (true);
            //curY += mAreaBox->getSize().height;
        }
    }

    void EditEffectDialog::onRangeButtonClicked (MyGUI::Widget* sender)
    {
        mEffect.mRange = (mEffect.mRange+1)%3;

        // cycle through range types until we find something that's allowed
        // does not handle the case where nothing is allowed (this should be prevented before opening the Add Effect dialog)
        bool allowSelf = (mMagicEffect->mData.mFlags & ESM::MagicEffect::CastSelf) != 0;
        bool allowTouch = (mMagicEffect->mData.mFlags & ESM::MagicEffect::CastTouch) && !mConstantEffect;
        bool allowTarget = (mMagicEffect->mData.mFlags & ESM::MagicEffect::CastTarget) && !mConstantEffect;
        if (mEffect.mRange == ESM::RT_Self && !allowSelf)
            mEffect.mRange = (mEffect.mRange+1)%3;
        if (mEffect.mRange == ESM::RT_Touch && !allowTouch)
            mEffect.mRange = (mEffect.mRange+1)%3;
        if (mEffect.mRange == ESM::RT_Target && !allowTarget)
            mEffect.mRange = (mEffect.mRange+1)%3;

        if(mEffect.mRange == ESM::RT_Self)
        {
            mAreaSlider->setScrollPosition(0);
            onAreaChanged(mAreaSlider,0);
        }

        if (mEffect.mRange == ESM::RT_Self)
            mRangeButton->setCaptionWithReplacing ("#{sRangeSelf}");
        else if (mEffect.mRange == ESM::RT_Target)
            mRangeButton->setCaptionWithReplacing ("#{sRangeTarget}");
        else if (mEffect.mRange == ESM::RT_Touch)
            mRangeButton->setCaptionWithReplacing ("#{sRangeTouch}");

        updateBoxes();
        eventEffectModified(mEffect);
    }

    void EditEffectDialog::onDeleteButtonClicked (MyGUI::Widget* sender)
    {
        setVisible(false);

        eventEffectRemoved(mEffect);
    }

    void EditEffectDialog::onOkButtonClicked (MyGUI::Widget* sender)
    {
        setVisible(false);
    }

    void EditEffectDialog::onCancelButtonClicked (MyGUI::Widget* sender)
    {
        setVisible(false);
        exit();
    }

    void EditEffectDialog::setSkill (int skill)
    {
        mEffect.mSkill = skill;
        eventEffectModified(mEffect);
    }

    void EditEffectDialog::setAttribute (int attribute)
    {
        mEffect.mAttribute = attribute;
        eventEffectModified(mEffect);
    }

    void EditEffectDialog::onMagnitudeMinChanged (MyGUI::ScrollBar* sender, size_t pos)
    {
        mMagnitudeMinValue->setCaption(MyGUI::utility::toString(pos+1));
        mEffect.mMagnMin = pos+1;

        // trigger the check again (see below)
        onMagnitudeMaxChanged(mMagnitudeMaxSlider, mMagnitudeMaxSlider->getScrollPosition ());
        eventEffectModified(mEffect);
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
        const std::string to = MWBase::Environment::get().getWindowManager()->getGameSettingString("sTo", "-");

        mMagnitudeMaxValue->setCaption(to + " " + MyGUI::utility::toString(pos+1));

        eventEffectModified(mEffect);
    }

    void EditEffectDialog::onDurationChanged (MyGUI::ScrollBar* sender, size_t pos)
    {
        mDurationValue->setCaption(MyGUI::utility::toString(pos+1));
        mEffect.mDuration = pos+1;
        eventEffectModified(mEffect);
    }

    void EditEffectDialog::onAreaChanged (MyGUI::ScrollBar* sender, size_t pos)
    {
        mAreaValue->setCaption(MyGUI::utility::toString(pos));
        mEffect.mArea = pos;
        eventEffectModified(mEffect);
    }

    // ------------------------------------------------------------------------------------------------

    SpellCreationDialog::SpellCreationDialog()
        : WindowBase("openmw_spellcreation_dialog.layout")
        , EffectEditorBase(EffectEditorBase::Spellmaking)
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
        mNameEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &SpellCreationDialog::onAccept);

        setWidgets(mAvailableEffectsList, mUsedEffectsView);
    }

    void SpellCreationDialog::setPtr (const MWWorld::Ptr& actor)
    {
        mPtr = actor;
        mNameEdit->setCaption("");

        startEditing();
    }

    void SpellCreationDialog::onCancelButtonClicked (MyGUI::Widget* sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode (MWGui::GM_SpellCreation);
    }

    void SpellCreationDialog::onBuyButtonClicked (MyGUI::Widget* sender)
    {
        if (mEffects.size() <= 0)
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage30}");
            return;
        }

        if (mNameEdit->getCaption () == "")
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage10}");
            return;
        }

        if (mMagickaCost->getCaption() == "0")
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sEnchantmentMenu8}");
            return;
        }

        MWWorld::Ptr player = MWMechanics::getPlayer();
        int playerGold = player.getClass().getContainerStore(player).count(MWWorld::ContainerStore::sGoldId);

        if (MyGUI::utility::parseInt(mPriceLabel->getCaption()) > playerGold)
        {
            MWBase::Environment::get().getWindowManager()->messageBox ("#{sNotifyMessage18}");
            return;
        }

        mSpell.mName = mNameEdit->getCaption();

        int price = MyGUI::utility::parseInt(mPriceLabel->getCaption());

        player.getClass().getContainerStore(player).remove(MWWorld::ContainerStore::sGoldId, price, player);

        // add gold to NPC trading gold pool
        MWMechanics::CreatureStats& npcStats = mPtr.getClass().getCreatureStats(mPtr);
        npcStats.setGoldPool(npcStats.getGoldPool() + price);

        MWBase::Environment::get().getWindowManager()->playSound ("Mysticism Hit");

        const ESM::Spell* spell = MWBase::Environment::get().getWorld()->createRecord(mSpell);

        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();
        spells.add (spell->mId);

        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_SpellCreation);
    }

    void SpellCreationDialog::onAccept(MyGUI::EditBox *sender)
    {
        onBuyButtonClicked(sender);

        // To do not spam onAccept() again and again
        MWBase::Environment::get().getWindowManager()->injectKeyRelease(MyGUI::KeyCode::None);
    }

    void SpellCreationDialog::onOpen()
    {
        center();
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mNameEdit);
    }

    void SpellCreationDialog::onReferenceUnavailable ()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_Dialogue);
        MWBase::Environment::get().getWindowManager()->removeGuiMode (GM_SpellCreation);
    }

    void SpellCreationDialog::notifyEffectsChanged ()
    {
        if (mEffects.empty())
        {
            mMagickaCost->setCaption("0");
            mPriceLabel->setCaption("0");
            mSuccessChance->setCaption("0");
            return;
        }

        float y = 0;

        const MWWorld::ESMStore &store =
            MWBase::Environment::get().getWorld()->getStore();

        for (const ESM::ENAMstruct& effect : mEffects)
        {
            y += std::max(1.f, MWMechanics::calcEffectCost(effect));

            if (effect.mRange == ESM::RT_Target)
                y *= 1.5;
        }

        ESM::EffectList effectList;
        effectList.mList = mEffects;
        mSpell.mEffects = effectList;
        mSpell.mData.mCost = int(y);
        mSpell.mData.mType = ESM::Spell::ST_Spell;
        mSpell.mData.mFlags = 0;

        mMagickaCost->setCaption(MyGUI::utility::toString(int(y)));

        float fSpellMakingValueMult =
            store.get<ESM::GameSetting>().find("fSpellMakingValueMult")->mValue.getFloat();

        int price = MWBase::Environment::get().getMechanicsManager()->getBarterOffer(mPtr, static_cast<int>(y * fSpellMakingValueMult),true);

        mPriceLabel->setCaption(MyGUI::utility::toString(int(price)));

        float chance = MWMechanics::calcSpellBaseSuccessChance(&mSpell, MWMechanics::getPlayer(), nullptr);

        int intChance = std::min(100, int(chance));
        mSuccessChance->setCaption(MyGUI::utility::toString(intChance));
    }

    // ------------------------------------------------------------------------------------------------


    EffectEditorBase::EffectEditorBase(Type type)
        : mAvailableEffectsList(nullptr)
        , mUsedEffectsView(nullptr)
        , mAddEffectDialog()
        , mSelectAttributeDialog(nullptr)
        , mSelectSkillDialog(nullptr)
        , mSelectedEffect(0)
        , mSelectedKnownEffectId(0)
        , mConstantEffect(false)
        , mType(type)
    {
        mAddEffectDialog.eventEffectAdded += MyGUI::newDelegate(this, &EffectEditorBase::onEffectAdded);
        mAddEffectDialog.eventEffectModified += MyGUI::newDelegate(this, &EffectEditorBase::onEffectModified);
        mAddEffectDialog.eventEffectRemoved += MyGUI::newDelegate(this, &EffectEditorBase::onEffectRemoved);

        mAddEffectDialog.setVisible (false);
    }

    EffectEditorBase::~EffectEditorBase()
    {
    }

    void EffectEditorBase::startEditing ()
    {
        // get the list of magic effects that are known to the player

        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        std::vector<short> knownEffects;

        for (MWMechanics::Spells::TIterator it = spells.begin(); it != spells.end(); ++it)
        {
            const ESM::Spell* spell = it->first;

            // only normal spells count
            if (spell->mData.mType != ESM::Spell::ST_Spell)
                continue;

            for (const ESM::ENAMstruct& effectInfo : spell->mEffects.mList)
            {
                const ESM::MagicEffect * effect = MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(effectInfo.mEffectID);

                // skip effects that do not allow spellmaking/enchanting
                int requiredFlags = (mType == Spellmaking) ? ESM::MagicEffect::AllowSpellmaking : ESM::MagicEffect::AllowEnchanting;
                if (!(effect->mData.mFlags & requiredFlags))
                    continue;

                if (std::find(knownEffects.begin(), knownEffects.end(), effectInfo.mEffectID) == knownEffects.end())
                    knownEffects.push_back(effectInfo.mEffectID);
            }
        }

        std::sort(knownEffects.begin(), knownEffects.end(), sortMagicEffects);

        mAvailableEffectsList->clear ();

        int i=0;
        for (const short effectId : knownEffects)
        {
            mAvailableEffectsList->addItem(MWBase::Environment::get().getWorld ()->getStore ().get<ESM::GameSetting>().find(
                                               ESM::MagicEffect::effectIdToString(effectId))->mValue.getString());
            mButtonMapping[i] = effectId;
            ++i;
        }
        mAvailableEffectsList->adjustSize ();
        mAvailableEffectsList->scrollToTop();

        for (const short effectId : knownEffects)
        {
            std::string name = MWBase::Environment::get().getWorld ()->getStore ().get<ESM::GameSetting>().find(
                                               ESM::MagicEffect::effectIdToString(effectId))->mValue.getString();
            MyGUI::Widget* w = mAvailableEffectsList->getItemWidget(name);

            ToolTips::createMagicEffectToolTip (w, effectId);
        }

        mEffects.clear();
        updateEffectsView ();
    }

    void EffectEditorBase::setWidgets (Gui::MWList *availableEffectsList, MyGUI::ScrollView *usedEffectsView)
    {
        mAvailableEffectsList = availableEffectsList;
        mUsedEffectsView = usedEffectsView;

        mAvailableEffectsList->eventWidgetSelected += MyGUI::newDelegate(this, &EffectEditorBase::onAvailableEffectClicked);
    }

    void EffectEditorBase::onSelectAttribute ()
    {
        const ESM::MagicEffect* effect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(mSelectedKnownEffectId);

        mAddEffectDialog.newEffect(effect);
        mAddEffectDialog.setAttribute (mSelectAttributeDialog->getAttributeId());
        MWBase::Environment::get().getWindowManager ()->removeDialog (mSelectAttributeDialog);
        mSelectAttributeDialog = 0;
    }

    void EffectEditorBase::onSelectSkill ()
    {
        const ESM::MagicEffect* effect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(mSelectedKnownEffectId);

        mAddEffectDialog.newEffect(effect);
        mAddEffectDialog.setSkill (mSelectSkillDialog->getSkillId());
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
            MWBase::Environment::get().getWindowManager()->messageBox("#{sNotifyMessage28}");
            return;
        }

        int buttonId = *sender->getUserData<int>();
        mSelectedKnownEffectId = mButtonMapping[buttonId];

        const ESM::MagicEffect* effect =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::MagicEffect>().find(mSelectedKnownEffectId);

        if (effect->mData.mFlags & ESM::MagicEffect::TargetSkill)
        {
            delete mSelectSkillDialog;
            mSelectSkillDialog = new SelectSkillDialog();
            mSelectSkillDialog->eventCancel += MyGUI::newDelegate(this, &SpellCreationDialog::onAttributeOrSkillCancel);
            mSelectSkillDialog->eventItemSelected += MyGUI::newDelegate(this, &SpellCreationDialog::onSelectSkill);
            mSelectSkillDialog->setVisible (true);
        }
        else if (effect->mData.mFlags & ESM::MagicEffect::TargetAttribute)
        {
            delete mSelectAttributeDialog;
            mSelectAttributeDialog = new SelectAttributeDialog();
            mSelectAttributeDialog->eventCancel += MyGUI::newDelegate(this, &SpellCreationDialog::onAttributeOrSkillCancel);
            mSelectAttributeDialog->eventItemSelected += MyGUI::newDelegate(this, &SpellCreationDialog::onSelectAttribute);
            mSelectAttributeDialog->setVisible (true);
        }
        else
        {
            for (const ESM::ENAMstruct& effectInfo : mEffects)
            {
                if (effectInfo.mEffectID == mSelectedKnownEffectId)
                {
                    MWBase::Environment::get().getWindowManager()->messageBox ("#{sOnetypeEffectMessage}");
                    return;
                }
            }

            mAddEffectDialog.newEffect(effect);
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
        for (const ESM::ENAMstruct& effectInfo : mEffects)
        {
            Widgets::SpellEffectParams params;
            params.mEffectID = effectInfo.mEffectID;
            params.mSkill = effectInfo.mSkill;
            params.mAttribute = effectInfo.mAttribute;
            params.mDuration = effectInfo.mDuration;
            params.mMagnMin = effectInfo.mMagnMin;
            params.mMagnMax = effectInfo.mMagnMax;
            params.mRange = effectInfo.mRange;
            params.mArea = effectInfo.mArea;
            params.mIsConstant = mConstantEffect;

            MyGUI::Button* button = mUsedEffectsView->createWidget<MyGUI::Button>("", MyGUI::IntCoord(0, size.height, 0, 24), MyGUI::Align::Default);
            button->setUserData(i);
            button->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellCreationDialog::onEditEffect);
            button->setNeedMouseFocus (true);

            Widgets::MWSpellEffectPtr effect = button->createWidget<Widgets::MWSpellEffect>("MW_EffectImage", MyGUI::IntCoord(0,0,0,24), MyGUI::Align::Default);

            effect->setNeedMouseFocus (false);
            effect->setSpellEffect (params);

            effect->setSize(effect->getRequestedWidth (), 24);
            button->setSize(effect->getRequestedWidth (), 24);

            size.width = std::max(size.width, effect->getRequestedWidth ());
            size.height += 24;
            ++i;
        }

        // Canvas size must be expressed with HScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
        mUsedEffectsView->setVisibleHScroll(false);
        mUsedEffectsView->setCanvasSize(size);
        mUsedEffectsView->setVisibleHScroll(true);

        notifyEffectsChanged();
    }

    void EffectEditorBase::onEffectAdded (ESM::ENAMstruct effect)
    {
        mEffects.push_back(effect);
        mSelectedEffect=mEffects.size()-1;

        updateEffectsView();
    }

    void EffectEditorBase::onEditEffect (MyGUI::Widget *sender)
    {
        int id = *sender->getUserData<int>();

        mSelectedEffect = id;

        mAddEffectDialog.editEffect (mEffects[id]);
        mAddEffectDialog.setVisible (true);
    }

    void EffectEditorBase::setConstantEffect(bool constant)
    {
        mAddEffectDialog.setConstantEffect(constant);
        mConstantEffect = constant;
    }
}
