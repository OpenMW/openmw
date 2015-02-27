#include "container.hpp"

#include <MyGUI_InputManager.h>
#include <MyGUI_Button.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/containerstore.hpp"

#include "../mwmechanics/pickpocket.hpp"
#include "../mwmechanics/creaturestats.hpp"

#include "countdialog.hpp"
#include "tradewindow.hpp"
#include "inventorywindow.hpp"

#include "itemview.hpp"
#include "itemwidget.hpp"
#include "inventoryitemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "pickpocketitemmodel.hpp"
#include "draganddrop.hpp"

namespace MWGui
{

    ContainerWindow::ContainerWindow(DragAndDrop* dragAndDrop)
        : WindowBase("openmw_container_window.layout")
        , mDragAndDrop(dragAndDrop)
        , mSelectedItem(-1)
        , mModel(NULL)
        , mSortModel(NULL)
        , mPickpocketDetected(false)
    {
        getWidget(mDisposeCorpseButton, "DisposeCorpseButton");
        getWidget(mTakeButton, "TakeButton");
        getWidget(mCloseButton, "CloseButton");

        getWidget(mItemView, "ItemView");
        mItemView->eventBackgroundClicked += MyGUI::newDelegate(this, &ContainerWindow::onBackgroundSelected);
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &ContainerWindow::onItemSelected);

        mDisposeCorpseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onDisposeCorpseButtonClicked);
        mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onCloseButtonClicked);
        mCloseButton->eventKeyButtonPressed += MyGUI::newDelegate(this, &ContainerWindow::onKeyPressed);
        mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onTakeAllButtonClicked);

        setCoord(200,0,600,300);
    }

    void ContainerWindow::onItemSelected(int index)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            if (!dynamic_cast<PickpocketItemModel*>(mModel))
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
            dialog->open(object.getClass().getName(object), "#{sTake}", count);
            dialog->eventOkClicked.clear();
            dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerWindow::dragItem);
        }
        else
            dragItem (NULL, count);
    }

    void ContainerWindow::dragItem(MyGUI::Widget* sender, int count)
    {
        if (!onTakeItem(mModel->getItem(mSelectedItem), count))
            return;

        mDragAndDrop->startDrag(mSelectedItem, mSortModel, mModel, mItemView, count);
    }

    void ContainerWindow::dropItem()
    {
        if (mPtr.getTypeName() == typeid(ESM::Container).name())
        {
            // check container organic flag
            MWWorld::LiveCellRef<ESM::Container>* ref = mPtr.get<ESM::Container>();
            if (ref->mBase->mFlags & ESM::Container::Organic)
            {
                MWBase::Environment::get().getWindowManager()->
                    messageBox("#{sContentsMessage2}");
                return;
            }

            // check that we don't exceed container capacity
            MWWorld::Ptr item = mDragAndDrop->mItem.mBase;
            float weight = item.getClass().getWeight(item) * mDragAndDrop->mDraggedCount;
            if (mPtr.getClass().getCapacity(mPtr) < mPtr.getClass().getEncumbrance(mPtr) + weight)
            {
                MWBase::Environment::get().getWindowManager()->messageBox("#{sContentsMessage3}");
                return;
            }
        }

        mDragAndDrop->drop(mModel, mItemView);
    }

    void ContainerWindow::onBackgroundSelected()
    {
        if (mDragAndDrop->mIsOnDragAndDrop && !dynamic_cast<PickpocketItemModel*>(mModel))
            dropItem();
    }

    void ContainerWindow::open(const MWWorld::Ptr& container, bool loot)
    {
        mPickpocketDetected = false;
        mPtr = container;

        if (mPtr.getTypeName() == typeid(ESM::NPC).name() && !loot)
        {
            // we are stealing stuff
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            mModel = new PickpocketItemModel(player, new InventoryItemModel(container),
                                             !mPtr.getClass().getCreatureStats(mPtr).getKnockedDown());
        }
        else
            mModel = new InventoryItemModel(container);

        mDisposeCorpseButton->setVisible(loot);

        mSortModel = new SortFilterItemModel(mModel);

        mItemView->setModel (mSortModel);

        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mCloseButton);

        setTitle(container.getClass().getName(container));
    }

    void ContainerWindow::onKeyPressed(MyGUI::Widget *_sender, MyGUI::KeyCode _key, MyGUI::Char _char)
    {
        if (_key == MyGUI::KeyCode::Space)
            onCloseButtonClicked(mCloseButton);
        if (_key == MyGUI::KeyCode::Return || _key == MyGUI::KeyCode::NumpadEnter)
            onTakeAllButtonClicked(mTakeButton);
    }

    void ContainerWindow::resetReference()
    {
        ReferenceInterface::resetReference();
        mItemView->setModel(NULL);
        mModel = NULL;
        mSortModel = NULL;
    }

    void ContainerWindow::close()
    {
        WindowBase::close();

        if (dynamic_cast<PickpocketItemModel*>(mModel)
                // Make sure we were actually closed, rather than just temporarily hidden (e.g. console or main menu opened)
                && !MWBase::Environment::get().getWindowManager()->containsMode(GM_Container)
                // If it was already detected while taking an item, no need to check now
                && !mPickpocketDetected
                )
        {
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
            MWMechanics::Pickpocket pickpocket(player, mPtr);
            if (pickpocket.finish())
            {
                MWBase::Environment::get().getMechanicsManager()->commitCrime(
                            player, mPtr, MWBase::MechanicsManager::OT_Pickpocket, 0, true);
                MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Container);
                mPickpocketDetected = true;
                return;
            }
        }
    }

    void ContainerWindow::exit()
    {
        if(mDragAndDrop == NULL || !mDragAndDrop->mIsOnDragAndDrop)
        {
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
        }
    }

    void ContainerWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
    {
        exit();
    }

    void ContainerWindow::onTakeAllButtonClicked(MyGUI::Widget* _sender)
    {
        if(mDragAndDrop == NULL || !mDragAndDrop->mIsOnDragAndDrop)
        {
            // transfer everything into the player's inventory
            ItemModel* playerModel = MWBase::Environment::get().getWindowManager()->getInventoryWindow()->getModel();
            mModel->update();
            for (size_t i=0; i<mModel->getItemCount(); ++i)
            {
                if (i==0)
                {
                    // play the sound of the first object
                    MWWorld::Ptr item = mModel->getItem(i).mBase;
                    std::string sound = item.getClass().getUpSoundId(item);
                    MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
                }

                const ItemStack& item = mModel->getItem(i);

                if (!onTakeItem(item, item.mCount))
                    break;

                mModel->moveItem(item, item.mCount, playerModel);
            }

            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
        }
    }

    void ContainerWindow::onDisposeCorpseButtonClicked(MyGUI::Widget *sender)
    {
        if(mDragAndDrop == NULL || !mDragAndDrop->mIsOnDragAndDrop)
        {
            onTakeAllButtonClicked(mTakeButton);

            if (mPtr.getClass().isPersistent(mPtr))
                MWBase::Environment::get().getWindowManager()->messageBox("#{sDisposeCorpseFail}");
            else
                MWBase::Environment::get().getWorld()->deleteObject(mPtr);

            mPtr = MWWorld::Ptr();
        }
    }

    void ContainerWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
    }

    bool ContainerWindow::onTakeItem(const ItemStack &item, int count)
    {
        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayerPtr();
        // TODO: move to ItemModels
        if (dynamic_cast<PickpocketItemModel*>(mModel)
                && !mPtr.getClass().getCreatureStats(mPtr).getKnockedDown())
        {
            MWMechanics::Pickpocket pickpocket(player, mPtr);
            if (pickpocket.pick(item.mBase, count))
            {
                int value = item.mBase.getClass().getValue(item.mBase) * count;
                MWBase::Environment::get().getMechanicsManager()->commitCrime(
                            player, MWWorld::Ptr(), MWBase::MechanicsManager::OT_Theft, value, true);
                MWBase::Environment::get().getWindowManager()->removeGuiMode(MWGui::GM_Container);
                mPickpocketDetected = true;
                return false;
            }
            else
                player.getClass().skillUsageSucceeded(player, ESM::Skill::Sneak, 1);
        }
        else
        {
            // Looting a dead corpse is considered OK
            if (mPtr.getClass().isActor() && mPtr.getClass().getCreatureStats(mPtr).isDead())
                return true;
            else
                MWBase::Environment::get().getMechanicsManager()->itemTaken(player, item.mBase, mPtr, count);
        }
        return true;
    }

}
