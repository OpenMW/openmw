#include "itemview.hpp"

#include <cmath>

#include <MyGUI_FactoryManager.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ScrollView.h>

#include <components/settings/values.hpp>

#include "itemmodel.hpp"
#include "itemwidget.hpp"

namespace MWGui
{

    ItemView::ItemView()
        : mScrollView(nullptr)
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

        mRows = maxHeight / 42;
        mRows = std::max(mRows, 1);
        mItemCount = dragArea->getChildCount();
        bool showScrollbar = int(std::ceil(mItemCount / float(mRows))) > mScrollView->getWidth() / 42;
        if (showScrollbar)
            maxHeight -= 18;

        for (unsigned int i = 0; i < mItemCount; ++i)
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
            if (mControllerFocus >= mItemCount)
                mControllerFocus = mItemCount - 1;
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

    void ItemView::onControllerButtonEvent(const SDL_ControllerButtonEvent& arg)
    {
        if (!mItemCount)
            return;

        int prevFocus = mControllerFocus;

        if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_UP && mControllerFocus % mRows != 0)
            mControllerFocus--;
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN && mControllerFocus % mRows != mRows - 1)
            mControllerFocus++;
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_LEFT && mControllerFocus >= mRows)
            mControllerFocus -= mRows;
        else if (arg.button == SDL_CONTROLLER_BUTTON_DPAD_RIGHT && mControllerFocus + mRows < mItemCount)
            mControllerFocus += mRows;

        if (mControllerFocus < 0)
            mControllerFocus = 0;
        else if (mControllerFocus >= mItemCount - 1)
            mControllerFocus = mItemCount - 1;

        if (prevFocus != mControllerFocus)
            updateControllerFocus(prevFocus, mControllerFocus);
    }

    void ItemView::updateControllerFocus(int _prevFocus, int _newFocus)
    {
        if (!mItemCount)
            return;

        MyGUI::Widget* dragArea = mScrollView->getChildAt(0);

        if (_prevFocus >= 0 && _prevFocus < mItemCount)
        {
            ItemWidget* prev = (ItemWidget *)dragArea->getChildAt(_prevFocus);
            if (prev)
                prev->setControllerFocus(false);
        }

        if (_newFocus >= 0 && _newFocus < mItemCount)
        {
            ItemWidget* focused = (ItemWidget *)dragArea->getChildAt(_newFocus);
            if (focused)
                focused->setControllerFocus(true);
        }
    }
}
