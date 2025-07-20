#include "spellwindow.hpp"

#include <MyGUI_EditBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Window.h>

#include <components/esm3/loadbsgn.hpp>
#include <components/esm3/loadrace.hpp>
#include <components/misc/strings/format.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/datetimemanager.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/inventorystore.hpp"
#include "../mwworld/player.hpp"

#include "../mwmechanics/actorutil.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/spells.hpp"
#include "../mwmechanics/spellutil.hpp"

#include "confirmationdialog.hpp"
#include "spellicons.hpp"
#include "spellview.hpp"
#include "statswindow.hpp"

namespace MWGui
{

    SpellWindow::SpellWindow(DragAndDrop* drag)
        : WindowPinnableBase("openmw_spell_window.layout")
        , NoDrop(drag, mMainWidget)
        , mSpellView(nullptr)
        , mUpdateTimer(0.0f)
    {
        mSpellIcons = std::make_unique<SpellIcons>();

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

        if (Settings::gui().mControllerMenus)
        {
            setPinButtonVisible(false);
            mControllerButtons.a = "#{sSelect}";
            mControllerButtons.b = "#{sBack}";
            mControllerButtons.r3 = "#{sInfo}";
        }
    }

    void SpellWindow::onPinToggled()
    {
        Settings::windows().mSpellsPin.set(mPinned);

        MWBase::Environment::get().getWindowManager()->setSpellVisibility(!mPinned);
    }

    void SpellWindow::onTitleDoubleClicked()
    {
        if (Settings::gui().mControllerMenus)
            return;
        else if (MyGUI::InputManager::getInstance().isShiftPressed())
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

        // Update effects if the time is unpaused for any reason (e.g. the window is pinned)
        if (!MWBase::Environment::get().getWorld()->getTimeManager()->isPaused())
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
        if (!alreadyEquipped && !item.getClass().getEquipmentSlots(item).first.empty())
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

    void SpellWindow::askDeleteSpell(const ESM::RefId& spellId)
    {
        // delete spell, if allowed
        const ESM::Spell* spell = MWBase::Environment::get().getESMStore()->get<ESM::Spell>().find(spellId);

        MWWorld::Ptr player = MWMechanics::getPlayer();
        const ESM::RefId& raceId = player.get<ESM::NPC>()->mBase->mRace;
        const ESM::Race* race = MWBase::Environment::get().getESMStore()->get<ESM::Race>().find(raceId);
        // can't delete racial spells, birthsign spells or powers
        bool isInherent = race->mPowers.exists(spell->mId) || spell->mData.mType == ESM::Spell::ST_Power;
        const ESM::RefId& signId = MWBase::Environment::get().getWorld()->getPlayer().getBirthSign();
        if (!isInherent && !signId.empty())
        {
            const ESM::BirthSign* sign = MWBase::Environment::get().getESMStore()->get<ESM::BirthSign>().find(signId);
            isInherent = sign->mPowers.exists(spell->mId);
        }

        const auto windowManager = MWBase::Environment::get().getWindowManager();
        if (isInherent)
        {
            windowManager->messageBox("#{sDeleteSpellError}");
        }
        else
        {
            // ask for confirmation
            mSpellToDelete = spellId;
            ConfirmationDialog* dialog = windowManager->getConfirmationDialog();
            std::string question{ windowManager->getGameSettingString("sQuestionDeleteSpell", "Delete %s?") };
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

    void SpellWindow::onFilterChanged(MyGUI::EditBox* sender)
    {
        mSpellView->setModel(new SpellModel(MWMechanics::getPlayer(), sender->getCaption()));
    }

    void SpellWindow::onDeleteClicked(MyGUI::Widget* widget)
    {
        SpellModel::ModelIndex selected = mSpellView->getModel()->getSelectedIndex();
        if (selected < 0)
            return;

        const Spell& spell = mSpellView->getModel()->getItem(selected);
        if (spell.mType != Spell::Type_EnchantedItem)
            askDeleteSpell(spell.mId);
    }

    void SpellWindow::onSpellSelected(const ESM::RefId& spellId)
    {
        MWWorld::Ptr player = MWMechanics::getPlayer();
        MWWorld::InventoryStore& store = player.getClass().getInventoryStore(player);
        store.setSelectedEnchantItem(store.end());
        MWBase::Environment::get().getWindowManager()->setSelectedSpell(
            spellId, int(MWMechanics::getSpellSuccessChance(spellId, player)));

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

        const MWMechanics::CreatureStats& stats = player.getClass().getCreatureStats(player);
        if (stats.isParalyzed() || stats.getKnockedDown() || stats.isDead() || stats.getHitRecovery())
            return;

        mSpellView->setModel(new SpellModel(MWMechanics::getPlayer()));
        int itemCount = mSpellView->getModel()->getItemCount();
        if (itemCount == 0)
            return;

        SpellModel::ModelIndex nextIndex;
        SpellModel::ModelIndex currentIndex = mSpellView->getModel()->getSelectedIndex();

        // If we have a selected index, search for a valid selection in the target direction
        if (currentIndex >= 0)
        {
            MWWorld::ContainerStore store;
            const Spell& currentSpell = mSpellView->getModel()->getItem(currentIndex);

            nextIndex = currentIndex;
            for (int i = 0; i < itemCount; i++)
            {
                nextIndex += next ? 1 : -1;
                nextIndex = (nextIndex + itemCount) % itemCount;

                // We can keep this selection if:
                //   * we're not switching off of an enchanted item
                //   * we're not switching to an enchanted item
                //   * the next item wouldn't stack with the current item
                if (currentSpell.mType != Spell::Type_EnchantedItem)
                    break;

                const Spell& nextSpell = mSpellView->getModel()->getItem(nextIndex);
                if (nextSpell.mType != Spell::Type_EnchantedItem || !store.stacks(currentSpell.mItem, nextSpell.mItem))
                    break;
            }
        }
        // Otherwise, the first selection is always index 0
        else
            nextIndex = 0;

        // Only trigger the selection event if the selection is actually changing.
        // The itemCount check earlier ensures we have at least one spell to select.
        if (nextIndex != currentIndex)
        {
            const Spell& selectedSpell = mSpellView->getModel()->getItem(nextIndex);
            if (selectedSpell.mType == Spell::Type_EnchantedItem)
                onEnchantedItemSelected(selectedSpell.mItem, selectedSpell.mActive);
            else
                onSpellSelected(selectedSpell.mId);
        }
    }

    bool SpellWindow::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (arg.button == SDL_CONTROLLER_BUTTON_B)
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
        else
            mSpellView->onControllerButton(arg.button);

        return true;
    }

    void SpellWindow::setActiveControllerWindow(bool active)
    {
        MWBase::WindowManager* winMgr = MWBase::Environment::get().getWindowManager();
        if (winMgr->getMode() == MWGui::GM_Inventory)
        {
            // Fill the screen, or limit to a certain size on large screens. Size chosen to
            // match the size of the stats window.
            MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
            int width = std::min(viewSize.width, StatsWindow::getIdealWidth());
            int height = std::min(winMgr->getControllerMenuHeight(), StatsWindow::getIdealHeight());
            int x = (viewSize.width - width) / 2;
            int y = (viewSize.height - height) / 2;

            MyGUI::Window* window = mMainWidget->castType<MyGUI::Window>();
            window->setCoord(x, active ? y : viewSize.height + 1, width, height);

            MWBase::Environment::get().getWindowManager()->setControllerTooltip(
                active && Settings::gui().mControllerTooltips);
        }

        mSpellView->setActiveControllerWindow(active);

        WindowBase::setActiveControllerWindow(active);
    }
}
