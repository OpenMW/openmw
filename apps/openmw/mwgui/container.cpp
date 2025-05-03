#include "container.hpp"

#include <MyGUI_Button.h>
#include <MyGUI_InputManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/aipackage.hpp"
#include "../mwmechanics/creaturestats.hpp"
#include "../mwmechanics/summoning.hpp"

#include "../mwscript/interpretercontext.hpp"

#include "countdialog.hpp"
#include "inventorywindow.hpp"

#include "containeritemmodel.hpp"
#include "draganddrop.hpp"
#include "inventoryitemmodel.hpp"
#include "itemview.hpp"
#include "pickpocketitemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "tooltips.hpp"

namespace MWGui
{

    ContainerWindow::ContainerWindow(DragAndDrop* dragAndDrop)
        : WindowBase("openmw_container_window.layout")
        , mDragAndDrop(dragAndDrop)
        , mSortModel(nullptr)
        , mModel(nullptr)
        , mSelectedItem(-1)
        , mUpdateNextFrame(false)
        , mTreatNextOpenAsLoot(false)
    {
        getWidget(mDisposeCorpseButton, "DisposeCorpseButton");
        getWidget(mTakeButton, "TakeButton");
        getWidget(mCloseButton, "CloseButton");

        getWidget(mItemView, "ItemView");
        mItemView->eventBackgroundClicked += MyGUI::newDelegate(this, &ContainerWindow::onBackgroundSelected);
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &ContainerWindow::onItemSelected);

