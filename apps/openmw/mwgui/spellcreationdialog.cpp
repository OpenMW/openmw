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

namespace
{

    bool sortMagicEffects (short id1, short id2)
    {
        return MWBase::Environment::get().getWorld ()->getStore ().gameSettings.find(MWGui::Widgets::MWSpellEffect::effectIDToString (id1))->getString()
                < MWBase::Environment::get().getWorld ()->getStore ().gameSettings.find(MWGui::Widgets::MWSpellEffect::effectIDToString (id2))->getString();
    }
}

namespace MWGui
{

    EditEffectDialog::EditEffectDialog(MWBase::WindowManager &parWindowManager)
        : WindowModal("openmw_edit_effect.layout", parWindowManager)
        , mRange(ESM::RT_Touch)
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
    }

    void EditEffectDialog::setEffect (const ESM::MagicEffect *effect)
    {
        std::string icon = effect->icon;
        icon[icon.size()-3] = 'd';
        icon[icon.size()-2] = 'd';
        icon[icon.size()-1] = 's';
        icon = "icons\\" + icon;

        mEffectImage->setImageTexture (icon);

        mEffectName->setCaptionWithReplacing("#{"+Widgets::MWSpellEffect::effectIDToString (effect->index)+"}");
    }

    void EditEffectDialog::onRangeButtonClicked (MyGUI::Widget* sender)
    {
        mRange = (mRange+1)%3;

        if (mRange == ESM::RT_Self)
            mRangeButton->setCaptionWithReplacing ("#{sRangeSelf}");
        else if (mRange == ESM::RT_Target)
            mRangeButton->setCaptionWithReplacing ("#{sRangeTarget}");
        else if (mRange == ESM::RT_Touch)
            mRangeButton->setCaptionWithReplacing ("#{sRangeTouch}");

        mAreaSlider->setVisible (mRange != ESM::RT_Self);
        mAreaText->setVisible (mRange != ESM::RT_Self);
    }

    void EditEffectDialog::onDeleteButtonClicked (MyGUI::Widget* sender)
    {

    }

    void EditEffectDialog::onOkButtonClicked (MyGUI::Widget* sender)
    {
        setVisible(false);
    }

    void EditEffectDialog::onCancelButtonClicked (MyGUI::Widget* sender)
    {
        setVisible(false);
    }

    // ------------------------------------------------------------------------------------------------

    SpellCreationDialog::SpellCreationDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_spellcreation_dialog.layout", parWindowManager)
        , mAddEffectDialog(parWindowManager)
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
            if (spell->data.type != ESM::Spell::ST_Spell)
                continue;

            const std::vector<ESM::ENAMstruct>& list = spell->effects.list;
            for (std::vector<ESM::ENAMstruct>::const_iterator it2 = list.begin(); it2 != list.end(); ++it2)
            {
                if (std::find(knownEffects.begin(), knownEffects.end(), it2->effectID) == knownEffects.end())
                    knownEffects.push_back(it2->effectID);
            }
        }

        std::sort(knownEffects.begin(), knownEffects.end(), sortMagicEffects);

        mAvailableEffectsList->clear ();

        for (std::vector<short>::const_iterator it = knownEffects.begin(); it != knownEffects.end(); ++it)
        {
            mAvailableEffectsList->addItem(MWBase::Environment::get().getWorld ()->getStore ().gameSettings.find(
                                               MWGui::Widgets::MWSpellEffect::effectIDToString (*it))->getString());
        }
        mAvailableEffectsList->adjustSize ();

        for (std::vector<short>::const_iterator it = knownEffects.begin(); it != knownEffects.end(); ++it)
        {
            std::string name = MWBase::Environment::get().getWorld ()->getStore ().gameSettings.find(
                                               MWGui::Widgets::MWSpellEffect::effectIDToString (*it))->getString();
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

    void SpellCreationDialog::onAvailableEffectClicked (MyGUI::Widget* sender)
    {
        mAddEffectDialog.setVisible(true);

        short effectId = *sender->getUserData<short>();
        const ESM::MagicEffect* effect = MWBase::Environment::get().getWorld()->getStore().magicEffects.find(effectId);
        mAddEffectDialog.setEffect (effect);
    }

}
