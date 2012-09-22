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

    SpellCreationDialog::SpellCreationDialog(MWBase::WindowManager &parWindowManager)
        : WindowBase("openmw_spellcreation_dialog.layout", parWindowManager)
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

}