        mDisposeCorpseButton->eventMouseButtonClick
            += MyGUI::newDelegate(this, &ContainerWindow::onDisposeCorpseButtonClicked);
        mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onCloseButtonClicked);
        mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onTakeAllButtonClicked);

        setCoord(200, 0, 600, 300);
    }

    void ContainerWindow::onItemSelected(int index)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            dropItem();
            return;
        }

        const ItemStack& item = mSortModel->getItem(index);

        // We can't take a conjured item from a container (some NPC we're pickpocketing, a box, etc)
        if (item.mFlags & ItemStack::Flag_Bound)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sContentsMessage1}");
            return;
        }

        MWWorld::Ptr object = item.mBase;
        int count = item.mCount;
        bool shift = MyGUI::InputManager::getInstance().isShiftPressed();
        if (MyGUI::InputManager::getInstance().isControlPressed())
            count = 1;

        mSelectedItem = mSortModel->mapToSource(index);

        if (count > 1 && !shift)
        {
            CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
            std::string name{ object.getClass().getName(object) };
            name += MWGui::ToolTips::getSoulString(object.getCellRef());
            dialog->openCountDialog(name, "#{sTake}", count);
            dialog->eventOkClicked.clear();
            dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerWindow::dragItem);
        }
        else
            dragItem(nullptr, count);
    }

    void ContainerWindow::dragItem(MyGUI::Widget* /*sender*/, std::size_t count)
    {
        if (!mModel)
            return;

        if (!onTakeItem(mModel->getItem(mSelectedItem), count))
            return;

        mDragAndDrop->startDrag(mSelectedItem, mSortModel, mModel, mItemView, count);
    }

    void ContainerWindow::dropItem()
    {
        if (!mModel)
            return;

        bool success = mModel->onDropItem(mDragAndDrop->mItem.mBase, mDragAndDrop->mDraggedCount);

        if (success)
            mDragAndDrop->drop(mModel, mItemView);
    }

    void ContainerWindow::onBackgroundSelected()
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
            dropItem();
    }

    void ContainerWindow::setPtr(const MWWorld::Ptr& container)
    {
        if (container.isEmpty() || (container.getType() != ESM::REC_CONT && !container.getClass().isActor()))
            throw std::runtime_error("Invalid argument in ContainerWindow::setPtr");
        bool lootAnyway = mTreatNextOpenAsLoot;
        mTreatNextOpenAsLoot = false;
        mPtr = container;

        bool loot = mPtr.getClass().isActor() && mPtr.getClass().getCreatureStats(mPtr).isDead();

        std::unique_ptr<ItemModel> model;
        if (mPtr.getClass().hasInventoryStore(mPtr))
        {
            if (mPtr.getClass().isNpc() && !loot && !lootAnyway)
            {
                // we are stealing stuff
                model = std::make_unique<PickpocketItemModel>(mPtr, std::make_unique<InventoryItemModel>(container),
                    !mPtr.getClass().getCreatureStats(mPtr).getKnockedDown());
            }
            else
                model = std::make_unique<InventoryItemModel>(container);
        }
        else
        {
            model = std::make_unique<ContainerItemModel>(container);
        }

        mDisposeCorpseButton->setVisible(loot);
        mModel = model.get();
        auto sortModel = std::make_unique<SortFilterItemModel>(std::move(model));
        mSortModel = sortModel.get();

        mItemView->setModel(std::move(sortModel));
        mItemView->resetScrollBars();

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

        setTitle(container.getClass().getName(container));

        mPtr.getClass().getContainerStore(mPtr).setContListener(this);
    }

    void ContainerWindow::resetReference()
    {
        ReferenceInterface::resetReference();
        mItemView->setModel(nullptr);
        mModel = nullptr;
        mSortModel = nullptr;
    }

    void ContainerWindow::onClose()
    {
        WindowBase::onClose();

        // Make sure the window was actually closed and not temporarily hidden.
        if (MWBase::Environment::get().getWindowManager()->containsMode(GM_Container))
            return;

        if (mModel)
            mModel->onClose();

        if (!mPtr.isEmpty())
            MWBase::Environment::get().getMechanicsManager()->onClose(mPtr);
        resetReference();
    }

    void ContainerWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::onTakeAllButtonClicked(MyGUI::Widget* _sender)
    {
        if (!mModel)
            return;
        if (mDragAndDrop != nullptr && mDragAndDrop->mIsOnDragAndDrop)
            return;

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

        // transfer everything into the player's inventory
        ItemModel* playerModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getModel();
        assert(mModel);
        mModel->update();

        // unequip all items to avoid unequipping/reequipping
        if (mPtr.getClass().hasInventoryStore(mPtr))
        {
            MWWorld::InventoryStore& invStore = mPtr.getClass().getInventoryStore(mPtr);
            for (size_t i = 0; i < mModel->getItemCount(); ++i)
            {
                const ItemStack& item = mModel->getItem(i);
                if (invStore.isEquipped(item.mBase) == false)
                    continue;

                invStore.unequipItem(item.mBase);
            }
        }

        mModel->update();

        for (size_t i = 0; i < mModel->getItemCount(); ++i)
        {
            if (i == 0)
            {
                // play the sound of the first object
                MWWorld::Ptr item = mModel->getItem(i).mBase;
                const ESM::RefId& sound = item.getClass().getUpSoundId(item);
                MWBase::Environment::get().getWindowManager()->playSound(sound);
            }

            const ItemStack& item = mModel->getItem(i);

            if (!onTakeItem(item, item.mCount))
                break;

            mModel->moveItem(item, item.mCount, playerModel);
        }

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::onDisposeCorpseButtonClicked(MyGUI::Widget* sender)
    {
        if (mDragAndDrop == nullptr || !mDragAndDrop->mIsOnDragAndDrop)
        {
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

            // Copy mPtr because onTakeAllButtonClicked closes the window which resets the reference
            MWWorld::Ptr ptr = mPtr;
            onTakeAllButtonClicked(mTakeButton);

            if (ptr.getClass().isPersistent(ptr))
                MWBase::Environment::get().getWindowManager()->messageBox("#{sDisposeCorpseFail}");
            else
            {
                MWMechanics::CreatureStats& creatureStats = ptr.getClass().getCreatureStats(ptr);

                // If we dispose corpse before end of death animation, we should update death counter counter manually.
                // Also we should run actor's script - it may react on actor's death.
                if (creatureStats.isDead() && !creatureStats.isDeathAnimationFinished())
                {
                    creatureStats.setDeathAnimationFinished(true);
                    MWBase::Environment::get().getMechanicsManager()->notifyDied(ptr);

                    const ESM::RefId& script = ptr.getClass().getScript(ptr);
                    if (!script.empty() && MWBase::Environment::get().getWorld()->getScriptsEnabled())
                    {
                        MWScript::InterpreterContext interpreterContext(&ptr.getRefData().getLocals(), ptr);
                        MWBase::Environment::get().getScriptManager()->run(script, interpreterContext);
                    }

                    // Clean up summoned creatures as well
                    auto& creatureMap = creatureStats.getSummonedCreatureMap();
                    for (const auto& creature : creatureMap)
                        MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(ptr, creature.second);
                    creatureMap.clear();

                    // Check if we are a summon and inform our master we've bit the dust
                    for (const auto& package : creatureStats.getAiSequence())
                    {
                        if (package->followTargetThroughDoors() && !package->getTarget().isEmpty())
                        {
                            const auto& summoner = package->getTarget();
                            auto& summons = summoner.getClass().getCreatureStats(summoner).getSummonedCreatureMap();
                            auto it = std::find_if(summons.begin(), summons.end(),
                                [&](const auto& entry) { return entry.second == creatureStats.getActorId(); });
                            if (it != summons.end())
                            {
                                auto summon = *it;
                                summons.erase(it);
                                MWMechanics::purgeSummonEffect(summoner, summon);
                                break;
                            }
                        }
                    }
                }

                MWBase::Environment::get().getWorld()->deleteObject(ptr);
            }

            mPtr = MWWorld::Ptr();
        }
    }

    void ContainerWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    bool ContainerWindow::onTakeItem(const ItemStack& item, int count)
    {
        return mModel->onTakeItem(item.mBase, count);
    }

    void ContainerWindow::onDeleteCustomData(const MWWorld::Ptr& ptr)
    {
        if (mModel && mModel->usesContainer(ptr))
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::onFrame(float dt)
    {
        checkReferenceAvailable();

        if (mUpdateNextFrame)
        {
            mItemView->update();
            mUpdateNextFrame = false;
        }
    }

    void ContainerWindow::itemAdded(const MWWorld::ConstPtr& item, int count)
    {
        mUpdateNextFrame = true;
    }

    void ContainerWindow::itemRemoved(const MWWorld::ConstPtr& item, int count)
    {
        mUpdateNextFrame = true;
    }
}
