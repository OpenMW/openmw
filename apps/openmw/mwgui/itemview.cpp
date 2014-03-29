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
        std::string path = std::string("icons\\");
        path += MWWorld::Class::get(item.mBase).getInventoryIcon(item.mBase);

        // background widget (for the "equipped" frame and magic item background image)
        bool isMagic = (item.mFlags & ItemStack::Flag_Enchanted);
        MyGUI::ImageBox* backgroundWidget = dragArea->createWidget<MyGUI::ImageBox>("ImageBox",
            MyGUI::IntCoord(x, y, 42, 42), MyGUI::Align::Default);
        backgroundWidget->setUserString("ToolTipType", "ItemModelIndex");
        backgroundWidget->setUserData(std::make_pair(i, mModel));

        std::string backgroundTex = "textures\\menu_icon";
        if (isMagic)
            backgroundTex += "_magic";
        if (item.mType == ItemStack::Type_Normal)
        {
            if (!isMagic)
                backgroundTex = "";
        }
        else if (item.mType == ItemStack::Type_Equipped)
            backgroundTex += "_equip";
        else if (item.mType == ItemStack::Type_Barter)
            backgroundTex += "_barter";

        if (backgroundTex != "")
            backgroundTex += ".dds";

        backgroundWidget->setImageTexture(backgroundTex);
        if ((item.mType == ItemStack::Type_Barter) && !isMagic)
            backgroundWidget->setProperty("ImageCoord", "2 2 42 42");
        else
            backgroundWidget->setProperty("ImageCoord", "0 0 42 42");
        backgroundWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ItemView::onSelectedItem);
        backgroundWidget->eventMouseWheel += MyGUI::newDelegate(this, &ItemView::onMouseWheel);

        // image
        MyGUI::ImageBox* image = backgroundWidget->createWidget<MyGUI::ImageBox>("ImageBox",
            MyGUI::IntCoord(5, 5, 32, 32), MyGUI::Align::Default);
        std::string::size_type pos = path.rfind(".");
        if(pos != std::string::npos)
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
    mScrollView->setCanvasSize(size);
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
    Base::setSize(_value);
    update();
}

void ItemView::setSize(int _width, int _height)
{
    Base::setSize(_width, _height);
    update();
}

void ItemView::setCoord(const MyGUI::IntCoord &_value)
{
    Base::setCoord(_value);
    update();
}

void ItemView::setCoord(int _left, int _top, int _width, int _height)
{
    Base::setCoord(_left, _top, _width, _height);
    update();
}

void ItemView::registerComponents()
{
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::ItemView>("Widget");
}

}
