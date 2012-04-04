#include "layouts.hpp"

#include "../mwmechanics/mechanicsmanager.hpp"
#include "window_manager.hpp"

#include <cmath>
#include <algorithm>
#include <iterator>

#undef min
#undef max

using namespace MWGui;


HUD::HUD(int width, int height, int fpsLevel)
    : Layout("openmw_hud_layout.xml")
    , health(NULL)
    , magicka(NULL)
    , stamina(NULL)
    , weapImage(NULL)
    , spellImage(NULL)
    , weapStatus(NULL)
    , spellStatus(NULL)
    , effectBox(NULL)
    , effect1(NULL)
    , minimap(NULL)
    , compass(NULL)
    , crosshair(NULL)
    , fpsbox(NULL)
    , fpscounter(NULL)
    , trianglecounter(NULL)
    , batchcounter(NULL)
{
    setCoord(0,0, width, height);

    // Energy bars
    getWidget(health, "Health");
    getWidget(magicka, "Magicka");
    getWidget(stamina, "Stamina");

    // Item and spell images and status bars
    getWidget(weapImage, "WeapImage");
    getWidget(weapStatus, "WeapStatus");
    getWidget(spellImage, "SpellImage");
    getWidget(spellStatus, "SpellStatus");

    getWidget(effectBox, "EffectBox");
    getWidget(effect1, "Effect1");

    getWidget(minimap, "MiniMap");
    getWidget(compass, "Compass");

    getWidget(crosshair, "Crosshair");

    if ( fpsLevel == 2 ){
        getWidget(fpsbox, "FPSBoxAdv");
        fpsbox->setVisible(true);
        getWidget(fpscounter, "FPSCounterAdv");
    }else if ( fpsLevel == 1 ){
        getWidget(fpsbox, "FPSBox");
        fpsbox->setVisible(true);
        getWidget(fpscounter, "FPSCounter");
    }else{
        getWidget(fpscounter, "FPSCounter");
    }
    getWidget(trianglecounter, "TriangleCounter");
    getWidget(batchcounter, "BatchCounter");

    compass->setImageTexture("textures\\compass.dds");
    crosshair->setImageTexture("textures\\target.dds");

    // These are just demo values, you should replace these with
    // real calls from outside the class later.
    setWeapIcon("icons\\w\\tx_knife_iron.dds");
    setWeapStatus(90, 100);
    setSpellIcon("icons\\s\\b_tx_s_rstor_health.dds");
    setSpellStatus(65, 100);
    setEffect("icons\\s\\tx_s_chameleon.dds");

    LocalMapBase::init(minimap, this);
}

void HUD::setFPS(float fps)
{
    fpscounter->setCaption(boost::lexical_cast<std::string>((int)fps));
}

void HUD::setTriangleCount(size_t count)
{
    trianglecounter->setCaption(boost::lexical_cast<std::string>(count));
}

void HUD::setBatchCount(size_t count)
{
    batchcounter->setCaption(boost::lexical_cast<std::string>(count));
}

void HUD::setStats(int h, int hmax, int m, int mmax, int s, int smax)
{
    health->setProgressRange(hmax);
    health->setProgressPosition(h);
    magicka->setProgressRange(mmax);
    magicka->setProgressPosition(m);
    stamina->setProgressRange(smax);
    stamina->setProgressPosition(s);
}

void HUD::setWeapIcon(const char *str)
{
    weapImage->setImageTexture(str);
}

void HUD::setSpellIcon(const char *str)
{
    spellImage->setImageTexture(str);
}

void HUD::setWeapStatus(int s, int smax)
{
    weapStatus->setProgressRange(smax);
    weapStatus->setProgressPosition(s);
}

void HUD::setSpellStatus(int s, int smax)
{
    spellStatus->setProgressRange(smax);
    spellStatus->setProgressPosition(s);
}

void HUD::setEffect(const char *img)
{
    effect1->setImageTexture(img);
}

void HUD::setValue(const std::string& id, const MWMechanics::DynamicStat<int>& value)
{
    static const char *ids[] =
    {
        "HBar", "MBar", "FBar", 0
    };

    for (int i=0; ids[i]; ++i)
        if (ids[i]==id)
        {
            switch (i)
            {
                case 0:
                    health->setProgressRange (value.getModified());
                    health->setProgressPosition (value.getCurrent());
                    break;
                case 1:
                    magicka->setProgressRange (value.getModified());
                    magicka->setProgressPosition (value.getCurrent());
                    break;
                case 2:
                    stamina->setProgressRange (value.getModified());
                    stamina->setProgressPosition (value.getCurrent());
                    break;
            }
        }
}

void HUD::setPlayerDir(const float x, const float y)
{
    MyGUI::ISubWidget* main = compass->getSubWidgetMain();
    MyGUI::RotatingSkin* rotatingSubskin = main->castType<MyGUI::RotatingSkin>();
    rotatingSubskin->setCenter(MyGUI::IntPoint(16,16));
    float angle = std::atan2(x,y);
    rotatingSubskin->setAngle(angle);
}

void HUD::setPlayerPos(const float x, const float y)
{
    MyGUI::IntSize size = minimap->getCanvasSize();
    MyGUI::IntPoint middle = MyGUI::IntPoint((1/3.f + x/3.f)*size.width,(1/3.f + y/3.f)*size.height);
    MyGUI::IntCoord viewsize = minimap->getCoord();
    MyGUI::IntPoint pos(0.5*viewsize.width - middle.left, 0.5*viewsize.height - middle.top);

    minimap->setViewOffset(pos);
    compass->setPosition(MyGUI::IntPoint(x*512-16, y*512-16));
}

MapWindow::MapWindow()
  : Layout("openmw_map_window_layout.xml")
  , mGlobal(false)
  , mVisible(false)
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

void MapWindow::setVisible(bool b)
{
    mMainWidget->setVisible(b);
    if (b)
        mVisible = true;
    else
        mVisible = false;
}

void MapWindow::setCellName(const std::string& cellName)
{
    static_cast<MyGUI::Window*>(mMainWidget)->setCaption(cellName);
    adjustWindowCaption();
}

void MapWindow::setPlayerPos(const float x, const float y)
{
    if (mGlobal || mVisible) return;
    MyGUI::IntSize size = mLocalMap->getCanvasSize();
    MyGUI::IntPoint middle = MyGUI::IntPoint((1/3.f + x/3.f)*size.width,(1/3.f + y/3.f)*size.height);
    MyGUI::IntCoord viewsize = mLocalMap->getCoord();
    MyGUI::IntPoint pos(0.5*viewsize.width - middle.left, 0.5*viewsize.height - middle.top);
    mLocalMap->setViewOffset(pos);

    mPlayerArrow->setPosition(MyGUI::IntPoint(x*512-16, y*512-16));
}

void MapWindow::setPlayerDir(const float x, const float y)
{
    if (!mVisible) return;
    MyGUI::ISubWidget* main = mPlayerArrow->getSubWidgetMain();
    MyGUI::RotatingSkin* rotatingSubskin = main->castType<MyGUI::RotatingSkin>();
    rotatingSubskin->setCenter(MyGUI::IntPoint(16,16));
    float angle = std::atan2(x,y);
    rotatingSubskin->setAngle(angle);
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

LocalMapBase::LocalMapBase()
    : mCurX(0)
    , mCurY(0)
    , mInterior(false)
    , mFogOfWar(true)
    , mLocalMap(NULL)
    , mPrefix()
    , mChanged(true)
    , mLayout(NULL)
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

