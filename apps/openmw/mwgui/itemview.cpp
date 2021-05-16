#include "itemview.hpp"

#include <cmath>

#include <MyGUI_FactoryManager.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_ScrollView.h>

#include "../mwworld/class.hpp"

#include "itemmodel.hpp"
#include "itemwidget.hpp"

namespace MWGui
{

ItemView::ItemView()
    : mModel(nullptr)
    , mScrollView(nullptr)
{
}

ItemView::~ItemView()
{
    delete mModel;
}

void ItemView::setModel(ItemModel *model)
{
    if (mModel == model)
        return;

    delete mModel;
    mModel = model;

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

    int rows = maxHeight/42;
    rows = std::max(rows, 1);
    bool showScrollbar = int(std::ceil(dragArea->getChildCount()/float(rows))) > mScrollView->getWidth()/42;
    if (showScrollbar)
        maxHeight -= 18;

    for (unsigned int i=0; i<dragArea->getChildCount(); ++i)
    {
        MyGUI::Widget* w = dragArea->getChildAt(i);

        w->setPosition(x, y);

        y += 42;

        if (y > maxHeight-42 && i < dragArea->getChildCount()-1)
        {
            x += 42;
            y = 0;
        }
    }
    x += 42;

    MyGUI::IntSize size = MyGUI::IntSize(std::max(mScrollView->getSize().width, x), mScrollView->getSize().height);

    // Canvas size must be expressed with VScroll disabled, otherwise MyGUI would expand the scroll area when the scrollbar is hidden
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

    MyGUI::Widget* dragArea = mScrollView->createWidget<MyGUI::Widget>("",0,0,mScrollView->getWidth(),mScrollView->getHeight(),
                                                                       MyGUI::Align::Stretch);
    dragArea->setNeedMouseFocus(true);
    dragArea->eventMouseButtonClick += MyGUI::newDelegate(this, &ItemView::onSelectedBackground);
    dragArea->eventMouseWheel += MyGUI::newDelegate(this, &ItemView::onMouseWheelMoved);

    for (ItemModel::ModelIndex i=0; i<static_cast<int>(mModel->getItemCount()); ++i)
    {
        const ItemStack& item = mModel->getItem(i);

        ItemWidget* itemWidget = dragArea->createWidget<ItemWidget>("MW_ItemIcon",
            MyGUI::IntCoord(0, 0, 42, 42), MyGUI::Align::Default);
        itemWidget->setUserString("ToolTipType", "ItemModelIndex");
        itemWidget->setUserData(std::make_pair(i, mModel));
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
}

void ItemView::onSelectedItem(MyGUI::Widget *sender)
{
    ItemModel::ModelIndex index = (*sender->getUserData<std::pair<ItemModel::ModelIndex, ItemModel*> >()).first;
    eventItemClicked(index);
}

void ItemView::onSelectedBackground(MyGUI::Widget *sender)
{
    eventBackgroundClicked();
}

void ItemView::onMouseWheelMoved(MyGUI::Widget *_sender, int _rel)
{
    if (mScrollView->getViewOffset().left + _rel*0.3f > 0)
        mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
    else
        mScrollView->setViewOffset(MyGUI::IntPoint(static_cast<int>(mScrollView->getViewOffset().left + _rel*0.3f), 0));
}

void ItemView::setSize(const MyGUI::IntSize &_value)
{
    bool changed = (_value.width != getWidth() || _value.height != getHeight());
    Base::setSize(_value);
    if (changed)
        layoutWidgets();
}

void ItemView::setCoord(const MyGUI::IntCoord &_value)
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

}
