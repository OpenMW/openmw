#include "spellwindow.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_Window.h>

#include <components/misc/stringops.hpp>
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
        , mSpellView(nullptr)
        , mUpdateTimer(0.0f)
    {
        mSpellIcons = new SpellIcons();

        MyGUI::Widget* deleteButton;
        getWidget(deleteButton, "DeleteSpellButton");

        getWidget(mSpellView, "SpellView");
        getWidget(mEffectBox, "EffectsBox");
        getWidget(mFilterEdit, "FilterEdit");

        mSpellView->eventSpellClicked += MyGUI::newDelegate(this, &SpellWindow::onModelIndexSelected);
        mFilterEdit->eventEditTextChange += MyGUI::newDelegate(this, &SpellWindow::onFilterChanged);
        deleteButton->eventMouseButtonClick += MyGUI::newDelegate(this, &SpellWindow::onDeleteClicked);

        setCoord(498, 300, 302, 300);

        // Adjust the spell filtering widget size because of MyGUI limitations.
        int filterWidth = mSpellView->getSize().width - deleteButton->getSize().width - 3;
        mFilterEdit->setSize(filterWidth, mFilterEdit->getSize().height);
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
        if (MyGUI::InputManager::getInstance().isShiftPressed())
            MWBase::Environment::get().getWindowManager()->toggleMaximized(this);
        else if (!mPinned)
            MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Magic);
    }

    void SpellWindow::onOpen()
    {
        // Reset the filter focus when opening the window
        MyGUI::Widget* focus = MyGUI::InputManager::getInstance().getKeyFocusWidget();
        if (focus == mFilterEdit)
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(nullptr);

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

        // Update effects in-game too if the window is pinned
        if (mPinned && !MWBase::Environment::get().getWindowManager()->isGuiMode())
            mSpellIcons->updateWidgets(mEffectBox, false);
    }

    void SpellWindow::updateSpells()
    {
        mSpellIcons->updateWidgets(mEffectBox, false);

        mSpellView->setModel(new SpellModel(MWMechanics::getPlayer(), mFilterEdit->getCaption()));
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
        const ESM::Race* race = MWBase::Environment::get().getWorld()->getStore().get<ESM::Race>().find(raceId);
        // can't delete racial spells, birthsign spells or powers
        bool isInherent = race->mPowers.exists(spell->mId) || spell->mData.mType == ESM::Spell::ST_Power;
        const std::string& signId = MWBase::Environment::get().getWorld()->getPlayer().getBirthSign();
        if (!isInherent && !signId.empty())
        {
            const ESM::BirthSign* sign = MWBase::Environment::get().getWorld()->getStore().get<ESM::BirthSign>().find(signId);
            isInherent = sign->mPowers.exists(spell->mId);
        }

        if (isInherent)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sDeleteSpellError}");
        }
        else
        {
            // ask for confirmation
            mSpellToDelete = spellId;
            ConfirmationDialog* dialog = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
            std::string question = MWBase::Environment::get().getWindowManager()->getGameSettingString("sQuestionDeleteSpell", "Delete %s?");
            question = Misc::StringUtils::format(question, spell->mName);
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

    void SpellWindow::onFilterChanged(MyGUI::EditBox *sender)
    {
        mSpellView->setModel(new SpellModel(MWMechanics::getPlayer(), sender->getCaption()));
    }

    void SpellWindow::onDeleteClicked(MyGUI::Widget *widget)
    {
        SpellModel::ModelIndex selected = mSpellView->getModel()->getSelectedIndex();
        if (selected < 0)
            return;

        const Spell& spell = mSpellView->getModel()->getItem(selected);
        if (spell.mType != Spell::Type_EnchantedItem)
            askDeleteSpell(spell.mId);
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

        mSpellView->setModel(new SpellModel(MWMechanics::getPlayer(), ""));

        SpellModel::ModelIndex selected = mSpellView->getModel()->getSelectedIndex();
        if (selected < 0)
            selected = 0;

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
