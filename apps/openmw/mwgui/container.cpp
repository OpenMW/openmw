#include "container.hpp"

#include "window_manager.hpp"
#include "widgets.hpp"

#include "../mwbase/environment.hpp"
#include "../mwworld/manualref.hpp"
#include "../mwworld/world.hpp"
#include "../mwworld/containerstore.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/player.hpp"
#include "../mwclass/container.hpp"
#include "../mwinput/inputmanager.hpp"
#include "../mwsound/soundmanager.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>
#include <assert.h>
#include <iostream>

#include <boost/lexical_cast.hpp>


using namespace MWGui;
using namespace Widgets;


ContainerBase::ContainerBase(DragAndDrop* dragAndDrop) :
    mDragAndDrop(dragAndDrop),
    mFilter(ContainerBase::Filter_All)
{
}

void ContainerBase::setWidgets(Widget* containerWidget, ScrollView* itemView)
{
    mContainerWidget = containerWidget;
    mItemView = itemView;

    mContainerWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerBase::onContainerClicked);
    mContainerWidget->eventMouseWheel += MyGUI::newDelegate(this, &ContainerWindow::onMouseWheel);
}

ContainerBase::~ContainerBase()
{
}

void ContainerBase::onSelectedItem(MyGUI::Widget* _sender)
{
    if(!mDragAndDrop->mIsOnDragAndDrop)
    {
        mDragAndDrop->mIsOnDragAndDrop = true;
        _sender->detachFromWidget();
        _sender->attachToWidget(mDragAndDrop->mDragAndDropWidget);

        MWWorld::Ptr object = *_sender->getUserData<MWWorld::Ptr>();
        mDragAndDrop->mStore.add(object);
        object.getRefData().setCount(0);

        std::string sound = MWWorld::Class::get(object).getUpSoundId(object);
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        mDragAndDrop->mDraggedWidget = _sender;
        mDragAndDrop->mContainerWindow = const_cast<MWGui::ContainerBase*>(this);
        // hide the count text
        _sender->getChildAt(0)->getChildAt(0)->setVisible(false);
        drawItems();

        MWBase::Environment::get().getWindowManager()->setDragDrop(true);
    }
    else
        onContainerClicked(mContainerWidget);
}

void ContainerBase::onContainerClicked(MyGUI::Widget* _sender)
{
    if(mDragAndDrop->mIsOnDragAndDrop) //drop widget here
    {
        MWWorld::Ptr object = *mDragAndDrop->mDraggedWidget->getUserData<MWWorld::Ptr>();
        assert(object.getContainerStore() && "Item is not in a container!");

        std::string sound = MWWorld::Class::get(object).getDownSoundId(object);
        MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);

        MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);
        containerStore.add(*mDragAndDrop->mStore.begin());
        mDragAndDrop->mStore.clear();
        mDragAndDrop->mIsOnDragAndDrop = false;
        mDragAndDrop->mDraggedWidget->detachFromWidget();
        mDragAndDrop->mDraggedWidget->attachToWidget(mContainerWidget);
        mDragAndDrop->mDraggedWidget = 0;
        mDragAndDrop->mContainerWindow = 0;
        drawItems();

        MWBase::Environment::get().getWindowManager()->setDragDrop(false);
    }
}

void ContainerBase::onMouseWheel(MyGUI::Widget* _sender, int _rel)
{
    if (mItemView->getViewOffset().left + _rel*0.3 > 0)
        mItemView->setViewOffset(MyGUI::IntPoint(0, 0));
    else
        mItemView->setViewOffset(MyGUI::IntPoint(mItemView->getViewOffset().left + _rel*0.3, 0));
}

void ContainerBase::setFilter(ContainerBase::Filter filter)
{
    mFilter = filter;
    drawItems();
}

void ContainerBase::openContainer(MWWorld::Ptr container)
{
    mContainer = container;
    drawItems();
}

