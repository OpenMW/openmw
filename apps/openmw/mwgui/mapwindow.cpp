#include "mapwindow.hpp"

#include <boost/lexical_cast.hpp>

#include <OgreSceneNode.h>
#include <OgreVector2.h>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwrender/globalmap.hpp"

#include "../components/esm/globalmap.hpp"

#include "widgets.hpp"

namespace MWGui
{

    LocalMapBase::LocalMapBase()
        : mCurX(0)
        , mCurY(0)
        , mInterior(false)
        , mFogOfWar(true)
        , mLocalMap(NULL)
        , mMapDragAndDrop(false)
        , mPrefix()
        , mChanged(true)
        , mLayout(NULL)
        , mLastPositionX(0.0f)
        , mLastPositionY(0.0f)
        , mLastDirectionX(0.0f)
        , mLastDirectionY(0.0f)
        , mCompass(NULL)
        , mMarkerUpdateTimer(0.0f)
    {
    }

    LocalMapBase::~LocalMapBase()
    {
        // Clear our "lost focus" delegate for marker widgets first, otherwise it will
        // fire when the widget is about to be destroyed and the mouse cursor is over it.
        // At that point, other widgets may already be destroyed, so applyFogOfWar (which is called by the delegate) would crash.
        for (std::vector<MyGUI::Widget*>::iterator it = mDoorMarkerWidgets.begin(); it != mDoorMarkerWidgets.end(); ++it)
            (*it)->eventMouseLostFocus.clear();
        for (std::vector<MyGUI::Widget*>::iterator it = mMarkerWidgets.begin(); it != mMarkerWidgets.end(); ++it)
            (*it)->eventMouseLostFocus.clear();
    }

