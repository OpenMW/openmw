#include "container.hpp"

#include <boost/lexical_cast.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"

#include "countdialog.hpp"
#include "tradewindow.hpp"
#include "inventorywindow.hpp"

#include "itemview.hpp"
#include "inventoryitemmodel.hpp"
#include "sortfilteritemmodel.hpp"
#include "pickpocketitemmodel.hpp"

namespace
{
    std::string getCountString(const int count)
    {
        if (count == 1)
            return "";
        if (count > 9999)
            return boost::lexical_cast<std::string>(int(count/1000.f)) + "k";
        else
            return boost::lexical_cast<std::string>(count);
    }
}

namespace MWGui
{

    void DragAndDrop::startDrag (int index, SortFilterItemModel* sortModel, ItemModel* sourceModel, ItemView* sourceView, int count)
    {
        mItem = sourceModel->getItem(index);
        mDraggedCount = count;
        mSourceModel = sourceModel;
        mSourceView = sourceView;
        mSourceSortModel = sortModel;
        mIsOnDragAndDrop = true;
        mDragAndDropWidget->setVisible(true);

        std::string sound = MWWorld::Class::get(mItem.mBase).getUpSoundId(mItem.mBase);
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        if (mSourceSortModel)
        {
            mSourceSortModel->clearDragItems();
            mSourceSortModel->addDragItem(mItem.mBase, count);
        }

        std::string path = std::string("icons\\");
        path += MWWorld::Class::get(mItem.mBase).getInventoryIcon(mItem.mBase);
        MyGUI::ImageBox* baseWidget = mDragAndDropWidget->createWidget<MyGUI::ImageBox>
                ("ImageBox", MyGUI::IntCoord(0, 0, 42, 42), MyGUI::Align::Default);
        mDraggedWidget = baseWidget;
        MyGUI::ImageBox* image = baseWidget->createWidget<MyGUI::ImageBox>("ImageBox",
            MyGUI::IntCoord(5, 5, 32, 32), MyGUI::Align::Default);
        size_t pos = path.rfind(".");
        if (pos != std::string::npos)
            path.erase(pos);
        path.append(".dds");
        image->setImageTexture(path);
        image->setNeedMouseFocus(false);

        // text widget that shows item count
        MyGUI::TextBox* text = image->createWidget<MyGUI::TextBox>("SandBrightText",
            MyGUI::IntCoord(0, 14, 32, 18), MyGUI::Align::Default, std::string("Label"));
        text->setTextAlign(MyGUI::Align::Right);
        text->setNeedMouseFocus(false);
        text->setTextShadow(true);
        text->setTextShadowColour(MyGUI::Colour(0,0,0));
        text->setCaption(getCountString(count));

        sourceView->update();

        MWBase::Environment::get().getWindowManager()->setDragDrop(true);
    }

    void DragAndDrop::drop(ItemModel *targetModel, ItemView *targetView)
    {
        std::string sound = MWWorld::Class::get(mItem.mBase).getDownSoundId(mItem.mBase);
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        mDragAndDropWidget->setVisible(false);

        // If item is dropped where it was taken from, we don't need to do anything -
        // otherwise, do the transfer
        if (targetModel != mSourceModel)
        {
            targetModel->copyItem(mItem, mDraggedCount);
            mSourceModel->removeItem(mItem, mDraggedCount);
        }

        mSourceModel->update();

        finish();
        targetView->update();

        // We need to update the view since an other item could be auto-equipped.
        mSourceView->update();
    }

    void DragAndDrop::finish()
    {
        mIsOnDragAndDrop = false;
        mSourceSortModel->clearDragItems();

        MyGUI::Gui::getInstance().destroyWidget(mDraggedWidget);
        mDraggedWidget = 0;
        MWBase::Environment::get().getWindowManager()->setDragDrop(false);
    }


    ContainerWindow::ContainerWindow(DragAndDrop* dragAndDrop)
        : WindowBase("openmw_container_window.layout")
        , mDragAndDrop(dragAndDrop)
        , mSelectedItem(-1)
        , mModel(NULL)
        , mSortModel(NULL)
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
            if (!dynamic_cast<PickpocketItemModel*>(mModel))
                dropItem();
            return;
        }

        const ItemStack& item = mSortModel->getItem(index);

        MWWorld::Ptr object = item.mBase;
        int count = item.mCount;
        bool shift = MyGUI::InputManager::getInstance().isShiftPressed();
        if (MyGUI::InputManager::getInstance().isControlPressed())
            count = 1;

        mSelectedItem = mSortModel->mapToSource(index);

        if (count > 1 && !shift)
        {
            CountDialog* dialog = MWBase::Environment::get().getWindowManager()->getCountDialog();
            dialog->open(MWWorld::Class::get(object).getName(object), "#{sTake}", count);
            dialog->eventOkClicked.clear();
            dialog->eventOkClicked += MyGUI::newDelegate(this, &ContainerWindow::dragItem);
        }
        else
            dragItem (NULL, count);
    }

    void ContainerWindow::dragItem(MyGUI::Widget* sender, int count)
    {
        mDragAndDrop->startDrag(mSelectedItem, mSortModel, mModel, mItemView, count);
    }

    void ContainerWindow::dropItem()
    {
        if (mPtr.getTypeName() == typeid(ESM::Container).name())
        {
            // check that we don't exceed container capacity
            MWWorld::Ptr item = mDragAndDrop->mItem.mBase;
            float weight = MWWorld::Class::get(item).getWeight(item) * mDragAndDrop->mDraggedCount;
            if (MWWorld::Class::get(mPtr).getCapacity(mPtr) < MWWorld::Class::get(mPtr).getEncumbrance(mPtr) + weight)
            {
                MWBase::Environment::get().getWindowManager()->messageBox("#{sContentsMessage3}");
                return;
            }

            // check container organic flag
            MWWorld::LiveCellRef<ESM::Container>* ref = mPtr.get<ESM::Container>();
            if (ref->mBase->mFlags & ESM::Container::Organic)
            {
                MWBase::Environment::get().getWindowManager()->
                    messageBox("#{sContentsMessage2}");
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
        mPtr = container;

        if (mPtr.getTypeName() == typeid(ESM::NPC).name() && !loot)
        {
            // we are stealing stuff
            MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
            mModel = new PickpocketItemModel(player, new InventoryItemModel(container));
        }
        else
            mModel = new InventoryItemModel(container);

        mDisposeCorpseButton->setVisible(loot);

        mSortModel = new SortFilterItemModel(mModel);

        mItemView->setModel (mSortModel);

        // Careful here. setTitle may cause size updates, causing itemview redraw, so make sure to do it last
        // or we end up using a possibly invalid model.
        setTitle(MWWorld::Class::get(container).getName(container));
    }

    void ContainerWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
    {
        if(mDragAndDrop == NULL || !mDragAndDrop->mIsOnDragAndDrop)
        {
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
        }
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
                    std::string sound = MWWorld::Class::get(item).getUpSoundId(item);
                    MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
                }

                playerModel->copyItem(mModel->getItem(i), mModel->getItem(i).mCount);
                mModel->removeItem(mModel->getItem(i), mModel->getItem(i).mCount);
            }

            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Container);
        }
    }

    void ContainerWindow::onDisposeCorpseButtonClicked(MyGUI::Widget *sender)
    {
        if(mDragAndDrop == NULL || !mDragAndDrop->mIsOnDragAndDrop)
        {
            onTakeAllButtonClicked(mTakeButton);

            if (MWWorld::Class::get(mPtr).isPersistent(mPtr))
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

}
