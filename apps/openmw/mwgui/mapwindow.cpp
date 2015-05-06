#include "mapwindow.hpp"

#include <OgreSceneNode.h>
#include <OgreVector2.h>

#include <MyGUI_ScrollView.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_Gui.h>
#include <MyGUI_LanguageManager.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_RotatingSkin.h>
#include <MyGUI_FactoryManager.h>

#include <components/esm/globalmap.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/environment.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"

#include "../mwrender/globalmap.hpp"

#include "widgets.hpp"
#include "confirmationdialog.hpp"
#include "tooltips.hpp"

namespace
{

    const int cellSize = 8192;

    enum LocalMapWidgetDepth
    {
        Local_CompassLayer = 0,
        Local_MarkerAboveFogLayer = 1,
        Local_FogLayer = 2,
        Local_MarkerLayer = 3,
        Local_MapLayer = 4
    };

    enum GlobalMapWidgetDepth
    {
        Global_CompassLayer = 0,
        Global_MarkerLayer = 1,
        Global_ExploreOverlayLayer = 2,
        Global_MapLayer = 3
    };


    /// @brief A widget that changes its color when hovered.
    class MarkerWidget: public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(MarkerWidget)

    public:
        void setNormalColour(const MyGUI::Colour& colour)
        {
            mNormalColour = colour;
            setColour(colour);
        }

        void setHoverColour(const MyGUI::Colour& colour)
        {
            mHoverColour = colour;
        }

    private:
        MyGUI::Colour mNormalColour;
        MyGUI::Colour mHoverColour;

        void onMouseLostFocus(MyGUI::Widget* _new)
        {
            setColour(mNormalColour);
        }

        void onMouseSetFocus(MyGUI::Widget* _old)
        {
            setColour(mHoverColour);
        }
    };
}

namespace MWGui
{

    void CustomMarkerCollection::addMarker(const ESM::CustomMarker &marker, bool triggerEvent)
    {
        mMarkers.push_back(marker);
        if (triggerEvent)
            eventMarkersChanged();
    }

    void CustomMarkerCollection::deleteMarker(const ESM::CustomMarker &marker)
    {
        std::vector<ESM::CustomMarker>::iterator it = std::find(mMarkers.begin(), mMarkers.end(), marker);
        if (it != mMarkers.end())
            mMarkers.erase(it);
        else
            throw std::runtime_error("can't find marker to delete");

        eventMarkersChanged();
    }

    void CustomMarkerCollection::updateMarker(const ESM::CustomMarker &marker, const std::string &newNote)
    {
        std::vector<ESM::CustomMarker>::iterator it = std::find(mMarkers.begin(), mMarkers.end(), marker);
        if (it != mMarkers.end())
            it->mNote = newNote;
        else
            throw std::runtime_error("can't find marker to update");

        eventMarkersChanged();
    }

    void CustomMarkerCollection::clear()
    {
        mMarkers.clear();
        eventMarkersChanged();
    }

    std::vector<ESM::CustomMarker>::const_iterator CustomMarkerCollection::begin() const
    {
        return mMarkers.begin();
    }

    std::vector<ESM::CustomMarker>::const_iterator CustomMarkerCollection::end() const
    {
        return mMarkers.end();
    }

    size_t CustomMarkerCollection::size() const
    {
        return mMarkers.size();
    }

    // ------------------------------------------------------

    LocalMapBase::LocalMapBase(CustomMarkerCollection &markers)
        : mCurX(0)
        , mCurY(0)
        , mInterior(false)
        , mFogOfWar(true)
        , mLocalMap(NULL)
        , mPrefix()
        , mChanged(true)
        , mLastDirectionX(0.0f)
        , mLastDirectionY(0.0f)
        , mCompass(NULL)
        , mMarkerUpdateTimer(0.0f)
        , mCustomMarkers(markers)
        , mMapWidgetSize(0)
    {
        mCustomMarkers.eventMarkersChanged += MyGUI::newDelegate(this, &LocalMapBase::updateCustomMarkers);
    }

    LocalMapBase::~LocalMapBase()
    {
        mCustomMarkers.eventMarkersChanged -= MyGUI::newDelegate(this, &LocalMapBase::updateCustomMarkers);
    }

