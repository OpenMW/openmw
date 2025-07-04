#include "itemview.hpp"

#include <cmath>

#include <MyGUI_FactoryManager.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ScrollView.h>

#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "itemmodel.hpp"
#include "itemwidget.hpp"

namespace MWGui
{

    ItemView::ItemView()
        : mScrollView(nullptr)
        , mControllerActiveWindow(false)
    {
    }

    void ItemView::setModel(std::unique_ptr<ItemModel> model)
    {
        mModel = std::move(model);

        update();
    }

    void ItemView::initialiseOverride()
    {
        Base::initialiseOverride();

        assignWidget(mScrollView, "ScrollView");
        if (mScrollView == nullptr)
            throw std::runtime_error("Item view needs a scroll view");

        mScrollView->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
    }

    void ItemView::layoutWidgets()
    {
        if (!mScrollView->getChildCount())
            return;

        int x = 0;
        int y = 0;
        MyGUI::Widget* dragArea = mScrollView->getChildAt(0);
        int maxHeight = mScrollView->getHeight();

        mRows = std::max(maxHeight / 42, 1);
        mItemCount = dragArea->getChildCount();
        bool showScrollbar = int(std::ceil(mItemCount / float(mRows))) > mScrollView->getWidth() / 42;
        if (showScrollbar)
        {
            maxHeight -= 18;
            mRows = std::max(maxHeight / 42, 1);
        }

        for (int i = 0; i < mItemCount; ++i)
        {
            MyGUI::Widget* w = dragArea->getChildAt(i);

            w->setPosition(x, y);

            y += 42;

            if (y > maxHeight - 42 && i < mItemCount - 1)
            {
                x += 42;
                y = 0;
            }
        }
        x += 42;

        MyGUI::IntSize size = MyGUI::IntSize(std::max(mScrollView->getSize().width, x), mScrollView->getSize().height);

        if (Settings::gui().mControllerMenus)
        {
            mControllerFocus = std::clamp(mControllerFocus, 0, mItemCount - 1);
            updateControllerFocus(-1, mControllerFocus);
        }

        // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the
        // scrollbar is hidden
        mScrollView->setVisibleVScroll(false);
        mScrollView->setVisibleHScroll(false);
        mScrollView->setCanvasSize(size);
        mScrollView->setVisibleVScroll(true);
        mScrollView->setVisibleHScroll(true);
        dragArea->setSize(size);
    }

    void ItemView::update()
    {
        while (mScrollView->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mScrollView->getChildAt(0));

        if (!mModel)
            return;

        mModel->update();

        MyGUI::Widget* dragArea = mScrollView->createWidget<MyGUI::Widget>(
            {}, 0, 0, mScrollView->getWidth(), mScrollView->getHeight(), MyGUI::Align::Stretch);
        dragArea->setNeedMouseFocus(true);
        dragArea->eventMouseButtonClick += MyGUI::newDelegate(this, &ItemView::onSelectedBackground);
        dragArea->eventMouseWheel += MyGUI::newDelegate(this, &ItemView::onMouseWheelMoved);

        for (ItemModel::ModelIndex i = 0; i < static_cast<int>(mModel->getItemCount()); ++i)
        {
            const ItemStack& item = mModel->getItem(i);

            ItemWidget* itemWidget = dragArea->createWidget<ItemWidget>(
                "MW_ItemIcon", MyGUI::IntCoord(0, 0, 42, 42), MyGUI::Align::Default);
            itemWidget->setUserString("ToolTipType", "ItemModelIndex");
            itemWidget->setUserData(std::make_pair(i, mModel.get()));
            ItemWidget::ItemState state = ItemWidget::None;
            if (item.mType == ItemStack::Type_Barter)
                state = ItemWidget::Barter;
            if (item.mType == ItemStack::Type_Equipped)
                state = ItemWidget::Equip;
            itemWidget->setItem(item.mBase, state);
            itemWidget->setCount(item.mCount);

            itemWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ItemView::onSelectedItem);
            itemWidget->eventMouseWheel += MyGUI::newDelegate(this, &ItemView::onMouseWheelMoved);
        }