void ContainerBase::drawItems()
{
    while (mContainerWidget->getChildCount())
    {
        MyGUI::Gui::getInstance().destroyWidget(mContainerWidget->getChildAt(0));
    }
    MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);

    int x = 0;
    int y = 0;
    int maxHeight = mItemView->getSize().height - 58;

    int index = 0;


    bool onlyMagic = false;
    int categories;
    if (mFilter == Filter_All)
        categories = MWWorld::ContainerStore::Type_All;
    else if (mFilter == Filter_Weapon)
        categories = MWWorld::ContainerStore::Type_Weapon;
    else if (mFilter == Filter_Apparel)
        categories = MWWorld::ContainerStore::Type_Clothing + MWWorld::ContainerStore::Type_Armor;
    else if (mFilter == Filter_Magic)
    {
        categories = MWWorld::ContainerStore::Type_Clothing + MWWorld::ContainerStore::Type_Armor
                    + MWWorld::ContainerStore::Type_Weapon + MWWorld::ContainerStore::Type_Book
                    + MWWorld::ContainerStore::Type_Potion;
        onlyMagic = true;
    }
    else if (mFilter == Filter_Misc)
    {
        categories = MWWorld::ContainerStore::Type_Miscellaneous + MWWorld::ContainerStore::Type_Book
                    + MWWorld::ContainerStore::Type_Ingredient + MWWorld::ContainerStore::Type_Repair
                    + MWWorld::ContainerStore::Type_Lockpick + MWWorld::ContainerStore::Type_Light
                    + MWWorld::ContainerStore::Type_Apparatus;
    }

    for (MWWorld::ContainerStoreIterator iter (containerStore.begin(categories)); iter!=containerStore.end(); ++iter)
    {
        index++;
        if(iter->getRefData().getCount() > 0 && !(onlyMagic && MWWorld::Class::get(*iter).getEnchantment(*iter) == "" && iter->getTypeName() != typeid(ESM::Potion).name()))
        {
            std::string path = std::string("icons\\");
            path+=MWWorld::Class::get(*iter).getInventoryIcon(*iter);

            // background widget (for the "equipped" frame and magic item background image)
            bool isMagic = (MWWorld::Class::get(*iter).getEnchantment(*iter) != "");
            MyGUI::ImageBox* backgroundWidget = mContainerWidget->createWidget<ImageBox>("ImageBox", MyGUI::IntCoord(x, y, 42, 42), MyGUI::Align::Default);
            backgroundWidget->setUserString("ToolTipType", "ItemPtr");
            backgroundWidget->setUserData(*iter);
            backgroundWidget->setImageTexture( isMagic ? "textures\\menu_icon_magic.dds" : "");
            backgroundWidget->setProperty("ImageCoord", "0 0 42 42");
            backgroundWidget->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerBase::onSelectedItem);
            backgroundWidget->eventMouseWheel += MyGUI::newDelegate(this, &ContainerBase::onMouseWheel);

            // image
            ImageBox* image = backgroundWidget->createWidget<ImageBox>("ImageBox", MyGUI::IntCoord(5, 5, 32, 32), MyGUI::Align::Default);
            int pos = path.rfind(".");
            path.erase(pos);
            path.append(".dds");
            image->setImageTexture(path);
            image->setNeedMouseFocus(false);

            // text widget that shows item count
            MyGUI::TextBox* text = image->createWidget<MyGUI::TextBox>("SandBrightText", MyGUI::IntCoord(0, 14, 32, 18), MyGUI::Align::Default, std::string("Label"));
            text->setTextAlign(MyGUI::Align::Right);
            text->setNeedMouseFocus(false);
            text->setTextShadow(true);
            text->setTextShadowColour(MyGUI::Colour(0,0,0));

            y += 42;
            if (y > maxHeight)
            {
                x += 42;
                y = 0;
            }

            if(iter->getRefData().getCount() > 1)
            {
                if (iter->getRefData().getCount() > 9999)
                    text->setCaption(boost::lexical_cast<std::string>(iter->getRefData().getCount()/1000.f) + "k");
                else
                    text->setCaption(boost::lexical_cast<std::string>(iter->getRefData().getCount()));
            }
        }
    }

    MyGUI::IntSize size = MyGUI::IntSize(std::max(mItemView->getSize().width, x+42), mItemView->getSize().height);
    mItemView->setCanvasSize(size);
    mContainerWidget->setSize(size);
}