    void LocalMapBase::init(MyGUI::ScrollView* widget, MyGUI::ImageBox* compass, OEngine::GUI::Layout* layout, bool mapDragAndDrop)
    {
        mLocalMap = widget;
        mLayout = layout;
        mMapDragAndDrop = mapDragAndDrop;
        mCompass = compass;

        // create 3x3 map widgets, 512x512 each, holding a 1024x1024 texture each
        const int widgetSize = 512;
        for (int mx=0; mx<3; ++mx)
        {
            for (int my=0; my<3; ++my)
            {
                MyGUI::ImageBox* map = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                    MyGUI::IntCoord(mx*widgetSize, my*widgetSize, widgetSize, widgetSize),
                    MyGUI::Align::Top | MyGUI::Align::Left);

                MyGUI::ImageBox* fog = map->createWidget<MyGUI::ImageBox>("ImageBox",
                    MyGUI::IntCoord(0, 0, widgetSize, widgetSize),
                    MyGUI::Align::Top | MyGUI::Align::Left);

                if (!mMapDragAndDrop)
                {
                    map->setNeedMouseFocus(false);
                    fog->setNeedMouseFocus(false);
                }

                mMapWidgets.push_back(map);
                mFogWidgets.push_back(fog);
            }
        }
    }

    void LocalMapBase::setCellPrefix(const std::string& prefix)
    {
        mPrefix = prefix;
        mChanged = true;
    }

    bool LocalMapBase::toggleFogOfWar()
    {
        mFogOfWar = !mFogOfWar;
        applyFogOfWar();
        return mFogOfWar;
    }

    void LocalMapBase::applyFogOfWar()
    {
        for (int mx=0; mx<3; ++mx)
        {
            for (int my=0; my<3; ++my)
            {
                std::string image = mPrefix+"_"+ boost::lexical_cast<std::string>(mCurX + (mx-1)) + "_"
                        + boost::lexical_cast<std::string>(mCurY + (-1*(my-1)));
                MyGUI::ImageBox* fog = mFogWidgets[my + 3*mx];
                fog->setImageTexture(mFogOfWar ?
                    ((MyGUI::RenderManager::getInstance().getTexture(image+"_fog") != 0) ? image+"_fog"
                    : "black.png" )
                   : "");
            }
        }
        notifyMapChanged ();
    }

    void LocalMapBase::onMarkerFocused (MyGUI::Widget* w1, MyGUI::Widget* w2)
    {
        // Workaround to not make the marker visible if it's under fog of war
        applyFogOfWar ();
    }

    void LocalMapBase::onMarkerUnfocused (MyGUI::Widget* w1, MyGUI::Widget* w2)
    {
        // Workaround to not make the marker visible if it's under fog of war
        applyFogOfWar ();
    }

    MyGUI::IntPoint LocalMapBase::getMarkerPosition(float worldX, float worldY, MarkerPosition& markerPos)
    {
        MyGUI::IntPoint widgetPos;
        // normalized cell coordinates
        float nX,nY;

        markerPos.interior = mInterior;

        if (!mInterior)
        {
            int cellX, cellY;
            MWBase::Environment::get().getWorld()->positionToIndex(worldX, worldY, cellX, cellY);
            const int cellSize = 8192;
            nX = (worldX - cellSize * cellX) / cellSize;
            // Image space is -Y up, cells are Y up
            nY = 1 - (worldY - cellSize * cellY) / cellSize;

            float cellDx = cellX - mCurX;
            float cellDy = cellY - mCurY;

            markerPos.cellX = cellX;
            markerPos.cellY = cellY;

            widgetPos = MyGUI::IntPoint(nX * 512 + (1+cellDx) * 512,
                                        nY * 512 - (cellDy-1) * 512);
        }
        else
        {
            int cellX, cellY;
            Ogre::Vector2 worldPos (worldX, worldY);
            MWBase::Environment::get().getWorld ()->getInteriorMapPosition (worldPos, nX, nY, cellX, cellY);

            markerPos.cellX = cellX;
            markerPos.cellY = cellY;

            // Image space is -Y up, cells are Y up
            widgetPos = MyGUI::IntPoint(nX * 512 + (1+(cellX-mCurX)) * 512,
                                        nY * 512 + (1-(cellY-mCurY)) * 512);
        }

        markerPos.nX = nX;
        markerPos.nY = nY;
        return widgetPos;
    }

    void LocalMapBase::setActiveCell(const int x, const int y, bool interior)
    {
        if (x==mCurX && y==mCurY && mInterior==interior && !mChanged)
            return; // don't do anything if we're still in the same cell

        mCurX = x;
        mCurY = y;
        mInterior = interior;
        mChanged = false;

        // clear all previous door markers
        for (std::vector<MyGUI::Widget*>::iterator it = mDoorMarkerWidgets.begin(); it != mDoorMarkerWidgets.end(); ++it)
            MyGUI::Gui::getInstance().destroyWidget(*it);
        mDoorMarkerWidgets.clear();

        // Update the map textures
        for (int mx=0; mx<3; ++mx)
        {
            for (int my=0; my<3; ++my)
            {
                // map
                std::string image = mPrefix+"_"+ boost::lexical_cast<std::string>(x + (mx-1)) + "_"
                        + boost::lexical_cast<std::string>(y + (-1*(my-1)));

                MyGUI::ImageBox* box = mMapWidgets[my + 3*mx];

                if (MyGUI::RenderManager::getInstance().getTexture(image) != 0)
                    box->setImageTexture(image);
                else
                    box->setImageTexture("black.png");
            }
        }

        MWBase::World* world = MWBase::Environment::get().getWorld();

        // Retrieve the door markers we want to show
        std::vector<MWBase::World::DoorMarker> doors;
        if (interior)
        {
            MWWorld::CellStore* cell = world->getInterior (mPrefix);
            world->getDoorMarkers(cell, doors);
        }
        else
        {
            for (int dX=-1; dX<2; ++dX)
            {
                for (int dY=-1; dY<2; ++dY)
                {
                    MWWorld::CellStore* cell = world->getExterior (mCurX+dX, mCurY+dY);
                    world->getDoorMarkers(cell, doors);
                }
            }
        }

        // Create a widget for each marker
        int counter = 0;
        for (std::vector<MWBase::World::DoorMarker>::iterator it = doors.begin(); it != doors.end(); ++it)
        {
            MWBase::World::DoorMarker marker = *it;

            MarkerPosition markerPos;
            MyGUI::IntPoint widgetPos = getMarkerPosition(marker.x, marker.y, markerPos);
            MyGUI::IntCoord widgetCoord(widgetPos.left - 4,
                                        widgetPos.top - 4,
                                        8, 8);
            ++counter;
            MyGUI::Button* markerWidget = mLocalMap->createWidget<MyGUI::Button>("ButtonImage",
                widgetCoord, MyGUI::Align::Default);
            markerWidget->setImageResource("DoorMarker");
            markerWidget->setUserString("ToolTipType", "Layout");
            markerWidget->setUserString("ToolTipLayout", "TextToolTipOneLine");
            markerWidget->setUserString("Caption_TextOneLine", marker.name);
            markerWidget->eventMouseSetFocus += MyGUI::newDelegate(this, &LocalMapBase::onMarkerFocused);
            markerWidget->eventMouseLostFocus += MyGUI::newDelegate(this, &LocalMapBase::onMarkerUnfocused);
            // Used by tooltips to not show the tooltip if marker is hidden by fog of war
            markerWidget->setUserString("IsMarker", "true");
            markerWidget->setUserData(markerPos);

            mDoorMarkerWidgets.push_back(markerWidget);
        }

        updateMarkers();

        applyFogOfWar();

        // set the compass texture again, because MyGUI determines sorting of ImageBox widgets
        // based on the last setImageTexture call
        std::string tex = "textures\\compass.dds";
        mCompass->setImageTexture("");
        mCompass->setImageTexture(tex);
    }


    void LocalMapBase::setPlayerPos(const float x, const float y)
    {
        updateMarkers();

        if (x == mLastPositionX && y == mLastPositionY)
            return;

        notifyPlayerUpdate ();

        MyGUI::IntSize size = mLocalMap->getCanvasSize();
        MyGUI::IntPoint middle = MyGUI::IntPoint((1/3.f + x/3.f)*size.width,(1/3.f + y/3.f)*size.height);
        MyGUI::IntCoord viewsize = mLocalMap->getCoord();
        MyGUI::IntPoint pos(0.5*viewsize.width - middle.left, 0.5*viewsize.height - middle.top);
        mLocalMap->setViewOffset(pos);

        mCompass->setPosition(MyGUI::IntPoint(512+x*512-16, 512+y*512-16));
        mLastPositionX = x;
        mLastPositionY = y;
    }

    void LocalMapBase::setPlayerDir(const float x, const float y)
    {
        if (x == mLastDirectionX && y == mLastDirectionY)
            return;

        notifyPlayerUpdate ();

        MyGUI::ISubWidget* main = mCompass->getSubWidgetMain();
        MyGUI::RotatingSkin* rotatingSubskin = main->castType<MyGUI::RotatingSkin>();
        rotatingSubskin->setCenter(MyGUI::IntPoint(16,16));
        float angle = std::atan2(x,y);
        rotatingSubskin->setAngle(angle);

        mLastDirectionX = x;
        mLastDirectionY = y;
    }

    void LocalMapBase::addDetectionMarkers(int type)
    {
        std::vector<MWWorld::Ptr> markers;
        MWBase::World* world = MWBase::Environment::get().getWorld();
        world->listDetectedReferences(
                    world->getPlayerPtr(),
                    markers, MWBase::World::DetectionType(type));
        if (markers.empty())
            return;

        std::string markerTexture;
        MyGUI::Colour markerColour;
        if (type == MWBase::World::Detect_Creature)
        {
            markerTexture = "textures\\menu_map_dcreature.dds";
            markerColour = MyGUI::Colour(1,0,0,1);
        }
        if (type == MWBase::World::Detect_Key)
        {
            markerTexture = "textures\\menu_map_dkey.dds";
            markerColour = MyGUI::Colour(0,1,0,1);
        }
        if (type == MWBase::World::Detect_Enchantment)
        {
            markerTexture = "textures\\menu_map_dmagic.dds";
            markerColour = MyGUI::Colour(0,0,1,1);
        }

        int counter = 0;
        for (std::vector<MWWorld::Ptr>::iterator it = markers.begin(); it != markers.end(); ++it)
        {
            const ESM::Position& worldPos = it->getRefData().getPosition();
            MarkerPosition markerPos;
            MyGUI::IntPoint widgetPos = getMarkerPosition(worldPos.pos[0], worldPos.pos[1], markerPos);
            MyGUI::IntCoord widgetCoord(widgetPos.left - 4,
                                        widgetPos.top - 4,
                                        8, 8);
            ++counter;
            MyGUI::ImageBox* markerWidget = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                widgetCoord, MyGUI::Align::Default);
            markerWidget->setImageTexture(markerTexture);
            markerWidget->setUserString("IsMarker", "true");
            markerWidget->setUserData(markerPos);
            markerWidget->setColour(markerColour);
            mMarkerWidgets.push_back(markerWidget);
        }
    }

    void LocalMapBase::onFrame(float dt)
    {
        mMarkerUpdateTimer += dt;

        if (mMarkerUpdateTimer >= 0.25)
        {
            mMarkerUpdateTimer = 0;
            updateMarkers();
        }
    }

    void LocalMapBase::updateMarkers()
    {
        // clear all previous markers
        for (std::vector<MyGUI::Widget*>::iterator it = mMarkerWidgets.begin(); it != mMarkerWidgets.end(); ++it)
            MyGUI::Gui::getInstance().destroyWidget(*it);
        mMarkerWidgets.clear();

        addDetectionMarkers(MWBase::World::Detect_Creature);
        addDetectionMarkers(MWBase::World::Detect_Key);
        addDetectionMarkers(MWBase::World::Detect_Enchantment);

        // Add marker for the spot marked with Mark magic effect
        MWWorld::CellStore* markedCell = NULL;
        ESM::Position markedPosition;
        MWBase::Environment::get().getWorld()->getPlayer().getMarkedPosition(markedCell, markedPosition);
        if (markedCell && markedCell->isExterior() == !mInterior
                && (!mInterior || Misc::StringUtils::ciEqual(markedCell->getCell()->mName, mPrefix)))
        {
            MarkerPosition markerPos;
            MyGUI::IntPoint widgetPos = getMarkerPosition(markedPosition.pos[0], markedPosition.pos[1], markerPos);
            MyGUI::IntCoord widgetCoord(widgetPos.left - 4,
                                        widgetPos.top - 4,
                                        8, 8);
            MyGUI::ImageBox* markerWidget = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                widgetCoord, MyGUI::Align::Default);
            markerWidget->setImageTexture("textures\\menu_map_smark.dds");
            markerWidget->setUserString("IsMarker", "true");
            markerWidget->setUserData(markerPos);
            mMarkerWidgets.push_back(markerWidget);
        }
    }

    // ------------------------------------------------------------------------------------------

    MapWindow::MapWindow(DragAndDrop* drag, const std::string& cacheDir)
        : WindowPinnableBase("openmw_map_window.layout")
        , NoDrop(drag, mMainWidget)
        , mGlobal(false)
        , mGlobalMap(0)
        , mGlobalMapRender(0)
    {
        setCoord(500,0,320,300);

        getWidget(mLocalMap, "LocalMap");
        getWidget(mGlobalMap, "GlobalMap");
        getWidget(mGlobalMapImage, "GlobalMapImage");
        getWidget(mGlobalMapOverlay, "GlobalMapOverlay");
        getWidget(mPlayerArrowLocal, "CompassLocal");
        getWidget(mPlayerArrowGlobal, "CompassGlobal");

        mGlobalMap->setVisible (false);

        getWidget(mButton, "WorldButton");
        mButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MapWindow::onWorldButtonClicked);
        mButton->setCaptionWithReplacing("#{sWorld}");

        getWidget(mEventBoxGlobal, "EventBoxGlobal");
        mEventBoxGlobal->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        mEventBoxGlobal->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);
        getWidget(mEventBoxLocal, "EventBoxLocal");
        mEventBoxLocal->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        mEventBoxLocal->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);

        LocalMapBase::init(mLocalMap, mPlayerArrowLocal, this);
    }

    void MapWindow::renderGlobalMap(Loading::Listener* loadingListener)
    {
        mGlobalMapRender = new MWRender::GlobalMap("");
        mGlobalMapRender->render(loadingListener);
        mGlobalMap->setCanvasSize (mGlobalMapRender->getWidth(), mGlobalMapRender->getHeight());
        mGlobalMapImage->setSize(mGlobalMapRender->getWidth(), mGlobalMapRender->getHeight());

        mGlobalMapImage->setImageTexture("GlobalMap.png");
        mGlobalMapOverlay->setImageTexture("GlobalMapOverlay");
    }

    MapWindow::~MapWindow()
    {
        delete mGlobalMapRender;
    }

    void MapWindow::setCellName(const std::string& cellName)
    {
        setTitle("#{sCell=" + cellName + "}");
    }

    void MapWindow::addVisitedLocation(const std::string& name, int x, int y)
    {
        float worldX, worldY;
        mGlobalMapRender->cellTopLeftCornerToImageSpace (x, y, worldX, worldY);

        MyGUI::IntCoord widgetCoord(
                    worldX * mGlobalMapRender->getWidth()+6,
                    worldY * mGlobalMapRender->getHeight()+6,
                    12, 12);

        static int _counter=0;
        MyGUI::Button* markerWidget = mGlobalMapOverlay->createWidget<MyGUI::Button>("ButtonImage",
            widgetCoord, MyGUI::Align::Default);
        markerWidget->setImageResource("DoorMarker");
        markerWidget->setUserString("ToolTipType", "Layout");
        markerWidget->setUserString("ToolTipLayout", "TextToolTipOneLine");
        markerWidget->setUserString("Caption_TextOneLine", name);
        ++_counter;

        markerWidget = mEventBoxGlobal->createWidget<MyGUI::Button>("",
            widgetCoord, MyGUI::Align::Default);
        markerWidget->setNeedMouseFocus (true);
        markerWidget->setUserString("ToolTipType", "Layout");
        markerWidget->setUserString("ToolTipLayout", "TextToolTipOneLine");
        markerWidget->setUserString("Caption_TextOneLine", name);

        CellId cell;
        cell.first = x;
        cell.second = y;
        mMarkers.push_back(cell);
    }

    void MapWindow::cellExplored(int x, int y)
    {
        mQueuedToExplore.push_back(std::make_pair(x,y));
    }

    void MapWindow::onFrame(float dt)
    {
        LocalMapBase::onFrame(dt);

        for (std::vector<CellId>::iterator it = mQueuedToExplore.begin(); it != mQueuedToExplore.end(); ++it)
        {
            mGlobalMapRender->exploreCell(it->first, it->second);
        }
        mQueuedToExplore.clear();

        NoDrop::onFrame(dt);
    }

    void MapWindow::onDragStart(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        if (_id!=MyGUI::MouseButton::Left) return;
        mLastDragPos = MyGUI::IntPoint(_left, _top);
    }

    void MapWindow::onMouseDrag(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        if (_id!=MyGUI::MouseButton::Left) return;

        MyGUI::IntPoint diff = MyGUI::IntPoint(_left, _top) - mLastDragPos;

        if (!mGlobal)
            mLocalMap->setViewOffset( mLocalMap->getViewOffset() + diff );
        else
            mGlobalMap->setViewOffset( mGlobalMap->getViewOffset() + diff );

        mLastDragPos = MyGUI::IntPoint(_left, _top);
    }

    void MapWindow::onWorldButtonClicked(MyGUI::Widget* _sender)
    {
        mGlobal = !mGlobal;
        mGlobalMap->setVisible(mGlobal);
        mLocalMap->setVisible(!mGlobal);

        mButton->setCaptionWithReplacing( mGlobal ? "#{sLocal}" :
                "#{sWorld}");

        if (mGlobal)
            globalMapUpdatePlayer ();
    }

    void MapWindow::onPinToggled()
    {
        MWBase::Environment::get().getWindowManager()->setMinimapVisibility(!mPinned);
    }

    void MapWindow::open()
    {
        // force markers to foreground
        for (unsigned int i=0; i<mGlobalMapOverlay->getChildCount (); ++i)
        {
            if (mGlobalMapOverlay->getChildAt (i)->getName().substr(0,4) == "Door")
                mGlobalMapOverlay->getChildAt (i)->castType<MyGUI::Button>()->setImageResource("DoorMarker");
        }

        globalMapUpdatePlayer();

        mPlayerArrowGlobal->setImageTexture ("textures\\compass.dds");
    }

    void MapWindow::globalMapUpdatePlayer ()
    {
        // For interiors, position is set by WindowManager via setGlobalMapPlayerPosition
        if (MWBase::Environment::get().getWorld ()->isCellExterior ())
        {
            Ogre::Vector3 pos = MWBase::Environment::get().getWorld ()->getPlayerPtr().getRefData ().getBaseNode ()->_getDerivedPosition ();
            Ogre::Quaternion orient = MWBase::Environment::get().getWorld ()->getPlayerPtr().getRefData ().getBaseNode ()->_getDerivedOrientation ();
            Ogre::Vector2 dir (orient.yAxis ().x, orient.yAxis().y);

            float worldX, worldY;
            mGlobalMapRender->worldPosToImageSpace (pos.x, pos.y, worldX, worldY);
            worldX *= mGlobalMapRender->getWidth();
            worldY *= mGlobalMapRender->getHeight();

            mPlayerArrowGlobal->setPosition(MyGUI::IntPoint(worldX - 16, worldY - 16));

            MyGUI::ISubWidget* main = mPlayerArrowGlobal->getSubWidgetMain();
            MyGUI::RotatingSkin* rotatingSubskin = main->castType<MyGUI::RotatingSkin>();
            rotatingSubskin->setCenter(MyGUI::IntPoint(16,16));
            float angle = std::atan2(dir.x, dir.y);
            rotatingSubskin->setAngle(angle);

            // set the view offset so that player is in the center
            MyGUI::IntSize viewsize = mGlobalMap->getSize();
            MyGUI::IntPoint viewoffs(0.5*viewsize.width - worldX, 0.5*viewsize.height - worldY);
            mGlobalMap->setViewOffset(viewoffs);
        }
    }

    void MapWindow::notifyPlayerUpdate ()
    {
        globalMapUpdatePlayer ();
    }

    void MapWindow::notifyMapChanged ()
    {
        // workaround to prevent the map from drawing on top of the button
        MyGUI::IntCoord oldCoord = mButton->getCoord ();
        MyGUI::Gui::getInstance().destroyWidget (mButton);
        mButton = mMainWidget->createWidget<MWGui::Widgets::AutoSizedButton>("MW_Button",
             oldCoord, MyGUI::Align::Bottom | MyGUI::Align::Right);
        mButton->setProperty ("ExpandDirection", "Left");

        mButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MapWindow::onWorldButtonClicked);
        mButton->setCaptionWithReplacing( mGlobal ? "#{sLocal}" :
                "#{sWorld}");
    }

    void MapWindow::setGlobalMapPlayerPosition(float worldX, float worldY)
    {
        float x, y;
        mGlobalMapRender->worldPosToImageSpace (worldX, worldY, x, y);
        x *= mGlobalMapRender->getWidth();
        y *= mGlobalMapRender->getHeight();

        mPlayerArrowGlobal->setPosition(MyGUI::IntPoint(x - 16, y - 16));

        // set the view offset so that player is in the center
        MyGUI::IntSize viewsize = mGlobalMap->getSize();
        MyGUI::IntPoint viewoffs(0.5*viewsize.width - x, 0.5*viewsize.height - y);
        mGlobalMap->setViewOffset(viewoffs);
    }

    void MapWindow::clear()
    {
        mMarkers.clear();
        mGlobalMapRender->clear();

        while (mEventBoxGlobal->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mEventBoxGlobal->getChildAt(0));
        while (mGlobalMapOverlay->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mGlobalMapOverlay->getChildAt(0));
    }

    void MapWindow::write(ESM::ESMWriter &writer, Loading::Listener& progress)
    {
        ESM::GlobalMap map;
        mGlobalMapRender->write(map);

        map.mMarkers = mMarkers;

        writer.startRecord(ESM::REC_GMAP);
        map.save(writer);
        writer.endRecord(ESM::REC_GMAP);
        progress.increaseProgress();
    }

    void MapWindow::readRecord(ESM::ESMReader &reader, int32_t type)
    {
        if (type == ESM::REC_GMAP)
        {
            ESM::GlobalMap map;
            map.load(reader);

            mGlobalMapRender->read(map);

            for (std::vector<ESM::GlobalMap::CellId>::iterator it = map.mMarkers.begin(); it != map.mMarkers.end(); ++it)
            {
                const ESM::Cell* cell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Cell>().search(it->first, it->second);
                if (cell && !cell->mName.empty())
                    addVisitedLocation(cell->mName, it->first, it->second);
            }
        }
    }
}