        layoutWidgets();
    }

    void ItemView::resetScrollBars()
    {
        mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
        if (Settings::gui().mControllerMenus)
        {
            updateControllerFocus(mControllerFocus, 0);
            mControllerFocus = 0;
        }
    }

    void ItemView::onSelectedItem(MyGUI::Widget* sender)
    {
        ItemModel::ModelIndex index = (*sender->getUserData<std::pair<ItemModel::ModelIndex, ItemModel*>>()).first;
        eventItemClicked(index);
    }

    void ItemView::onSelectedBackground(MyGUI::Widget* sender)
    {
        eventBackgroundClicked();
    }

    void ItemView::onMouseWheelMoved(MyGUI::Widget* _sender, int _rel)
    {
        if (mScrollView->getViewOffset().left + _rel * 0.3f > 0)
            mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
        else
            mScrollView->setViewOffset(
                MyGUI::IntPoint(static_cast<int>(mScrollView->getViewOffset().left + _rel * 0.3f), 0));
    }

    void ItemView::setSize(const MyGUI::IntSize& _value)
    {
        bool changed = (_value.width != getWidth() || _value.height != getHeight());
        Base::setSize(_value);
        if (changed)
            layoutWidgets();
    }

    void ItemView::setCoord(const MyGUI::IntCoord& _value)
    {
        bool changed = (_value.width != getWidth() || _value.height != getHeight());
        Base::setCoord(_value);
        if (changed)
            layoutWidgets();
    }

    void ItemView::registerComponents()
    {
        MyGUI::FactoryManager::getInstance().registerFactory<MWGui::ItemView>("Widget");
    }

    void ItemView::setActiveControllerWindow(bool active)
    {
        mControllerActiveWindow = active;

        MWBase::Environment::get().getWindowManager()->setControllerTooltip(
            active && Settings::gui().mControllerTooltips);

        if (active)
            updateControllerFocus(-1, mControllerFocus);
        else
            updateControllerFocus(mControllerFocus, -1);
    }

    void ItemView::onControllerButton(const unsigned char button)
    {
        if (!mItemCount)
            return;

        int prevFocus = mControllerFocus;

        if (button == SDL_CONTROLLER_BUTTON_A)
        {
            // Select the focused item, if any.
            if (mControllerFocus >= 0 && mControllerFocus < mItemCount)
            {
                MyGUI::Widget* dragArea = mScrollView->getChildAt(0);
                onSelectedItem(dragArea->getChildAt(mControllerFocus));
            }
        }
        else if (button == SDL_CONTROLLER_BUTTON_RIGHTSTICK)
        {
            // Toggle info tooltip
            MWBase::Environment::get().getWindowManager()->setControllerTooltip(
                !MWBase::Environment::get().getWindowManager()->getControllerTooltip());
            updateControllerFocus(-1, mControllerFocus);
        }
        else if (button == SDL_CONTROLLER_BUTTON_DPAD_UP)
        {
            if (mControllerFocus % mRows == 0)
                mControllerFocus = std::min(mControllerFocus + mRows - 1, mItemCount - 1);
            else
                mControllerFocus--;
        }
        else if (button == SDL_CONTROLLER_BUTTON_DPAD_DOWN)
        {
            if (mControllerFocus % mRows == mRows - 1 || mControllerFocus == mItemCount - 1)
                mControllerFocus -= mControllerFocus % mRows;
            else
                mControllerFocus++;
        }
        else if (button == SDL_CONTROLLER_BUTTON_DPAD_LEFT && mControllerFocus >= mRows)
            mControllerFocus -= mRows;
        else if (button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
        {
            if (mControllerFocus + mRows < mItemCount)
                mControllerFocus += mRows;
            else if (mControllerFocus / mRows != (mItemCount - 1) / mRows)
                mControllerFocus = mItemCount - 1;
        }

        if (prevFocus != mControllerFocus)
            updateControllerFocus(prevFocus, mControllerFocus);
        else
            updateControllerFocus(-1, mControllerFocus);
    }

    void ItemView::updateControllerFocus(int prevFocus, int newFocus)
    {
        MWBase::Environment::get().getWindowManager()->setCursorVisible(
            !MWBase::Environment::get().getWindowManager()->getControllerTooltip());

        if (!mItemCount)
            return;

        MyGUI::Widget* dragArea = mScrollView->getChildAt(0);

        if (prevFocus >= 0 && prevFocus < mItemCount)
        {
            ItemWidget* prev = (ItemWidget*)dragArea->getChildAt(prevFocus);
            if (prev)
                prev->setControllerFocus(false);
        }

        if (mControllerActiveWindow && newFocus >= 0 && newFocus < mItemCount)
        {
            ItemWidget* focused = (ItemWidget*)dragArea->getChildAt(newFocus);
            if (focused)
            {
                focused->setControllerFocus(true);

                // Scroll the list to keep the active item in view
                int column = newFocus / mRows;
                if (column <= 3)
                    mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
                else
                    mScrollView->setViewOffset(MyGUI::IntPoint(-42 * (column - 3), 0));

                if (MWBase::Environment::get().getWindowManager()->getControllerTooltip())
                    MWBase::Environment::get().getInputManager()->warpMouseToWidget(focused);
            }
        }
    }
}
