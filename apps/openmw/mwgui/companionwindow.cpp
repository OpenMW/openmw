#include "companionwindow.hpp"

#include <cmath>

#include <MyGUI_Button.h>
#include <MyGUI_EditBox.h>
#include <MyGUI_InputManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"

#include "companionitemmodel.hpp"
#include "countdialog.hpp"
#include "draganddrop.hpp"
#include "itemview.hpp"
#include "messagebox.hpp"
#include "sortfilteritemmodel.hpp"
#include "tooltips.hpp"
#include "widgets.hpp"

namespace
{

    int getProfit(const MWWorld::Ptr& actor)
    {
        const ESM::RefId& script = actor.getClass().getScript(actor);
        if (!script.empty())
        {
            return actor.getRefData().getLocals().getIntVar(script, "minimumprofit");
        }
        return 0;
    }

}

namespace MWGui
{

    CompanionWindow::CompanionWindow(DragAndDrop* dragAndDrop, MessageBoxManager* manager)
        : WindowBase("openmw_companion_window.layout")
        , mSortModel(nullptr)
        , mModel(nullptr)
        , mSelectedItem(-1)
        , mUpdateNextFrame(false)
        , mDragAndDrop(dragAndDrop)
        , mMessageBoxManager(manager)
    {
        getWidget(mCloseButton, "CloseButton");
        getWidget(mProfitLabel, "ProfitLabel");
        getWidget(mEncumbranceBar, "EncumbranceBar");
        getWidget(mFilterEdit, "FilterEdit");
        getWidget(mItemView, "ItemView");
        mItemView->eventBackgroundClicked += MyGUI::newDelegate(this, &CompanionWindow::onBackgroundSelected);
        mItemView->eventItemClicked += MyGUI::newDelegate(this, &CompanionWindow::onItemSelected);
        mFilterEdit->eventEditTextChange += MyGUI::newDelegate(this, &CompanionWindow::onNameFilterChanged);

        mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &CompanionWindow::onCloseButtonClicked);

        setCoord(200, 0, 600, 300);
    }

    void CompanionWindow::onItemSelected(int index)
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            mDragAndDrop->drop(mModel, mItemView);
            updateEncumbranceBar();
            return;
        }

        const ItemStack& item = mSortModel->getItem(index);

        // We can't take conjured items from a companion actor
        if (item.mFlags & ItemStack::Flag_Bound)
        {
            MWBase::Environment::get().getWindowManager()->messageBox("#{sBarterDialog12}");
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
            dialog->eventOkClicked += MyGUI::newDelegate(this, &CompanionWindow::dragItem);
        }
        else
            dragItem(nullptr, count);
    }

    void CompanionWindow::onNameFilterChanged(MyGUI::EditBox* _sender)
    {
        mSortModel->setNameFilter(_sender->getCaption());
        mItemView->update();
    }

    void CompanionWindow::dragItem(MyGUI::Widget* /*sender*/, std::size_t count)
    {
        mDragAndDrop->startDrag(mSelectedItem, mSortModel, mModel, mItemView, count);
    }

    void CompanionWindow::onBackgroundSelected()
    {
        if (mDragAndDrop->mIsOnDragAndDrop)
        {
            mDragAndDrop->drop(mModel, mItemView);
            updateEncumbranceBar();
        }
    }

    void CompanionWindow::setPtr(const MWWorld::Ptr& actor)
    {
        if (actor.isEmpty() || !actor.getClass().isActor())
            throw std::runtime_error("Invalid argument in CompanionWindow::setPtr");
        mPtr = actor;
        updateEncumbranceBar();
        auto model = std::make_unique<CompanionItemModel>(actor);
        mModel = model.get();
        auto sortModel = std::make_unique<SortFilterItemModel>(std::move(model));
        mSortModel = sortModel.get();
        mFilterEdit->setCaption({});
        mItemView->setModel(std::move(sortModel));
        mItemView->resetScrollBars();

        setTitle(actor.getClass().getName(actor));

        mPtr.getClass().getContainerStore(mPtr).setContListener(this);
    }

    void CompanionWindow::onFrame(float dt)
    {
        checkReferenceAvailable();

        if (mUpdateNextFrame)
        {
            updateEncumbranceBar();
            mItemView->update();
            mUpdateNextFrame = false;
        }
    }

    void CompanionWindow::updateEncumbranceBar()
    {
        if (mPtr.isEmpty())
            return;
        float capacity = mPtr.getClass().getCapacity(mPtr);
        float encumbrance = mPtr.getClass().getEncumbrance(mPtr);
        mEncumbranceBar->setValue(std::ceil(encumbrance), static_cast<int>(capacity));

        if (mModel && mModel->hasProfit(mPtr))
        {
            mProfitLabel->setCaptionWithReplacing("#{sProfitValue} " + MyGUI::utility::toString(getProfit(mPtr)));
        }
        else
            mProfitLabel->setCaption({});
    }

    void CompanionWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
    {
        if (exit())
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Companion);
    }

    bool CompanionWindow::exit()
    {
        if (mModel && mModel->hasProfit(mPtr) && getProfit(mPtr) < 0)
        {
            std::vector<std::string> buttons;
            buttons.emplace_back("#{sCompanionWarningButtonOne}");
            buttons.emplace_back("#{sCompanionWarningButtonTwo}");
            mMessageBoxManager->createInteractiveMessageBox("#{sCompanionWarningMessage}", buttons);
            mMessageBoxManager->eventButtonPressed
                += MyGUI::newDelegate(this, &CompanionWindow::onMessageBoxButtonClicked);
            return false;
        }
        return true;
    }

    void CompanionWindow::onMessageBoxButtonClicked(int button)
    {
        if (button == 0)
        {
            MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Companion);
            // Important for Calvus' contract script to work properly
            MWBase::Environment::get().getWindowManager()->exitCurrentGuiMode();
        }
    }

    void CompanionWindow::onReferenceUnavailable()
    {
        MWBase::Environment::get().getWindowManager()->removeGuiMode(GM_Companion);
    }

    void CompanionWindow::resetReference()
    {
        ReferenceInterface::resetReference();
        mItemView->setModel(nullptr);
        mModel = nullptr;
        mSortModel = nullptr;
    }

    void CompanionWindow::itemAdded(const MWWorld::ConstPtr& item, int count)
    {
        mUpdateNextFrame = true;
    }

    void CompanionWindow::itemRemoved(const MWWorld::ConstPtr& item, int count)
    {
        mUpdateNextFrame = true;
    }
}
