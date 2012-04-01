#include "map_window.hpp"
/*
#include "../mwmechanics/mechanicsmanager.hpp"
#include "window_manager.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>

#undef min
#undef max
*/
using namespace MWGui;

MapWindow::MapWindow(WindowManager& parWindowManager) : 
    MWGui::WindowPinnableBase("openmw_map_window_layout.xml", parWindowManager),
    mGlobal(false),
    mLastPositionX(0.0f),
    mLastPositionY(0.0f),
    mLastDirectionX(0.0f),
    mLastDirectionY(0.0f)
{
    setCoord(500,0,320,300);
    setText("WorldButton", "World");
    setImage("Compass", "textures\\compass.dds");

    // Obviously you should override this later on
    setCellName("No Cell Loaded");

    getWidget(mLocalMap, "LocalMap");
    getWidget(mGlobalMap, "GlobalMap");
    getWidget(mPlayerArrow, "Compass");

    getWidget(mButton, "WorldButton");
    mButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MapWindow::onWorldButtonClicked);

    MyGUI::Button* eventbox;
    getWidget(eventbox, "EventBox");
    eventbox->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
    eventbox->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);

    LocalMapBase::init(mLocalMap, this);
}

void MapWindow::setCellName(const std::string& cellName)
{
    static_cast<MyGUI::Window*>(mMainWidget)->setCaption(cellName);
    adjustWindowCaption();
}

void MapWindow::setPlayerPos(const float x, const float y)
{
    if (mGlobal || !mVisible || (x == mLastPositionX && y == mLastPositionY)) return;
    MyGUI::IntSize size = mLocalMap->getCanvasSize();
    MyGUI::IntPoint middle = MyGUI::IntPoint((1/3.f + x/3.f)*size.width,(1/3.f + y/3.f)*size.height);
    MyGUI::IntCoord viewsize = mLocalMap->getCoord();
    MyGUI::IntPoint pos(0.5*viewsize.width - middle.left, 0.5*viewsize.height - middle.top);
    mLocalMap->setViewOffset(pos);

    mPlayerArrow->setPosition(MyGUI::IntPoint(x*512-16, y*512-16));
    mLastPositionX = x;
    mLastPositionY = y;
}

void MapWindow::setPlayerDir(const float x, const float y)
{
    if (!mVisible || (x == mLastDirectionX && y == mLastDirectionY)) return;
    MyGUI::ISubWidget* main = mPlayerArrow->getSubWidgetMain();
    MyGUI::RotatingSkin* rotatingSubskin = main->castType<MyGUI::RotatingSkin>();
    rotatingSubskin->setCenter(MyGUI::IntPoint(16,16));
    float angle = std::atan2(x,y);
    rotatingSubskin->setAngle(angle);

    mLastDirectionX = x;
    mLastDirectionY = y;
}

void MapWindow::onDragStart(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
{
    if (_id!=MyGUI::MouseButton::Left) return;
    if (!mGlobal)
        mLastDragPos = MyGUI::IntPoint(_left, _top);
}

void MapWindow::onMouseDrag(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
{
    if (_id!=MyGUI::MouseButton::Left) return;

    if (!mGlobal)
    {
        MyGUI::IntPoint diff = MyGUI::IntPoint(_left, _top) - mLastDragPos;
        mLocalMap->setViewOffset( mLocalMap->getViewOffset() + diff );

        mLastDragPos = MyGUI::IntPoint(_left, _top);
    }
}

void MapWindow::onWorldButtonClicked(MyGUI::Widget* _sender)
{
    mGlobal = !mGlobal;
    mGlobalMap->setVisible(mGlobal);
    mLocalMap->setVisible(!mGlobal);

    mButton->setCaption( mGlobal ? "Local" : "World" );
}