    void LocalMapBase::init(MyGUI::ScrollView* widget, MyGUI::ImageBox* compass, int mapWidgetSize)
    {
        mLocalMap = widget;
        mCompass = compass;
        mMapWidgetSize = mapWidgetSize;

        mLocalMap->setCanvasSize(mMapWidgetSize*3, mMapWidgetSize*3);

        mCompass->setDepth(Local_CompassLayer);
        mCompass->setNeedMouseFocus(false);

        for (int mx=0; mx<3; ++mx)
        {
            for (int my=0; my<3; ++my)
            {
                MyGUI::ImageBox* map = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                    MyGUI::IntCoord(mx*mMapWidgetSize, my*mMapWidgetSize, mMapWidgetSize, mMapWidgetSize),
                    MyGUI::Align::Top | MyGUI::Align::Left);
                map->setDepth(Local_MapLayer);

                MyGUI::ImageBox* fog = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                    MyGUI::IntCoord(mx*mMapWidgetSize, my*mMapWidgetSize, mMapWidgetSize, mMapWidgetSize),
                    MyGUI::Align::Top | MyGUI::Align::Left);
                fog->setDepth(Local_FogLayer);

                map->setNeedMouseFocus(false);
                fog->setNeedMouseFocus(false);

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
                std::string image = mPrefix+"_"+ MyGUI::utility::toString(mCurX + (mx-1)) + "_"
                        + MyGUI::utility::toString(mCurY + (-1*(my-1)));
                MyGUI::ImageBox* fog = mFogWidgets[my + 3*mx];
                fog->setImageTexture(mFogOfWar ?
                    ((MyGUI::RenderManager::getInstance().getTexture(image+"_fog") != 0) ? image+"_fog"
                    : "black.png" )
                   : "");
            }
        }
        redraw();
    }

    MyGUI::IntPoint LocalMapBase::getMarkerPosition(float worldX, float worldY, MarkerUserData& markerPos)
    {
        MyGUI::IntPoint widgetPos;
        // normalized cell coordinates
        float nX,nY;

        markerPos.interior = mInterior;

        if (!mInterior)
        {
            int cellX, cellY;
            MWBase::Environment::get().getWorld()->positionToIndex(worldX, worldY, cellX, cellY);
            nX = (worldX - cellSize * cellX) / cellSize;
            // Image space is -Y up, cells are Y up
            nY = 1 - (worldY - cellSize * cellY) / cellSize;

            float cellDx = static_cast<float>(cellX - mCurX);
            float cellDy = static_cast<float>(cellY - mCurY);

            markerPos.cellX = cellX;
            markerPos.cellY = cellY;

            widgetPos = MyGUI::IntPoint(static_cast<int>(nX * mMapWidgetSize + (1 + cellDx) * mMapWidgetSize),
                                        static_cast<int>(nY * mMapWidgetSize - (cellDy-1) * mMapWidgetSize));
        }
        else
        {
            int cellX, cellY;
            Ogre::Vector2 worldPos (worldX, worldY);
            MWBase::Environment::get().getWorld ()->worldToInteriorMapPosition (worldPos, nX, nY, cellX, cellY);

            markerPos.cellX = cellX;
            markerPos.cellY = cellY;

            // Image space is -Y up, cells are Y up
            widgetPos = MyGUI::IntPoint(static_cast<int>(nX * mMapWidgetSize + (1 + (cellX - mCurX)) * mMapWidgetSize),
                                        static_cast<int>(nY * mMapWidgetSize + (1-(cellY-mCurY)) * mMapWidgetSize));
        }

        markerPos.nX = nX;
        markerPos.nY = nY;
        return widgetPos;
    }

    void LocalMapBase::updateCustomMarkers()
    {
        for (std::vector<MyGUI::Widget*>::iterator it = mCustomMarkerWidgets.begin(); it != mCustomMarkerWidgets.end(); ++it)
            MyGUI::Gui::getInstance().destroyWidget(*it);
        mCustomMarkerWidgets.clear();

        for (std::vector<ESM::CustomMarker>::const_iterator it = mCustomMarkers.begin(); it != mCustomMarkers.end(); ++it)
        {
            const ESM::CustomMarker& marker = *it;

            if (marker.mCell.mPaged != !mInterior)
                continue;
            if (mInterior)
            {
                if (marker.mCell.mWorldspace != mPrefix)
                    continue;
            }
            else
            {
                if (std::abs(marker.mCell.mIndex.mX - mCurX) > 1)
                    continue;
                if (std::abs(marker.mCell.mIndex.mY - mCurY) > 1)
                    continue;
            }

            MarkerUserData markerPos;
            MyGUI::IntPoint widgetPos = getMarkerPosition(marker.mWorldX, marker.mWorldY, markerPos);

            MyGUI::IntCoord widgetCoord(widgetPos.left - 8,
                                        widgetPos.top - 8,
                                        16, 16);
            MarkerWidget* markerWidget = mLocalMap->createWidget<MarkerWidget>("CustomMarkerButton",
                widgetCoord, MyGUI::Align::Default);
            markerWidget->setDepth(Local_MarkerAboveFogLayer);
            markerWidget->setUserString("ToolTipType", "Layout");
            markerWidget->setUserString("ToolTipLayout", "TextToolTipOneLine");
            markerWidget->setUserString("Caption_TextOneLine", MyGUI::TextIterator::toTagsString(marker.mNote));
            markerWidget->setNormalColour(MyGUI::Colour(0.6f, 0.6f, 0.6f));
            markerWidget->setHoverColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
            markerWidget->setUserData(marker);
            markerWidget->setNeedMouseFocus(true);
            customMarkerCreated(markerWidget);
            mCustomMarkerWidgets.push_back(markerWidget);
        }
        redraw();
    }

    void LocalMapBase::setActiveCell(const int x, const int y, bool interior)
    {
        if (x==mCurX && y==mCurY && mInterior==interior && !mChanged)
            return; // don't do anything if we're still in the same cell

        mCurX = x;
        mCurY = y;
        mInterior = interior;
        mChanged = false;

        applyFogOfWar();


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
                std::string image = mPrefix+"_"+ MyGUI::utility::toString(x + (mx-1)) + "_"
                        + MyGUI::utility::toString(y + (-1*(my-1)));

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

            std::vector<std::string> destNotes;
            for (std::vector<ESM::CustomMarker>::const_iterator it = mCustomMarkers.begin(); it != mCustomMarkers.end(); ++it)
            {
                if (it->mCell == marker.dest)
                    destNotes.push_back(it->mNote);
            }

            MarkerUserData data;
            data.notes = destNotes;
            data.caption = marker.name;
            MyGUI::IntPoint widgetPos = getMarkerPosition(marker.x, marker.y, data);
            MyGUI::IntCoord widgetCoord(widgetPos.left - 4,
                                        widgetPos.top - 4,
                                        8, 8);
            ++counter;
            MarkerWidget* markerWidget = mLocalMap->createWidget<MarkerWidget>("MarkerButton",
                widgetCoord, MyGUI::Align::Default);
            markerWidget->setNormalColour(MyGUI::Colour::parse(MyGUI::LanguageManager::getInstance().replaceTags("#{fontcolour=normal}")));
            markerWidget->setHoverColour(MyGUI::Colour::parse(MyGUI::LanguageManager::getInstance().replaceTags("#{fontcolour=normal_over}")));
            markerWidget->setDepth(Local_MarkerLayer);
            markerWidget->setNeedMouseFocus(true);
            // Used by tooltips to not show the tooltip if marker is hidden by fog of war
            markerWidget->setUserString("ToolTipType", "MapMarker");

            markerWidget->setUserData(data);
            doorMarkerCreated(markerWidget);

            mDoorMarkerWidgets.push_back(markerWidget);
        }

        updateMagicMarkers();
        updateCustomMarkers();
    }

    void LocalMapBase::redraw()
    {
        // Redraw children in proper order
        mLocalMap->getParent()->_updateChilds();
    }

    void LocalMapBase::setPlayerPos(int cellX, int cellY, const float nx, const float ny)
    {
        MyGUI::IntPoint pos(static_cast<int>(mMapWidgetSize + nx*mMapWidgetSize - 16), static_cast<int>(mMapWidgetSize + ny*mMapWidgetSize - 16));
        pos.left += (cellX - mCurX) * mMapWidgetSize;
        pos.top -= (cellY - mCurY) * mMapWidgetSize;

        if (pos != mCompass->getPosition())
        {
            notifyPlayerUpdate ();

            mCompass->setPosition(pos);
            MyGUI::IntPoint middle (pos.left+16, pos.top+16);
                    MyGUI::IntCoord viewsize = mLocalMap->getCoord();
            MyGUI::IntPoint viewOffset((viewsize.width / 2) - middle.left, (viewsize.height / 2) - middle.top);
            mLocalMap->setViewOffset(viewOffset);
        }
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
        if (type == MWBase::World::Detect_Creature)
        {
            markerTexture = "textures\\detect_animal_icon.dds";
        }
        if (type == MWBase::World::Detect_Key)
        {
            markerTexture = "textures\\detect_key_icon.dds";
        }
        if (type == MWBase::World::Detect_Enchantment)
        {
            markerTexture = "textures\\detect_enchantment_icon.dds";
        }

        int counter = 0;
        for (std::vector<MWWorld::Ptr>::iterator it = markers.begin(); it != markers.end(); ++it)
        {
            const ESM::Position& worldPos = it->getRefData().getPosition();
            MarkerUserData markerPos;
            MyGUI::IntPoint widgetPos = getMarkerPosition(worldPos.pos[0], worldPos.pos[1], markerPos);
            MyGUI::IntCoord widgetCoord(widgetPos.left - 4,
                                        widgetPos.top - 4,
                                        8, 8);
            ++counter;
            MyGUI::ImageBox* markerWidget = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                widgetCoord, MyGUI::Align::Default);
            markerWidget->setDepth(Local_MarkerAboveFogLayer);
            markerWidget->setImageTexture(markerTexture);
            markerWidget->setImageCoord(MyGUI::IntCoord(0,0,8,8));
            markerWidget->setNeedMouseFocus(false);
            mMagicMarkerWidgets.push_back(markerWidget);
        }
    }

    void LocalMapBase::onFrame(float dt)
    {
        mMarkerUpdateTimer += dt;

        if (mMarkerUpdateTimer >= 0.25)
        {
            mMarkerUpdateTimer = 0;
            updateMagicMarkers();
        }
    }

    void LocalMapBase::updateMagicMarkers()
    {
        // clear all previous markers
        for (std::vector<MyGUI::Widget*>::iterator it = mMagicMarkerWidgets.begin(); it != mMagicMarkerWidgets.end(); ++it)
            MyGUI::Gui::getInstance().destroyWidget(*it);
        mMagicMarkerWidgets.clear();

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
            MarkerUserData markerPos;
            MyGUI::IntPoint widgetPos = getMarkerPosition(markedPosition.pos[0], markedPosition.pos[1], markerPos);
            MyGUI::IntCoord widgetCoord(widgetPos.left - 4,
                                        widgetPos.top - 4,
                                        8, 8);
            MyGUI::ImageBox* markerWidget = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                widgetCoord, MyGUI::Align::Default);
            markerWidget->setDepth(Local_MarkerAboveFogLayer);
            markerWidget->setImageTexture("textures\\menu_map_smark.dds");
            markerWidget->setNeedMouseFocus(false);
            mMagicMarkerWidgets.push_back(markerWidget);
        }

        redraw();
    }

    // ------------------------------------------------------------------------------------------

    MapWindow::MapWindow(CustomMarkerCollection &customMarkers, DragAndDrop* drag, const std::string& cacheDir)
        : WindowPinnableBase("openmw_map_window.layout")
        , NoDrop(drag, mMainWidget)
        , LocalMapBase(customMarkers)
        , mGlobal(false)
        , mGlobalMap(0)
        , mGlobalMapRender(0)
        , mEditNoteDialog()
        , mEventBoxGlobal(NULL)
        , mEventBoxLocal(NULL)
        , mGlobalMapImage(NULL)
        , mGlobalMapOverlay(NULL)
    {
        static bool registered = false;
        if (!registered)
        {
            MyGUI::FactoryManager::getInstance().registerFactory<MarkerWidget>("Widget");
            registered = true;
        }

        mEditNoteDialog.setVisible(false);
        mEditNoteDialog.eventOkClicked += MyGUI::newDelegate(this, &MapWindow::onNoteEditOk);
        mEditNoteDialog.eventDeleteClicked += MyGUI::newDelegate(this, &MapWindow::onNoteEditDelete);

        setCoord(500,0,320,300);

        getWidget(mLocalMap, "LocalMap");
        getWidget(mGlobalMap, "GlobalMap");
        getWidget(mGlobalMapImage, "GlobalMapImage");
        getWidget(mGlobalMapOverlay, "GlobalMapOverlay");
        getWidget(mPlayerArrowLocal, "CompassLocal");
        getWidget(mPlayerArrowGlobal, "CompassGlobal");

        mPlayerArrowGlobal->setDepth(Global_CompassLayer);
        mPlayerArrowGlobal->setNeedMouseFocus(false);
        mGlobalMapImage->setDepth(Global_MapLayer);
        mGlobalMapOverlay->setDepth(Global_ExploreOverlayLayer);

        mLastScrollWindowCoordinates = mLocalMap->getCoord();
        mLocalMap->eventChangeCoord += MyGUI::newDelegate(this, &MapWindow::onChangeScrollWindowCoord);

        mGlobalMap->setVisible (false);

        getWidget(mButton, "WorldButton");
        mButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MapWindow::onWorldButtonClicked);
        mButton->setCaptionWithReplacing("#{sWorld}");

        getWidget(mEventBoxGlobal, "EventBoxGlobal");
        mEventBoxGlobal->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        mEventBoxGlobal->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);
        mEventBoxGlobal->setDepth(Global_ExploreOverlayLayer);

        getWidget(mEventBoxLocal, "EventBoxLocal");
        mEventBoxLocal->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        mEventBoxLocal->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);
        mEventBoxLocal->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &MapWindow::onMapDoubleClicked);

        LocalMapBase::init(mLocalMap, mPlayerArrowLocal, Settings::Manager::getInt("local map widget size", "Map"));    }

    void MapWindow::onNoteEditOk()
    {
        if (mEditNoteDialog.getDeleteButtonShown())
            mCustomMarkers.updateMarker(mEditingMarker, mEditNoteDialog.getText());
        else
        {
            mEditingMarker.mNote = mEditNoteDialog.getText();
            mCustomMarkers.addMarker(mEditingMarker);
        }

        mEditNoteDialog.setVisible(false);
    }

    void MapWindow::onNoteEditDelete()
    {
        ConfirmationDialog* confirmation = MWBase::Environment::get().getWindowManager()->getConfirmationDialog();
        confirmation->open("#{sDeleteNote}", "#{sYes}", "#{sNo}");
        confirmation->eventCancelClicked.clear();
        confirmation->eventOkClicked.clear();
        confirmation->eventOkClicked += MyGUI::newDelegate(this, &MapWindow::onNoteEditDeleteConfirm);
    }

    void MapWindow::onNoteEditDeleteConfirm()
    {
        mCustomMarkers.deleteMarker(mEditingMarker);

        mEditNoteDialog.setVisible(false);
    }

    void MapWindow::onCustomMarkerDoubleClicked(MyGUI::Widget *sender)
    {
        mEditingMarker = *sender->getUserData<ESM::CustomMarker>();
        mEditNoteDialog.setText(mEditingMarker.mNote);
        mEditNoteDialog.showDeleteButton(true);
        mEditNoteDialog.setVisible(true);
    }

    void MapWindow::onMapDoubleClicked(MyGUI::Widget *sender)
    {
        MyGUI::IntPoint clickedPos = MyGUI::InputManager::getInstance().getMousePosition();

        MyGUI::IntPoint widgetPos = clickedPos - mEventBoxLocal->getAbsolutePosition();
        int x = int(widgetPos.left/float(mMapWidgetSize))-1;
        int y = (int(widgetPos.top/float(mMapWidgetSize))-1)*-1;
        float nX = widgetPos.left/float(mMapWidgetSize) - int(widgetPos.left/float(mMapWidgetSize));
        float nY = widgetPos.top/float(mMapWidgetSize) - int(widgetPos.top/float(mMapWidgetSize));
        x += mCurX;
        y += mCurY;

        Ogre::Vector2 worldPos;
        if (mInterior)
        {
            worldPos = MWBase::Environment::get().getWorld()->interiorMapToWorldPosition(nX, nY, x, y);
        }
        else
        {
            worldPos.x = (x + nX) * cellSize;
            worldPos.y = (y + (1.0f-nY)) * cellSize;
        }

        mEditingMarker.mWorldX = worldPos.x;
        mEditingMarker.mWorldY = worldPos.y;

        mEditingMarker.mCell.mPaged = !mInterior;
        if (mInterior)
            mEditingMarker.mCell.mWorldspace = LocalMapBase::mPrefix;
        else
        {
            mEditingMarker.mCell.mWorldspace = "sys::default";
            mEditingMarker.mCell.mIndex.mX = x;
            mEditingMarker.mCell.mIndex.mY = y;
        }

        mEditNoteDialog.setVisible(true);
        mEditNoteDialog.showDeleteButton(false);
        mEditNoteDialog.setText("");
    }

    void MapWindow::onChangeScrollWindowCoord(MyGUI::Widget* sender)
    {
        MyGUI::IntCoord currentCoordinates = sender->getCoord();

        MyGUI::IntPoint currentViewPortCenter = MyGUI::IntPoint(currentCoordinates.width / 2, currentCoordinates.height / 2);
        MyGUI::IntPoint lastViewPortCenter = MyGUI::IntPoint(mLastScrollWindowCoordinates.width / 2, mLastScrollWindowCoordinates.height / 2);
        MyGUI::IntPoint viewPortCenterDiff = currentViewPortCenter - lastViewPortCenter;

        mLocalMap->setViewOffset(mLocalMap->getViewOffset() + viewPortCenterDiff);
        mGlobalMap->setViewOffset(mGlobalMap->getViewOffset() + viewPortCenterDiff);

        mLastScrollWindowCoordinates = currentCoordinates;
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
        CellId cell;
        cell.first = x;
        cell.second = y;
        if (mMarkers.insert(cell).second)
        {
            float worldX, worldY;
            mGlobalMapRender->cellTopLeftCornerToImageSpace (x, y, worldX, worldY);

            int markerSize = 12;
            int offset = mGlobalMapRender->getCellSize()/2 - markerSize/2;
            MyGUI::IntCoord widgetCoord(
                        static_cast<int>(worldX * mGlobalMapRender->getWidth()+offset),
                        static_cast<int>(worldY * mGlobalMapRender->getHeight() + offset),
                        markerSize, markerSize);

            MyGUI::Widget* markerWidget = mGlobalMap->createWidget<MyGUI::Widget>("MarkerButton",
                widgetCoord, MyGUI::Align::Default);
            markerWidget->setNeedMouseFocus(true);
            markerWidget->setColour(MyGUI::Colour::parse(MyGUI::LanguageManager::getInstance().replaceTags("#{fontcolour=normal}")));
            markerWidget->setUserString("ToolTipType", "Layout");
            markerWidget->setUserString("ToolTipLayout", "TextToolTipOneLine");
            markerWidget->setUserString("Caption_TextOneLine", name);
            markerWidget->setDepth(Global_MarkerLayer);
            markerWidget->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
            markerWidget->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);
        }
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

    void MapWindow::onTitleDoubleClicked()
    {
        if (!mPinned)
            MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Map);
    }

    void MapWindow::open()
    {
        globalMapUpdatePlayer();
    }

    void MapWindow::globalMapUpdatePlayer ()
    {
        // For interiors, position is set by WindowManager via setGlobalMapPlayerPosition
        if (MWBase::Environment::get().getWorld ()->isCellExterior ())
        {
            Ogre::Vector3 pos = MWBase::Environment::get().getWorld ()->getPlayerPtr().getRefData ().getBaseNode ()->_getDerivedPosition ();
            setGlobalMapPlayerPosition(pos.x, pos.y);
        }
    }

    void MapWindow::notifyPlayerUpdate ()
    {
        globalMapUpdatePlayer ();
    }

    void MapWindow::setGlobalMapPlayerPosition(float worldX, float worldY)
    {
        float x, y;
        mGlobalMapRender->worldPosToImageSpace (worldX, worldY, x, y);
        x *= mGlobalMapRender->getWidth();
        y *= mGlobalMapRender->getHeight();

        mPlayerArrowGlobal->setPosition(MyGUI::IntPoint(static_cast<int>(x - 16), static_cast<int>(y - 16)));

        // set the view offset so that player is in the center
        MyGUI::IntSize viewsize = mGlobalMap->getSize();
        MyGUI::IntPoint viewoffs(static_cast<int>(viewsize.width * 0.5f - x), static_cast<int>(viewsize.height *0.5 - y));
        mGlobalMap->setViewOffset(viewoffs);
    }

    void MapWindow::setGlobalMapPlayerDir(const float x, const float y)
    {
        MyGUI::ISubWidget* main = mPlayerArrowGlobal->getSubWidgetMain();
        MyGUI::RotatingSkin* rotatingSubskin = main->castType<MyGUI::RotatingSkin>();
        rotatingSubskin->setCenter(MyGUI::IntPoint(16,16));
        float angle = std::atan2(x,y);
        rotatingSubskin->setAngle(angle);
    }

    void MapWindow::clear()
    {
        mMarkers.clear();
        mGlobalMapRender->clear();
        mChanged = true;

        while (mEventBoxGlobal->getChildCount())
            MyGUI::Gui::getInstance().destroyWidget(mEventBoxGlobal->getChildAt(0));
    }

    void MapWindow::write(ESM::ESMWriter &writer, Loading::Listener& progress)
    {
        ESM::GlobalMap map;
        mGlobalMapRender->write(map);

        map.mMarkers = mMarkers;

        writer.startRecord(ESM::REC_GMAP);
        map.save(writer);
        writer.endRecord(ESM::REC_GMAP);
    }

    void MapWindow::readRecord(ESM::ESMReader &reader, uint32_t type)
    {
        if (type == ESM::REC_GMAP)
        {
            ESM::GlobalMap map;
            map.load(reader);

            mGlobalMapRender->read(map);

            for (std::set<ESM::GlobalMap::CellId>::iterator it = map.mMarkers.begin(); it != map.mMarkers.end(); ++it)
            {
                const ESM::Cell* cell = MWBase::Environment::get().getWorld()->getStore().get<ESM::Cell>().search(it->first, it->second);
                if (cell && !cell->mName.empty())
                    addVisitedLocation(cell->mName, it->first, it->second);
            }
        }
    }

    void MapWindow::setAlpha(float alpha)
    {
        NoDrop::setAlpha(alpha);
        // can't allow showing map with partial transparency, as the fog of war will also go transparent
        // and reveal parts of the map you shouldn't be able to see
        for (std::vector<MyGUI::ImageBox*>::iterator it = mMapWidgets.begin(); it != mMapWidgets.end(); ++it)
            (*it)->setVisible(alpha == 1);
    }

    void MapWindow::customMarkerCreated(MyGUI::Widget *marker)
    {
        marker->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        marker->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);
        marker->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &MapWindow::onCustomMarkerDoubleClicked);
    }

    void MapWindow::doorMarkerCreated(MyGUI::Widget *marker)
    {
        marker->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        marker->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);
    }

    // -------------------------------------------------------------------

    EditNoteDialog::EditNoteDialog()
        : WindowModal("openmw_edit_note.layout")
    {
        getWidget(mOkButton, "OkButton");
        getWidget(mCancelButton, "CancelButton");
        getWidget(mDeleteButton, "DeleteButton");
        getWidget(mTextEdit, "TextEdit");

        mCancelButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditNoteDialog::onCancelButtonClicked);
        mOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditNoteDialog::onOkButtonClicked);
        mDeleteButton->eventMouseButtonClick += MyGUI::newDelegate(this, &EditNoteDialog::onDeleteButtonClicked);
    }

    void EditNoteDialog::showDeleteButton(bool show)
    {
        mDeleteButton->setVisible(show);
    }

    bool EditNoteDialog::getDeleteButtonShown()
    {
        return mDeleteButton->getVisible();
    }

    void EditNoteDialog::setText(const std::string &text)
    {
        mTextEdit->setCaption(MyGUI::TextIterator::toTagsString(text));
    }

    std::string EditNoteDialog::getText()
    {
        return MyGUI::TextIterator::getOnlyText(mTextEdit->getCaption());
    }

    void EditNoteDialog::open()
    {
        WindowModal::open();
        center();
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTextEdit);
    }

    void EditNoteDialog::exit()
    {
        setVisible(false);
    }

    void EditNoteDialog::onCancelButtonClicked(MyGUI::Widget *sender)
    {
        setVisible(false);
    }

    void EditNoteDialog::onOkButtonClicked(MyGUI::Widget *sender)
    {
        eventOkClicked();
    }

    void EditNoteDialog::onDeleteButtonClicked(MyGUI::Widget *sender)
    {
        eventDeleteClicked();
    }

}