void ContainerBase::Update()
{
    if(mDragAndDrop->mIsOnDragAndDrop)
    {
        if(mDragAndDrop->mDraggedWidget)
            mDragAndDrop->mDraggedWidget->setPosition(MyGUI::InputManager::getInstance().getMousePosition() - MyGUI::IntPoint(21, 21));
        else mDragAndDrop->mIsOnDragAndDrop = false; //If this happens, there is a bug.
    }
}

// ------------------------------------------------------------------------------------------------

ContainerWindow::ContainerWindow(WindowManager& parWindowManager,DragAndDrop* dragAndDrop)
    : ContainerBase(dragAndDrop)
    , WindowBase("openmw_container_window_layout.xml", parWindowManager)
{
    getWidget(mTakeButton, "TakeButton");
    getWidget(mCloseButton, "CloseButton");

    MyGUI::ScrollView* itemView;
    MyGUI::Widget* containerWidget;
    getWidget(containerWidget, "Items");
    getWidget(itemView, "ItemView");
    setWidgets(containerWidget, itemView);

    mCloseButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onCloseButtonClicked);
    mTakeButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ContainerWindow::onTakeAllButtonClicked);

    setText("CloseButton", MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sClose")->str);
    setText("TakeButton", MWBase::Environment::get().getWorld()->getStore().gameSettings.search("sTakeAll")->str);

    // adjust buttons size to fit text
    int closeButtonWidth = mCloseButton->getTextSize().width+24;
    int takeButtonWidth = mTakeButton->getTextSize().width+24;
    mCloseButton->setCoord(600-20-closeButtonWidth, mCloseButton->getCoord().top, closeButtonWidth, mCloseButton->getCoord().height);
    mTakeButton->setCoord(600-20-closeButtonWidth-takeButtonWidth-8, mTakeButton->getCoord().top, takeButtonWidth, mTakeButton->getCoord().height);

    int w = MyGUI::RenderManager::getInstance().getViewSize().width;
    //int h = MyGUI::RenderManager::getInstance().getViewSize().height;

    static_cast<MyGUI::Window*>(mMainWidget)->eventWindowChangeCoord += MyGUI::newDelegate(this, &ContainerWindow::onWindowResize);

    setCoord(w-600,0,600,300);
}

ContainerWindow::~ContainerWindow()
{
}

void ContainerWindow::onWindowResize(MyGUI::Window* window)
{
    drawItems();
}

void ContainerWindow::open(MWWorld::Ptr container)
{
    openContainer(container);
    setTitle(MWWorld::Class::get(container).getName(container));
}

void ContainerWindow::onCloseButtonClicked(MyGUI::Widget* _sender)
{
    if(!mDragAndDrop->mIsOnDragAndDrop)
    {
        MWBase::Environment::get().getWindowManager()->setGuiMode(GM_Game);
        setVisible(false);
    }
}

void ContainerWindow::onTakeAllButtonClicked(MyGUI::Widget* _sender)
{
    if(!mDragAndDrop->mIsOnDragAndDrop)
    {
        // transfer everything into the player's inventory
        MWWorld::ContainerStore& containerStore = MWWorld::Class::get(mContainer).getContainerStore(mContainer);

        MWWorld::Ptr player = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();
        MWWorld::ContainerStore& playerStore = MWWorld::Class::get(player).getContainerStore(player);

        int i=0;
        for (MWWorld::ContainerStoreIterator iter (containerStore.begin()); iter!=containerStore.end(); ++iter)
        {
            if(iter->getRefData().getCount() > 0)
            {
                playerStore.add(*iter);

                if (i==0)
                {
                    // play the sound of the first object
                    std::string sound = MWWorld::Class::get(*iter).getUpSoundId(*iter);
                    MWBase::Environment::get().getSoundManager()->playSound (sound, 1.0, 1.0);
                }

                ++i;
            }
        }

        containerStore.clear();

        MWBase::Environment::get().getWindowManager()->setGuiMode(GM_Game);
        setVisible(false);
    }
}
