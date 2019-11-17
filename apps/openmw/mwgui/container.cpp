#include "container.hpp"

#include <MyGUI_InputManager.h>
#include <MyGUI_Button.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/inventorystore.hpp"

#include "../mwmechanics/creaturestats.hpp"

#include "../mwscript/interpretercontext.hpp"

#include "countdialog.hpp"
#include "inventorywindow.hpp"

#include "itemview.hpp"
#include "inventoryitemmodel.hpp"
#include "containeritemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "pickpocketitemmodel.hpp"
#include "draganddrop.hpp"

namespace MWGui
{

    ContainerWindow::ContainerWindow(DragAndDrop* dragAndDrop)
        : WindowBase("openmw_container_window.layout")
        , mDragAndDrop(dragAndDrop)
        , mSortModel(nullptr)
        , mModel(nullptr)
        , mSelectedItem(-1)
    {
        getWidget(mDisposeCorpseButton, "DisposeCorpseButton");
        getWidget(mTakeButton, "TakeButton");
        getWidget(mCloseButton, "CloseButton");

        getWidget(mItemView, "ItemView");
        mItemView->eventBackgroundClicked += MyGUI::newDelegate(this, &ContainerWindow::onBackgroundSelected);
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &ContainerWindow::onItemSelected);

        mDisposeCorpseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onDisposeCorpseButtonClicked);
        mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onCloseButtonClicked);
        mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onTakeAllButtonClicked);

        setCoord(200,0,600,300);
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
            dialog->openCountDialog(object.getClass().getName(object), "#{sTake}", count);
            dialog->eventOkClicked.clear();
            dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerWindow::dragItem);
        }
        else
            dragItem (nullptr, count);
    }

    void ContainerWindow::dragItem(MyGUI::Widget* sender, int count)
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
        mPtr = container;

        bool loot = mPtr.getClass().isActor() && mPtr.getClass().getCreatureStats(mPtr).isDead();

        if (mPtr.getClass().hasInventoryStore(mPtr))
        {
            if (mPtr.getClass().isNpc() && !loot)
            {
                // we are stealing stuff
                mModel = new PickpocketItemModel(mPtr, new InventoryItemModel(container),
                                                 !mPtr.getClass().getCreatureStats(mPtr).getKnockedDown());
            }
            else
                mModel = new InventoryItemModel(container);
        }
        else
        {
            mModel = new ContainerItemModel(container);
        }

        mDisposeCorpseButton->setVisible(loot);

        mSortModel = new SortFilterItemModel(mModel);

        mItemView->setModel (mSortModel);
        mItemView->resetScrollBars();

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

        setTitle(container.getClass().getName(container));
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

        if (mModel)
            mModel->onClose();

        if (!mPtr.isEmpty())
            MWBase::Environment::get().getMechanicsManager()->onClose(mPtr);
    }

    void ContainerWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::onTakeAllButtonClicked(MyGUI::Widget* _sender)
    {
        if(mDragAndDrop != nullptr && mDragAndDrop->mIsOnDragAndDrop)
            return;

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

        // transfer everything into the player's inventory
        ItemModel* playerModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getModel();
        mModel->update();

        // unequip all items to avoid unequipping/reequipping
        if (mPtr.getClass().hasInventoryStore(mPtr))
        {
            MWWorld::InventoryStore& invStore = mPtr.getClass().getInventoryStore(mPtr);
            for (size_t i=0; i<mModel->getItemCount(); ++i)
            {
                const ItemStack& item = mModel->getItem(i);
                if (invStore.isEquipped(item.mBase) == false)
                    continue;

                invStore.unequipItem(item.mBase, mPtr);
            }
        }

        mModel->update();

        for (size_t i=0; i<mModel->getItemCount(); ++i)
        {
            if (i==0)
            {
                // play the sound of the first object
                MWWorld::Ptr item = mModel->getItem(i).mBase;
                std::string sound = item.getClass().getUpSoundId(item);
                MWBase::Environment::get().getWindowManager()->playSound(sound);
            }

            const ItemStack& item = mModel->getItem(i);

            if (!onTakeItem(item, item.mCount))
                break;

            mModel->moveItem(item, item.mCount, playerModel);
        }

        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    void ContainerWindow::onDisposeCorpseButtonClicked(MyGUI::Widget *sender)
    {
        if(mDragAndDrop == nullptr || !mDragAndDrop->mIsOnDragAndDrop)
        {
            MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

            onTakeAllButtonClicked(mTakeButton);

            if (mPtr.getClass().isPersistent(mPtr))
                MWBase::Environment::get().getWindowManager()->messageBox("#{sDisposeCorpseFail}");
            else
            {
                MWMechanics::CreatureStats& creatureStats = mPtr.getClass().getCreatureStats(mPtr);

                // If we dispose corpse before end of death animation, we should update death counter counter manually.
                // Also we should run actor's script - it may react on actor's death.
                if (creatureStats.isDead() && !creatureStats.isDeathAnimationFinished())
                {
                    creatureStats.setDeathAnimationFinished(true);
                    MWBase::Environment::get().getMechanicsManager()->notifyDied(mPtr);

                    const std::string script = mPtr.getClass().getScript(mPtr);
                    if (!script.empty() && MWBase::Environment::get().getWorld()->getScriptsEnabled())
                    {
                        MWScript::InterpreterContext interpreterContext (&mPtr.getRefData().getLocals(), mPtr);
                        MWBase::Environment::get().getScriptManager()->run (script, interpreterContext);
                    }

                    // Clean up summoned creatures as well
                    std::map<MWMechanics::CreatureStats::SummonKey, int>& creatureMap = creatureStats.getSummonedCreatureMap();
                    for (const auto& creature : creatureMap)
                        MWBase::Environment::get().getMechanicsManager()->cleanupSummonedCreature(mPtr, creature.second);
                    creatureMap.clear();
                }

                MWBase::Environment::get().getWorld()->deleteObject(mPtr);
            }

            mPtr = MWWorld::Ptr();
        }
    }

    void ContainerWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    bool ContainerWindow::onTakeItem(const ItemStack &item, int count)
    {
        return mModel->onTakeItem(item.mBase, count);
    }

}
