#include "spellwindow.hpp"

#include <fmt/format.h>

#include <MyGUI_InputManager.h>

#include <components/settings/settings.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/inventorystore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/spellcasting.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/actorutil.hpp"

#include "spellicons.hpp"
#include "confirmationdialog.hpp"
#include "spellview.hpp"

namespace MWGui
{

    SpellWindow::SpellWindow(DragAndDrop* drag)
        : WindowPinnableBase("openmw_spell_window.layout")
        , NoDrop(drag, mMainWidget)
        , mSpellView(NULL)
        , mUpdateTimer(0.0f)
    {
        mSpellIcons = new SpellIcons();

        getWidget(mSpellView, "SpellView");
        getWidget(mEffectBox, "EffectsBox");

        mSpellView->eventSpellClicked += MyGUI::newDelegate(this, &SpellWindow::onModelIndexSelected);

        setCoord(498, 300, 302, 300);
    }

    SpellWindow::~SpellWindow()
    {
        delete mSpellIcons;
    }

    void SpellWindow::onPinToggled()
    {
        Settings::Manager::setBool("spells pin", "Windows", mPinned);

        MWBase::Environment::get().getWindowManager()->setSpellVisibility(!mPinned);
    }

    void SpellWindow::onTitleDoubleClicked()
    {
        if (!mPinned)
            MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Magic);
    }

    void SpellWindow::onOpen()
    {
        updateSpells();
    }

    void SpellWindow::onFrame(float dt) 
    {
        NoDrop::onFrame(dt);
        mUpdateTimer += dt;
        if (0.5f < mUpdateTimer)
        {
            mUpdateTimer = 0;
            mSpellView->incrementalUpdate();
        }
    }

    void SpellWindow::updateSpells()
    {
        mSpellIcons->updateWidgets(mEffectBox, false);

        mSpellView->setModel(new SpellModel(MWMechanics::getPlayer()));
    }

    void SpellWindow::onEnchantedItemSelected(MWWorld::Ptr item, bool alreadyEquipped)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);

        // retrieve ContainerStoreIterator to the item
        MWWorld::ContainerStoreIterator it = store.begin();
        for (; it != store.end(); ++it)
        {
            if (*it == item)
            {
                break;
            }
        }
        if (it == store.end())
            throw std::runtime_error("can't find selected item");

        // equip, if it can be equipped and is not already equipped
        if (!alreadyEquipped
            && !item.getClass().getEquipmentSlots(item).first.empty())
        {
            MWBase::Environment::get().getWindowManager()->useItem(item);
            // make sure that item was successfully equipped
            if (!store.isEquipped(item))
                return;
        }

        store.setSelectedEnchantItem(it);
        // to reset WindowManager::mSelectedSpell immediately
        MWBase::Environment::get().getWindowManager()->setSelectedEnchantItem(*it);

        updateSpells();
    }

    void SpellWindow::askDeleteSpell(const std::string &spellId)
    {
        // delete spell, if allowed
        const ESM::Spell* spell =
            MWBase::Environment::get().getWorld()->getStore().get<ESM::Spell>().find(spellId);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        std::string raceId = player.get<ESM::NPC>()->mBase->mRace;
        const std::string& signId =
            MWBase::Environment::get().getWorld()->getPlayer().getBirthSign();
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(raceId);
        const ESM::BirthSign* birthsign = MWBase::Environment::get().getWorld()->getStore().get<ESM::BirthSign>().find(signId);

        // can't delete racial spells, birthsign spells or powers 
        if (race->mPowers.exists(spell->mId) || birthsign->mPowers.exists(spell->mId) || spell->mData.mType == ESM::Spell::ST_Power)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sDeleteSpellError}");
        }
        else
        {
            // ask for confirmation
            mSpellToDelete = spellId;
            ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
            std::string question = MWBase::Environment::get().getWindowManager()->getGameSettingString("sQuestionDeleteSpell", "Delete %s?");
            question = fmt::sprintf(question, spell->mName);
            dialog->askForConfirmation(question);
            dialog->eventOkClicked.clear();
            dialog->eventOkClicked += MyGUI::newDelegate(this, &SpellWindow::onDeleteSpellAccept);
            dialog->eventCancelClicked.clear();
        }
    }

    void SpellWindow::onModelIndexSelected(SpellModel::ModelIndex index)
    {
        const Spell& spell = mSpellView->getModel()->getItem(index);
        if (spell.mType == Spell::Type_EnchantedItem)
        {
            onEnchantedItemSelected(spell.mItem, spell.mActive);
        }
        else
        {
            if (MyGUI::InputManager::getInstance().isShiftPressed())
                askDeleteSpell(spell.mId);
            else
                onSpellSelected(spell.mId);
        }
    }

    void SpellWindow::onSpellSelected(const std::string& spellId)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        store.setSelectedEnchantItem(store.end());
        MWBase::Environment::get().getWindowManager()->setSelectedSpell(spellId, int(MWMechanics::getSpellSuccessChance(spellId, player)));

        updateSpells();
    }

    void SpellWindow::onDeleteSpellAccept()
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        MWMechanics::Spells& spells = stats.getSpells();

        if (MWBase::Environment::get().getWindowManager()->getSelectedSpell() == mSpellToDelete)
            MWBase::Environment::get().getWindowManager()->unsetSelectedSpell();

        spells.remove(mSpellToDelete);

        updateSpells();
    }

    void SpellWindow::cycle(bool next)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();

        if (MWBase::Environment::get().getMechanicsManager()->isAttackingOrSpell(player))
            return;

        const MWMechanics::CreatureStats &stats = player.getClass().getCreatureStats(player);
        if (stats.isParalyzed() || stats.getKnockedDown() || stats.isDead() || stats.getHitRecovery())
            return;

        mSpellView->setModel(new SpellModel(MWMechanics::getPlayer()));

        SpellModel::ModelIndex selected = 0;
        for (SpellModel::ModelIndex i = 0; i<int(mSpellView->getModel()->getItemCount()); ++i)
        {
            if (mSpellView->getModel()->getItem(i).mSelected)
                selected = i;
        }

        selected += next ? 1 : -1;
        int itemcount = mSpellView->getModel()->getItemCount();
        if (itemcount == 0)
            return;
        selected = (selected + itemcount) % itemcount;

        const Spell& spell = mSpellView->getModel()->getItem(selected);
        if (spell.mType == Spell::Type_EnchantedItem)
            onEnchantedItemSelected(spell.mItem, spell.mActive);
        else
            onSpellSelected(spell.mId);
    }
}
