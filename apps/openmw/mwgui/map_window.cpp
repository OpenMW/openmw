#include "map_window.hpp"
#include "window_manager.hpp"

#include <boost/lexical_cast.hpp>

using namespace MWGui;

LocalMapBase::LocalMapBase()
    : mCurX(0)
    , mCurY(0)
    , mInterior(false)
    , mFogOfWar(true)
    , mLocalMap(NULL)
    , mPrefix()
    , mChanged(true)
    , mLayout(NULL)
    , mLastPositionX(0.0f)
    , mLastPositionY(0.0f)
    , mLastDirectionX(0.0f)
    , mLastDirectionY(0.0f)
{
}

void LocalMapBase::init(MyGUI::ScrollView* widget, OEngine::GUI::Layout* layout)
{
    mLocalMap = widget;
    mLayout = layout;
}

void LocalMapBase::setCellPrefix(const std::string& prefix)
{
    mPrefix = prefix;
    mChanged = true;
}

void LocalMapBase::toggleFogOfWar()
{
    mFogOfWar = !mFogOfWar;
    applyFogOfWar();
}

void LocalMapBase::applyFogOfWar()
{
    for (int mx=0; mx<3; ++mx)
    {
        for (int my=0; my<3; ++my)
        {
            std::string name = "Map_" + boost::lexical_cast<std::string>(mx) + "_"
                    + boost::lexical_cast<std::string>(my);
            std::string image = mPrefix+"_"+ boost::lexical_cast<std::string>(mCurX + (mx-1)) + "_"
                    + boost::lexical_cast<std::string>(mCurY + (mInterior ? (my-1) : -1*(my-1)));
            MyGUI::ImageBox* fog;
            mLayout->getWidget(fog, name+"_fog");
            fog->setImageTexture(mFogOfWar ?
                ((MyGUI::RenderManager::getInstance().getTexture(image+"_fog") != 0) ? image+"_fog"
                : "black.png" )
               : "");
        }
    }
}

void LocalMapBase::setActiveCell(const int x, const int y, bool interior)
{
    if (x==mCurX && y==mCurY && mInterior==interior && !mChanged) return; // don't do anything if we're still in the same cell
    for (int mx=0; mx<3; ++mx)
    {
        for (int my=0; my<3; ++my)
        {
            std::string name = "Map_" + boost::lexical_cast<std::string>(mx) + "_"
                    + boost::lexical_cast<std::string>(my);

            std::string image = mPrefix+"_"+ boost::lexical_cast<std::string>(x + (mx-1)) + "_"
                    + boost::lexical_cast<std::string>(y + (interior ? (my-1) : -1*(my-1)));

            MyGUI::ImageBox* box;
            mLayout->getWidget(box, name);

            if (MyGUI::RenderManager::getInstance().getTexture(image) != 0)
                box->setImageTexture(image);
            else
                box->setImageTexture("black.png");
        }
    }
    mInterior = interior;
    mCurX = x;
    mCurY = y;
    mChanged = false;
    applyFogOfWar();
}

// ------------------------------------------------------------------------------------------

MapWindow::MapWindow(WindowManager& parWindowManager) : 
    MWGui::WindowPinnableBase("openmw_map_window_layout.xml", parWindowManager),
    mGlobal(false)
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
    mButton->setCaption(mWindowManager.getGameSettingString("sWorld", ""));
    int width = mButton->getTextSize().width + 24;
    mButton->setCoord(mMainWidget->getSize().width - width - 22, mMainWidget->getSize().height - 64, width, 22);

    MyGUI::Button* eventbox;
    getWidget(eventbox, "EventBox");
    eventbox->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
    eventbox->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);

    LocalMapBase::init(mLocalMap, this);
}

void MapWindow::setCellName(const std::string& cellName)
{
    setTitle(cellName);
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

    mButton->setCaption( mGlobal ? mWindowManager.getGameSettingString("sWorld", "") :
            mWindowManager.getGameSettingString("sLocal", ""));
    int width = mButton->getTextSize().width + 24;
    mButton->setCoord(mMainWidget->getSize().width - width - 22, mMainWidget->getSize().height - 64, width, 22);
}

void MapWindow::onPinToggled()
{
    mWindowManager.setMinimapVisibility(!mPinned);
}
