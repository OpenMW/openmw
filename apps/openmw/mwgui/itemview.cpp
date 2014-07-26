#include "itemview.hpp"

#include <boost/lexical_cast.hpp>

#include <MyGUI_FactoryManager.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_TextBox.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_Button.h>

#include "../mwworld/class.hpp"

#include "itemmodel.hpp"
#include "itemwidget.hpp"

namespace MWGui
{

std::string ItemView::getCountString(int count)
{
    if (count == 1)
        return "";
    if (count > 9999)
        return boost::lexical_cast<std::string>(int(count/1000.f)) + "k";
    else
        return boost::lexical_cast<std::string>(count);
}

ItemView::ItemView()
    : mModel(NULL)
    , mScrollView(NULL)
{
}

ItemView::~ItemView()
{
    delete mModel;
}

void ItemView::setModel(ItemModel *model)
{
    delete mModel;
    mModel = model;
    update();
}

void ItemView::initialiseOverride()
{
    Base::initialiseOverride();

    assignWidget(mScrollView, "ScrollView");
    if (mScrollView == NULL)
        throw std::runtime_error("Item view needs a scroll view");

    mScrollView->setCanvasAlign(MyGUI::Align::Left | MyGUI::Align::Top);
}

void ItemView::update()
{
    while (mScrollView->getChildCount())
        MyGUI::Gui::getInstance().destroyWidget(mScrollView->getChildAt(0));

    if (!mModel)
        return;

    int x = 0;
    int y = 0;
    int maxHeight = mScrollView->getSize().height - 58;

    mModel->update();

    MyGUI::Widget* dragArea = mScrollView->createWidget<MyGUI::Widget>("",0,0,mScrollView->getWidth(),mScrollView->getHeight(),
                                                                       MyGUI::Align::Stretch);
    dragArea->setNeedMouseFocus(true);
    dragArea->eventMouseButtonClick += MyGUI::newDelegate(this, &ItemView::onSelectedBackground);
    dragArea->eventMouseWheel += MyGUI::newDelegate(this, &ItemView::onMouseWheel);

    for (ItemModel::ModelIndex i=0; i<static_cast<int>(mModel->getItemCount()); ++i)
    {
        const ItemStack& item = mModel->getItem(i);

        /// \todo performance improvement: don't create/destroy all the widgets everytime the container window changes size, only reposition them
        ItemWidget* itemWidget = dragArea->createWidget<ItemWidget>("MW_ItemIcon",
            MyGUI::IntCoord(x, y, 42, 42), MyGUI::Align::Default);
        itemWidget->setUserString("ToolTipType", "ItemModelIndex");
        itemWidget->setUserData(std::make_pair(i, mModel));
        ItemWidget::ItemState state = ItemWidget::None;
        if (item.mType == ItemStack::Type_Barter)
            state = ItemWidget::Barter;
        if (item.mType == ItemStack::Type_Equipped)
            state = ItemWidget::Equip;
        itemWidget->setItem(item.mBase, state);

        itemWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ItemView::onSelectedItem);
        itemWidget->eventMouseWheel += MyGUI::newDelegate(this, &ItemView::onMouseWheel);

        // text widget that shows item count
        // TODO: move to ItemWidget
        MyGUI::TextBox* text = itemWidget->createWidget<MyGUI::TextBox>("SandBrightText",
            MyGUI::IntCoord(5, 19, 32, 18), MyGUI::Align::Default, std::string("Label"));
        text->setTextAlign(MyGUI::Align::Right);
        text->setNeedMouseFocus(false);
        text->setTextShadow(true);
        text->setTextShadowColour(MyGUI::Colour(0,0,0));
        text->setCaption(getCountString(item.mCount));

        y += 42;
        if (y > maxHeight)
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

void ItemView::onSelectedItem(MyGUI::Widget *sender)
{
    ItemModel::ModelIndex index = (*sender->getUserData<std::pair<ItemModel::ModelIndex, ItemModel*> >()).first;
    eventItemClicked(index);
}

void ItemView::onSelectedBackground(MyGUI::Widget *sender)
{
    eventBackgroundClicked();
}

void ItemView::onMouseWheel(MyGUI::Widget *_sender, int _rel)
{
    if (mScrollView->getViewOffset().left + _rel*0.3 > 0)
        mScrollView->setViewOffset(MyGUI::IntPoint(0, 0));
    else
        mScrollView->setViewOffset(MyGUI::IntPoint(mScrollView->getViewOffset().left + _rel*0.3, 0));
}

void ItemView::setSize(const MyGUI::IntSize &_value)
{
    bool changed = (_value.width != getWidth() || _value.height != getHeight());
    Base::setSize(_value);
    if (changed)
        update();
}

void ItemView::setSize(int _width, int _height)
{
    setSize(MyGUI::IntSize(_width, _height));
}

void ItemView::setCoord(const MyGUI::IntCoord &_value)
{
    bool changed = (_value.width != getWidth() || _value.height != getHeight());
    Base::setCoord(_value);
    if (changed)
        update();
}

void ItemView::setCoord(int _left, int _top, int _width, int _height)
{
    setCoord(MyGUI::IntCoord(_left, _top, _width, _height));
}

void ItemView::registerComponents()
{
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::ItemView>("Widget");
}

}
