#include "mapwindow.hpp"

#include <osg/Texture2D>

#include <MyGUI_Button.h>
#include <MyGUI_FactoryManager.h>
#include <MyGUI_Gui.h>
#include <MyGUI_ImageBox.h>
#include <MyGUI_InputManager.h>
#include <MyGUI_LanguageManager.h>
#include <MyGUI_RenderManager.h>
#include <MyGUI_RotatingSkin.h>
#include <MyGUI_ScrollView.h>
#include <MyGUI_TextIterator.h>

#include <components/esm3/esmwriter.hpp>
#include <components/esm3/globalmap.hpp>
#include <components/myguiplatform/myguitexture.hpp>
#include <components/settings/values.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/cellstore.hpp"
#include "../mwworld/esmstore.hpp"
#include "../mwworld/player.hpp"
#include "../mwworld/worldmodel.hpp"

#include "../mwrender/globalmap.hpp"
#include "../mwrender/localmap.hpp"

#include "confirmationdialog.hpp"

#include <numeric>

namespace
{

    const int cellSize = Constants::CellSizeInUnits;
    constexpr float speed = 1.08f; // the zoom speed, it should be greater than 1

    enum LocalMapWidgetDepth
    {
        Local_MarkerAboveFogLayer = 0,
        Local_CompassLayer = 1,
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
    class MarkerWidget final : public MyGUI::Widget
    {
        MYGUI_RTTI_DERIVED(MarkerWidget)

    public:
        void setNormalColour(const MyGUI::Colour& colour)
        {
            mNormalColour = colour;
            setColour(colour);
        }

        void setHoverColour(const MyGUI::Colour& colour) { mHoverColour = colour; }

    private:
        MyGUI::Colour mNormalColour;
        MyGUI::Colour mHoverColour;

        void onMouseLostFocus(MyGUI::Widget* _new) override { setColour(mNormalColour); }

        void onMouseSetFocus(MyGUI::Widget* _old) override { setColour(mHoverColour); }
    };

    MyGUI::IntRect createRect(const MyGUI::IntPoint& center, int radius)
    {
        return { center.left - radius, center.top + radius, center.left + radius, center.top - radius };
    }

    int getLocalViewingDistance()
    {
        if (!Settings::map().mAllowZooming)
            return Constants::CellGridRadius;
        if (!Settings::terrain().mDistantTerrain)
            return Constants::CellGridRadius;
        const int viewingDistanceInCells = Settings::camera().mViewingDistance / Constants::CellSizeInUnits;
        return std::clamp(
            viewingDistanceInCells, Constants::CellGridRadius, Settings::map().mMaxLocalViewingDistance.get());
    }

    ESM::RefId getCellIdInWorldSpace(const MWWorld::Cell& cell, int x, int y)
    {
        if (cell.isExterior())
            return ESM::Cell::generateIdForCell(true, {}, x, y);
        return cell.getId();
    }
}

namespace MWGui
{

    void CustomMarkerCollection::addMarker(const ESM::CustomMarker& marker, bool triggerEvent)
    {
        mMarkers.insert(std::make_pair(marker.mCell, marker));
        if (triggerEvent)
            eventMarkersChanged();
    }

    void CustomMarkerCollection::deleteMarker(const ESM::CustomMarker& marker)
    {
        std::pair<ContainerType::iterator, ContainerType::iterator> range = mMarkers.equal_range(marker.mCell);

        for (ContainerType::iterator it = range.first; it != range.second; ++it)
        {
            if (it->second == marker)
            {
                mMarkers.erase(it);
                eventMarkersChanged();
                return;
            }
        }
        throw std::runtime_error("can't find marker to delete");
    }

    void CustomMarkerCollection::updateMarker(const ESM::CustomMarker& marker, const std::string& newNote)
    {
        std::pair<ContainerType::iterator, ContainerType::iterator> range = mMarkers.equal_range(marker.mCell);

        for (ContainerType::iterator it = range.first; it != range.second; ++it)
        {
            if (it->second == marker)
            {
                it->second.mNote = newNote;
                eventMarkersChanged();
                return;
            }
        }
        throw std::runtime_error("can't find marker to update");
    }

    void CustomMarkerCollection::clear()
    {
        mMarkers.clear();
        eventMarkersChanged();
    }

    CustomMarkerCollection::ContainerType::const_iterator CustomMarkerCollection::begin() const
    {
        return mMarkers.begin();
    }

    CustomMarkerCollection::ContainerType::const_iterator CustomMarkerCollection::end() const
    {
        return mMarkers.end();
    }

    CustomMarkerCollection::RangeType CustomMarkerCollection::getMarkers(const ESM::RefId& cellId) const
    {
        return mMarkers.equal_range(cellId);
    }

    size_t CustomMarkerCollection::size() const
    {
        return mMarkers.size();
    }

    // ------------------------------------------------------

    LocalMapBase::LocalMapBase(
        CustomMarkerCollection& markers, MWRender::LocalMap* localMapRender, bool fogOfWarEnabled)
        : mLocalMapRender(localMapRender)
        , mActiveCell(nullptr)
        , mLocalMap(nullptr)
        , mCompass(nullptr)
        , mFogOfWarToggled(true)
        , mFogOfWarEnabled(fogOfWarEnabled)
        , mNumCells(1)
        , mCellDistance(0)
        , mCustomMarkers(markers)
        , mMarkerUpdateTimer(0.0f)
        , mLastDirectionX(0.0f)
        , mLastDirectionY(0.0f)
        , mNeedDoorMarkersUpdate(false)
    {
        mCustomMarkers.eventMarkersChanged += MyGUI::newDelegate(this, &LocalMapBase::updateCustomMarkers);
    }

    LocalMapBase::~LocalMapBase()
    {
        mCustomMarkers.eventMarkersChanged -= MyGUI::newDelegate(this, &LocalMapBase::updateCustomMarkers);
    }

    void LocalMapBase::init(MyGUI::ScrollView* widget, MyGUI::ImageBox* compass, int cellDistance)
    {
        mLocalMap = widget;
        mCompass = compass;
        mCellDistance = cellDistance;
        mNumCells = mCellDistance * 2 + 1;

        const int mapWidgetSize = Settings::map().mLocalMapWidgetSize;

        mLocalMap->setCanvasSize(mapWidgetSize * mNumCells, mapWidgetSize * mNumCells);

        mCompass->setDepth(Local_CompassLayer);
        mCompass->setNeedMouseFocus(false);

        for (int mx = 0; mx < mNumCells; ++mx)
        {
            for (int my = 0; my < mNumCells; ++my)
            {
                MyGUI::ImageBox* map = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                    MyGUI::IntCoord(mx * mapWidgetSize, my * mapWidgetSize, mapWidgetSize, mapWidgetSize),
                    MyGUI::Align::Top | MyGUI::Align::Left);
                map->setDepth(Local_MapLayer);

                MyGUI::ImageBox* fog = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                    MyGUI::IntCoord(mx * mapWidgetSize, my * mapWidgetSize, mapWidgetSize, mapWidgetSize),
                    MyGUI::Align::Top | MyGUI::Align::Left);
                fog->setDepth(Local_FogLayer);
                fog->setColour(MyGUI::Colour(0, 0, 0));

                map->setNeedMouseFocus(false);
                fog->setNeedMouseFocus(false);

                mMaps.emplace_back(map, fog);
            }
        }
    }

    bool LocalMapBase::toggleFogOfWar()
    {
        mFogOfWarToggled = !mFogOfWarToggled;
        applyFogOfWar();
        return mFogOfWarToggled;
    }

    void LocalMapBase::applyFogOfWar()
    {
        if (!mFogOfWarToggled || !mFogOfWarEnabled)
        {
            for (auto& entry : mMaps)
            {
                entry.mFogWidget->setImageTexture({});
                entry.mFogTexture.reset();
            }
        }

        redraw();
    }

    MyGUI::IntPoint LocalMapBase::getPosition(int cellX, int cellY, float nX, float nY) const
    {
        // normalized cell coordinates
        auto mapWidgetSize = getWidgetSize();
        return MyGUI::IntPoint(std::round((nX + mCellDistance + cellX - mActiveCell->getGridX()) * mapWidgetSize),
            std::round((nY + mCellDistance - cellY + mActiveCell->getGridY()) * mapWidgetSize));
    }

    MyGUI::IntPoint LocalMapBase::getMarkerPosition(float worldX, float worldY, MarkerUserData& markerPos) const
    {
        osg::Vec2i cellIndex;
        // normalized cell coordinates
        float nX, nY;

        if (mActiveCell->isExterior())
        {
            ESM::ExteriorCellLocation cellPos = ESM::positionToExteriorCellLocation(worldX, worldY);
            cellIndex.x() = cellPos.mX;
            cellIndex.y() = cellPos.mY;

            nX = (worldX - cellSize * cellIndex.x()) / cellSize;
            // Image space is -Y up, cells are Y up
            nY = 1 - (worldY - cellSize * cellIndex.y()) / cellSize;
        }
        else
            mLocalMapRender->worldToInteriorMapPosition({ worldX, worldY }, nX, nY, cellIndex.x(), cellIndex.y());

        markerPos.cellX = cellIndex.x();
        markerPos.cellY = cellIndex.y();
        markerPos.nX = nX;
        markerPos.nY = nY;
        return getPosition(markerPos.cellX, markerPos.cellY, markerPos.nX, markerPos.nY);
    }

    MyGUI::IntCoord LocalMapBase::getMarkerCoordinates(
        float worldX, float worldY, MarkerUserData& markerPos, size_t markerSize) const
    {
        int halfMarkerSize = markerSize / 2;
        auto position = getMarkerPosition(worldX, worldY, markerPos);
        return MyGUI::IntCoord(position.left - halfMarkerSize, position.top - halfMarkerSize, markerSize, markerSize);
    }

    MyGUI::Widget* LocalMapBase::createDoorMarker(const std::string& name, float x, float y) const
    {
        MarkerUserData data(mLocalMapRender);
        data.caption = name;
        MarkerWidget* markerWidget = mLocalMap->createWidget<MarkerWidget>(
            "MarkerButton", getMarkerCoordinates(x, y, data, 8), MyGUI::Align::Default);
        markerWidget->setNormalColour(
            MyGUI::Colour::parse(MyGUI::LanguageManager::getInstance().replaceTags("#{fontcolour=normal}")));
        markerWidget->setHoverColour(
            MyGUI::Colour::parse(MyGUI::LanguageManager::getInstance().replaceTags("#{fontcolour=normal_over}")));
        markerWidget->setDepth(Local_MarkerLayer);
        markerWidget->setNeedMouseFocus(true);
        // Used by tooltips to not show the tooltip if marker is hidden by fog of war
        markerWidget->setUserString("ToolTipType", "MapMarker");

        markerWidget->setUserData(data);
        return markerWidget;
    }

    void LocalMapBase::centerView()
    {
        MyGUI::IntPoint pos = mCompass->getPosition() + MyGUI::IntPoint{ 16, 16 };
        MyGUI::IntSize viewsize = mLocalMap->getSize();
        MyGUI::IntPoint viewOffset((viewsize.width / 2) - pos.left, (viewsize.height / 2) - pos.top);
        mLocalMap->setViewOffset(viewOffset);
    }

    MyGUI::IntCoord LocalMapBase::getMarkerCoordinates(MyGUI::Widget* widget, size_t markerSize) const
    {
        MarkerUserData& markerPos(*widget->getUserData<MarkerUserData>());
        auto position = getPosition(markerPos.cellX, markerPos.cellY, markerPos.nX, markerPos.nY);
        return MyGUI::IntCoord(position.left - markerSize / 2, position.top - markerSize / 2, markerSize, markerSize);
    }

    std::vector<MyGUI::Widget*>& LocalMapBase::currentDoorMarkersWidgets()
    {
        return mActiveCell->isExterior() ? mExteriorDoorMarkerWidgets : mInteriorDoorMarkerWidgets;
    }

    void LocalMapBase::updateCustomMarkers()
    {
        for (MyGUI::Widget* widget : mCustomMarkerWidgets)
            MyGUI::Gui::getInstance().destroyWidget(widget);
        mCustomMarkerWidgets.clear();
        if (!mActiveCell)
            return;
        for (int dX = -mCellDistance; dX <= mCellDistance; ++dX)
        {
            for (int dY = -mCellDistance; dY <= mCellDistance; ++dY)
            {
                ESM::RefId cellRefId
                    = getCellIdInWorldSpace(*mActiveCell, mActiveCell->getGridX() + dX, mActiveCell->getGridY() + dY);

                CustomMarkerCollection::RangeType markers = mCustomMarkers.getMarkers(cellRefId);
                for (CustomMarkerCollection::ContainerType::const_iterator it = markers.first; it != markers.second;
                     ++it)
                {
                    const ESM::CustomMarker& marker = it->second;

                    MarkerUserData markerPos(mLocalMapRender);
                    MarkerWidget* markerWidget = mLocalMap->createWidget<MarkerWidget>("CustomMarkerButton",
                        getMarkerCoordinates(marker.mWorldX, marker.mWorldY, markerPos, 16), MyGUI::Align::Default);
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
            }
        }

        redraw();
    }

    void LocalMapBase::setActiveCell(const MWWorld::Cell& cell)
    {
        if (&cell == mActiveCell)
            return; // don't do anything if we're still in the same cell

        const int x = cell.getGridX();
        const int y = cell.getGridY();

        if (cell.isExterior())
        {
            int curX = 0;
            int curY = 0;
            if (mActiveCell)
            {
                curX = mActiveCell->getGridX();
                curY = mActiveCell->getGridY();
            }
            const MyGUI::IntRect intersection = { std::max(x, curX) - mCellDistance, std::max(y, curY) - mCellDistance,
                std::min(x, curX) + mCellDistance, std::min(y, curY) + mCellDistance };

            const MyGUI::IntRect activeGrid = createRect({ x, y }, Constants::CellGridRadius);
            const MyGUI::IntRect currentView = createRect({ x, y }, mCellDistance);

            mExteriorDoorMarkerWidgets.clear();
            for (auto& [coord, doors] : mExteriorDoorsByCell)
            {
                if (!mHasALastActiveCell || !currentView.inside({ coord.first, coord.second })
                    || activeGrid.inside({ coord.first, coord.second }))
                {
                    mDoorMarkersToRecycle.insert(mDoorMarkersToRecycle.end(), doors.begin(), doors.end());
                    doors.clear();
                }
                else
                    mExteriorDoorMarkerWidgets.insert(mExteriorDoorMarkerWidgets.end(), doors.begin(), doors.end());
            }

            for (auto& widget : mDoorMarkersToRecycle)
                widget->setVisible(false);

            for (auto const& entry : mMaps)
            {
                if (mHasALastActiveCell && !intersection.inside({ entry.mCellX, entry.mCellY }))
                    mLocalMapRender->removeExteriorCell(entry.mCellX, entry.mCellY);
            }
        }

        mActiveCell = &cell;

        for (int mx = 0; mx < mNumCells; ++mx)
        {
            for (int my = 0; my < mNumCells; ++my)
            {
                MapEntry& entry = mMaps[my + mNumCells * mx];
                entry.mMapWidget->setRenderItemTexture(nullptr);
                entry.mFogWidget->setRenderItemTexture(nullptr);
                entry.mMapTexture.reset();
                entry.mFogTexture.reset();

                entry.mCellX = x + (mx - mCellDistance);
                entry.mCellY = y - (my - mCellDistance);
            }
        }

        // Delay the door markers update until scripts have been given a chance to run.
        // If we don't do this, door markers that should be disabled will still appear on the map.
        mNeedDoorMarkersUpdate = true;

        for (MyGUI::Widget* widget : currentDoorMarkersWidgets())
            widget->setCoord(getMarkerCoordinates(widget, 8));

        if (mActiveCell->isExterior())
            mHasALastActiveCell = true;

        updateMagicMarkers();
        updateCustomMarkers();
    }

    void LocalMapBase::requestMapRender(const MWWorld::CellStore* cell)
    {
        mLocalMapRender->requestMap(cell);
    }

    void LocalMapBase::redraw()
    {
        // Redraw children in proper order
        mLocalMap->getParent()->_updateChilds();
    }

    float LocalMapBase::getWidgetSize() const
    {
        return mLocalMapZoom * Settings::map().mLocalMapWidgetSize;
    }

    void LocalMapBase::setPlayerPos(int cellX, int cellY, const float nx, const float ny)
    {
        MyGUI::IntPoint pos = getPosition(cellX, cellY, nx, ny) - MyGUI::IntPoint{ 16, 16 };

        if (pos != mCompass->getPosition())
        {
            notifyPlayerUpdate();

            mCompass->setPosition(pos);
        }
        osg::Vec2f curPos((cellX + nx) * cellSize, (cellY + 1 - ny) * cellSize);
        if ((curPos - mCurPos).length2() > 0.001)
        {
            mCurPos = curPos;
            centerView();
        }
    }

    void LocalMapBase::setPlayerDir(const float x, const float y)
    {
        if (x == mLastDirectionX && y == mLastDirectionY)
            return;

        notifyPlayerUpdate();

        MyGUI::ISubWidget* main = mCompass->getSubWidgetMain();
        MyGUI::RotatingSkin* rotatingSubskin = main->castType<MyGUI::RotatingSkin>();
        rotatingSubskin->setCenter(MyGUI::IntPoint(16, 16));
        float angle = std::atan2(x, y);
        rotatingSubskin->setAngle(angle);

        mLastDirectionX = x;
        mLastDirectionY = y;
    }

    void LocalMapBase::addDetectionMarkers(int type)
    {
        std::vector<MWWorld::Ptr> markers;
        MWBase::World* world = MWBase::Environment::get().getWorld();
        world->listDetectedReferences(world->getPlayerPtr(), markers, MWBase::World::DetectionType(type));
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

        for (const MWWorld::Ptr& ptr : markers)
        {
            const ESM::Position& worldPos = ptr.getRefData().getPosition();
            MarkerUserData markerPos(mLocalMapRender);
            MyGUI::ImageBox* markerWidget = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                getMarkerCoordinates(worldPos.pos[0], worldPos.pos[1], markerPos, 8), MyGUI::Align::Default);
            markerWidget->setDepth(Local_MarkerAboveFogLayer);
            markerWidget->setImageTexture(markerTexture);
            markerWidget->setImageCoord(MyGUI::IntCoord(0, 0, 8, 8));
            markerWidget->setNeedMouseFocus(false);
            markerWidget->setUserData(markerPos);
            mMagicMarkerWidgets.push_back(markerWidget);
        }
    }

    void LocalMapBase::onFrame(float dt)
    {
        if (mNeedDoorMarkersUpdate)
        {
            updateDoorMarkers();
            mNeedDoorMarkersUpdate = false;
        }

        mMarkerUpdateTimer += dt;

        if (mMarkerUpdateTimer >= 0.25)
        {
            mMarkerUpdateTimer = 0;
            updateMagicMarkers();
        }

        updateRequiredMaps();
    }

    bool widgetCropped(MyGUI::Widget* widget, MyGUI::Widget* cropTo)
    {
        MyGUI::IntRect coord = widget->getAbsoluteRect();
        MyGUI::IntRect croppedCoord = cropTo->getAbsoluteRect();
        if (coord.left < croppedCoord.left && coord.right < croppedCoord.left)
            return true;
        if (coord.left > croppedCoord.right && coord.right > croppedCoord.right)
            return true;
        if (coord.top < croppedCoord.top && coord.bottom < croppedCoord.top)
            return true;
        if (coord.top > croppedCoord.bottom && coord.bottom > croppedCoord.bottom)
            return true;
        return false;
    }

    void LocalMapBase::updateRequiredMaps()
    {
        bool needRedraw = false;
        for (MapEntry& entry : mMaps)
        {
            if (widgetCropped(entry.mMapWidget, mLocalMap))
                continue;

            if (!entry.mMapTexture)
            {
                if (mActiveCell->isExterior())
                    requestMapRender(&MWBase::Environment::get().getWorldModel()->getExterior(
                        ESM::ExteriorCellLocation(entry.mCellX, entry.mCellY, ESM::Cell::sDefaultWorldspaceId)));

                osg::ref_ptr<osg::Texture2D> texture = mLocalMapRender->getMapTexture(entry.mCellX, entry.mCellY);
                if (texture)
                {
                    entry.mMapTexture = std::make_unique<osgMyGUI::OSGTexture>(texture);
                    entry.mMapWidget->setRenderItemTexture(entry.mMapTexture.get());
                    entry.mMapWidget->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, 1.f, 1.f));
                    needRedraw = true;
                }
                else
                    entry.mMapTexture = std::make_unique<osgMyGUI::OSGTexture>(std::string(), nullptr);
            }
            if (!entry.mFogTexture && mFogOfWarToggled && mFogOfWarEnabled)
            {
                osg::ref_ptr<osg::Texture2D> tex = mLocalMapRender->getFogOfWarTexture(entry.mCellX, entry.mCellY);
                if (tex)
                {
                    entry.mFogTexture = std::make_unique<osgMyGUI::OSGTexture>(tex);
                    entry.mFogWidget->setRenderItemTexture(entry.mFogTexture.get());
                    entry.mFogWidget->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 1.f, 1.f, 0.f));
                }
                else
                {
                    entry.mFogWidget->setImageTexture("black");
                    entry.mFogTexture = std::make_unique<osgMyGUI::OSGTexture>(std::string(), nullptr);
                }
                needRedraw = true;
            }
        }
        if (needRedraw)
            redraw();
    }

    void LocalMapBase::updateDoorMarkers()
    {
        std::vector<MWBase::World::DoorMarker> doors;
        MWBase::World* world = MWBase::Environment::get().getWorld();
        MWWorld::WorldModel* worldModel = MWBase::Environment::get().getWorldModel();

        mDoorMarkersToRecycle.insert(
            mDoorMarkersToRecycle.end(), mInteriorDoorMarkerWidgets.begin(), mInteriorDoorMarkerWidgets.end());
        mInteriorDoorMarkerWidgets.clear();

        if (!mActiveCell->isExterior())
        {
            for (MyGUI::Widget* widget : mExteriorDoorMarkerWidgets)
                widget->setVisible(false);

            MWWorld::CellStore& cell = worldModel->getInterior(mActiveCell->getNameId());
            world->getDoorMarkers(cell, doors);
        }
        else
        {
            for (MapEntry& entry : mMaps)
            {
                if (!entry.mMapTexture && !widgetCropped(entry.mMapWidget, mLocalMap))
                    world->getDoorMarkers(worldModel->getExterior(ESM::ExteriorCellLocation(
                                              entry.mCellX, entry.mCellY, ESM::Cell::sDefaultWorldspaceId)),
                        doors);
            }
            if (doors.empty())
                return;
        }

        // Create a widget for each marker
        for (MWBase::World::DoorMarker& marker : doors)
        {
            std::vector<std::string> destNotes;
            CustomMarkerCollection::RangeType markers = mCustomMarkers.getMarkers(marker.dest);
            for (CustomMarkerCollection::ContainerType::const_iterator iter = markers.first; iter != markers.second;
                 ++iter)
                destNotes.push_back(iter->second.mNote);

            MyGUI::Widget* markerWidget = nullptr;
            MarkerUserData* data;
            if (mDoorMarkersToRecycle.empty())
            {
                markerWidget = createDoorMarker(marker.name, marker.x, marker.y);
                data = markerWidget->getUserData<MarkerUserData>();
                data->notes = std::move(destNotes);
                doorMarkerCreated(markerWidget);
            }
            else
            {
                markerWidget = (MarkerWidget*)mDoorMarkersToRecycle.back();
                mDoorMarkersToRecycle.pop_back();

                data = markerWidget->getUserData<MarkerUserData>();
                data->notes = std::move(destNotes);
                data->caption = marker.name;
                markerWidget->setCoord(getMarkerCoordinates(marker.x, marker.y, *data, 8));
                markerWidget->setVisible(true);
            }

            currentDoorMarkersWidgets().push_back(markerWidget);
            if (mActiveCell->isExterior())
                mExteriorDoorsByCell[{ data->cellX, data->cellY }].push_back(markerWidget);
        }

        for (auto& widget : mDoorMarkersToRecycle)
            widget->setVisible(false);
    }

    void LocalMapBase::updateMagicMarkers()
    {
        // clear all previous markers
        for (MyGUI::Widget* widget : mMagicMarkerWidgets)
            MyGUI::Gui::getInstance().destroyWidget(widget);
        mMagicMarkerWidgets.clear();

        addDetectionMarkers(MWBase::World::Detect_Creature);
        addDetectionMarkers(MWBase::World::Detect_Key);
        addDetectionMarkers(MWBase::World::Detect_Enchantment);

        // Add marker for the spot marked with Mark magic effect
        MWWorld::CellStore* markedCell = nullptr;
        ESM::Position markedPosition;
        MWBase::Environment::get().getWorld()->getPlayer().getMarkedPosition(markedCell, markedPosition);
        if (markedCell && markedCell->getCell()->getWorldSpace() == mActiveCell->getWorldSpace())
        {
            MarkerUserData markerPos(mLocalMapRender);
            MyGUI::ImageBox* markerWidget = mLocalMap->createWidget<MyGUI::ImageBox>("ImageBox",
                getMarkerCoordinates(markedPosition.pos[0], markedPosition.pos[1], markerPos, 8),
                MyGUI::Align::Default);
            markerWidget->setDepth(Local_MarkerAboveFogLayer);
            markerWidget->setImageTexture("textures\\menu_map_smark.dds");
            markerWidget->setNeedMouseFocus(false);
            markerWidget->setUserData(markerPos);
            mMagicMarkerWidgets.push_back(markerWidget);
        }

        redraw();
    }

    void LocalMapBase::updateLocalMap()
    {
        auto mapWidgetSize = getWidgetSize();
        mLocalMap->setCanvasSize(mapWidgetSize * mNumCells, mapWidgetSize * mNumCells);

        const auto size = MyGUI::IntSize(std::ceil(mapWidgetSize), std::ceil(mapWidgetSize));
        for (auto& entry : mMaps)
        {
            const auto position = getPosition(entry.mCellX, entry.mCellY, 0, 0);
            entry.mMapWidget->setCoord({ position, size });
            entry.mFogWidget->setCoord({ position, size });
        }

        MarkerUserData markerPos(mLocalMapRender);
        for (MyGUI::Widget* widget : currentDoorMarkersWidgets())
            widget->setCoord(getMarkerCoordinates(widget, 8));

        for (MyGUI::Widget* widget : mCustomMarkerWidgets)
        {
            const auto& marker = *widget->getUserData<ESM::CustomMarker>();
            widget->setCoord(getMarkerCoordinates(marker.mWorldX, marker.mWorldY, markerPos, 16));
        }

        for (MyGUI::Widget* widget : mMagicMarkerWidgets)
            widget->setCoord(getMarkerCoordinates(widget, 8));
    }

    // ------------------------------------------------------------------------------------------
    MapWindow::MapWindow(CustomMarkerCollection& customMarkers, DragAndDrop* drag, MWRender::LocalMap* localMapRender,
        SceneUtil::WorkQueue* workQueue)
#ifdef USE_OPENXR
        : WindowPinnableBase("openmw_map_window_vr.layout")
#else
        : WindowPinnableBase("openmw_map_window.layout")
#endif
        , LocalMapBase(customMarkers, localMapRender, true)
        , NoDrop(drag, mMainWidget)
        , mGlobalMap(nullptr)
        , mGlobalMapImage(nullptr)
        , mGlobalMapOverlay(nullptr)
        , mEventBoxGlobal(nullptr)
        , mEventBoxLocal(nullptr)
        , mGlobalMapRender(std::make_unique<MWRender::GlobalMap>(localMapRender->getRoot(), workQueue))
        , mEditNoteDialog()
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

        setCoord(500, 0, 320, 300);

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

        mGlobalMap->setVisible(false);

        getWidget(mButton, "WorldButton");
        mButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MapWindow::onWorldButtonClicked);

        const bool global = Settings::map().mGlobal;

        mButton->setCaptionWithReplacing(global ? "#{sLocal}" : "#{sWorld}");

        getWidget(mEventBoxGlobal, "EventBoxGlobal");
        mEventBoxGlobal->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        mEventBoxGlobal->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);

        const bool allowZooming = Settings::map().mAllowZooming;

        if (allowZooming)
            mEventBoxGlobal->eventMouseWheel += MyGUI::newDelegate(this, &MapWindow::onMapZoomed);
        mEventBoxGlobal->setDepth(Global_ExploreOverlayLayer);

        getWidget(mEventBoxLocal, "EventBoxLocal");
        mEventBoxLocal->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        mEventBoxLocal->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);
        mEventBoxLocal->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &MapWindow::onMapDoubleClicked);
        if (allowZooming)
            mEventBoxLocal->eventMouseWheel += MyGUI::newDelegate(this, &MapWindow::onMapZoomed);

        LocalMapBase::init(mLocalMap, mPlayerArrowLocal, getLocalViewingDistance());

        mGlobalMap->setVisible(global);
        mLocalMap->setVisible(!global);
    }

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
        confirmation->askForConfirmation("#{sDeleteNote}");
        confirmation->eventCancelClicked.clear();
        confirmation->eventOkClicked.clear();
        confirmation->eventOkClicked += MyGUI::newDelegate(this, &MapWindow::onNoteEditDeleteConfirm);
    }

    void MapWindow::onNoteEditDeleteConfirm()
    {
        mCustomMarkers.deleteMarker(mEditingMarker);

        mEditNoteDialog.setVisible(false);
    }

    void MapWindow::onCustomMarkerDoubleClicked(MyGUI::Widget* sender)
    {
        mEditingMarker = *sender->getUserData<ESM::CustomMarker>();
        mEditNoteDialog.setText(mEditingMarker.mNote);
        mEditNoteDialog.showDeleteButton(true);
        mEditNoteDialog.setVisible(true);
    }

    void MapWindow::onMapDoubleClicked(MyGUI::Widget* sender)
    {
        MyGUI::IntPoint clickedPos = MyGUI::InputManager::getInstance().getMousePosition();

        MyGUI::IntPoint widgetPos = clickedPos - mEventBoxLocal->getAbsolutePosition();
        auto mapWidgetSize = getWidgetSize();
        int x = int(widgetPos.left / float(mapWidgetSize)) - mCellDistance;
        int y = (int(widgetPos.top / float(mapWidgetSize)) - mCellDistance) * -1;
        float nX = widgetPos.left / float(mapWidgetSize) - int(widgetPos.left / float(mapWidgetSize));
        float nY = widgetPos.top / float(mapWidgetSize) - int(widgetPos.top / float(mapWidgetSize));
        x += mActiveCell->getGridX();
        y += mActiveCell->getGridY();

        osg::Vec2f worldPos;
        if (!mActiveCell->isExterior())
        {
            worldPos = mLocalMapRender->interiorMapToWorldPosition(nX, nY, x, y);
        }
        else
        {
            worldPos.x() = (x + nX) * cellSize;
            worldPos.y() = (y + (1.0f - nY)) * cellSize;
        }

        mEditingMarker.mWorldX = worldPos.x();
        mEditingMarker.mWorldY = worldPos.y();
        ESM::RefId clickedId = getCellIdInWorldSpace(*mActiveCell, x, y);

        mEditingMarker.mCell = clickedId;

        mEditNoteDialog.setVisible(true);
        mEditNoteDialog.showDeleteButton(false);
        mEditNoteDialog.setText({});
    }

    void MapWindow::onMapZoomed(MyGUI::Widget* sender, int rel)
    {
        const int localWidgetSize = Settings::map().mLocalMapWidgetSize;
        const bool zoomOut = rel < 0;
        const bool zoomIn = !zoomOut;
        const double speedDiff = zoomOut ? 1.0 / speed : speed;
        const float localMapSizeInUnits = localWidgetSize * mNumCells;

        const float currentMinLocalMapZoom = std::max({ (float(Settings::map().mGlobalMapCellSize) * 4.f)
                / float(localWidgetSize),
            float(mLocalMap->getWidth()) / localMapSizeInUnits, float(mLocalMap->getHeight()) / localMapSizeInUnits });

        if (Settings::map().mGlobal)
        {
            const float currentGlobalZoom = mGlobalMapZoom;
            const float currentMinGlobalMapZoom
                = std::min(float(mGlobalMap->getWidth()) / float(mGlobalMapRender->getWidth()),
                    float(mGlobalMap->getHeight()) / float(mGlobalMapRender->getHeight()));

            mGlobalMapZoom *= speedDiff;

            if (zoomIn && mGlobalMapZoom > 4.f)
            {
                mGlobalMapZoom = currentGlobalZoom;
                mLocalMapZoom = currentMinLocalMapZoom;
                onWorldButtonClicked(nullptr);
                updateLocalMap();
                return; // the zoom in is too big
            }

            if (zoomOut && mGlobalMapZoom < currentMinGlobalMapZoom)
            {
                mGlobalMapZoom = currentGlobalZoom;
                return; // the zoom out is too big, we have reach the borders of the widget
            }
        }
        else
        {
            auto const currentLocalZoom = mLocalMapZoom;
            mLocalMapZoom *= speedDiff;

            if (zoomIn && mLocalMapZoom > 4.0f)
            {
                mLocalMapZoom = currentLocalZoom;
                return; // the zoom in is too big
            }

            if (zoomOut && mLocalMapZoom < currentMinLocalMapZoom)
            {
                mLocalMapZoom = currentLocalZoom;

                float zoomRatio = 4.f / mGlobalMapZoom;
                mGlobalMapZoom = 4.f;
                onWorldButtonClicked(nullptr);

                zoomOnCursor(zoomRatio);
                return; // the zoom out is too big, we switch to the global map
            }

            if (zoomOut)
                mNeedDoorMarkersUpdate = true;
        }
        zoomOnCursor(speedDiff);
    }

    void MapWindow::zoomOnCursor(float speedDiff)
    {
        auto map = Settings::map().mGlobal ? mGlobalMap : mLocalMap;
        auto cursor = MyGUI::InputManager::getInstance().getMousePosition() - map->getAbsolutePosition();
        auto centerView = map->getViewOffset() - cursor;

        Settings::map().mGlobal ? updateGlobalMap() : updateLocalMap();

        map->setViewOffset(MyGUI::IntPoint(std::round(centerView.left * speedDiff) + cursor.left,
            std::round(centerView.top * speedDiff) + cursor.top));
    }

    void MapWindow::updateGlobalMap()
    {
        resizeGlobalMap();

        float x = mCurPos.x(), y = mCurPos.y();
        if (!mActiveCell->isExterior())
        {
            auto pos = MWBase::Environment::get().getWorld()->getPlayer().getLastKnownExteriorPosition();
            x = pos.x();
            y = pos.y();
        }
        setGlobalMapPlayerPosition(x, y);

        for (auto& [marker, col] : mGlobalMapMarkers)
        {
            marker.widget->setCoord(createMarkerCoords(marker.position.x(), marker.position.y(), col.size()));
            marker.widget->setVisible(marker.widget->getHeight() >= 6);
        }
    }

    void MapWindow::onChangeScrollWindowCoord(MyGUI::Widget* sender)
    {
        MyGUI::IntCoord currentCoordinates = sender->getCoord();

        MyGUI::IntPoint currentViewPortCenter
            = MyGUI::IntPoint(currentCoordinates.width / 2, currentCoordinates.height / 2);
        MyGUI::IntPoint lastViewPortCenter
            = MyGUI::IntPoint(mLastScrollWindowCoordinates.width / 2, mLastScrollWindowCoordinates.height / 2);
        MyGUI::IntPoint viewPortCenterDiff = currentViewPortCenter - lastViewPortCenter;

        mLocalMap->setViewOffset(mLocalMap->getViewOffset() + viewPortCenterDiff);
        mGlobalMap->setViewOffset(mGlobalMap->getViewOffset() + viewPortCenterDiff);

        mLastScrollWindowCoordinates = currentCoordinates;
    }

    void MapWindow::setVisible(bool visible)
    {
        WindowBase::setVisible(visible);
        mButton->setVisible(visible && MWBase::Environment::get().getWindowManager()->getMode() != MWGui::GM_None);
    }

    void MapWindow::renderGlobalMap()
    {
        mGlobalMapRender->render();
        resizeGlobalMap();
    }

    MapWindow::~MapWindow() = default;

    void MapWindow::setCellName(const std::string& cellName)
    {
        setTitle("#{sCell=" + cellName + "}");
    }

    MyGUI::IntCoord MapWindow::createMarkerCoords(float x, float y, float agregatedWeight) const
    {
        float worldX, worldY;
        worldPosToGlobalMapImageSpace(
            (x + 0.5f) * Constants::CellSizeInUnits, (y + 0.5f) * Constants::CellSizeInUnits, worldX, worldY);

        const float markerSize = getMarkerSize(agregatedWeight);
        const float halfMarkerSize = markerSize / 2.0f;
        return MyGUI::IntCoord(static_cast<int>(worldX - halfMarkerSize), static_cast<int>(worldY - halfMarkerSize),
            markerSize, markerSize);
    }

    MyGUI::Widget* MapWindow::createMarker(const std::string& name, float x, float y, float agregatedWeight)
    {
        MyGUI::Widget* markerWidget = mGlobalMap->createWidget<MyGUI::Widget>(
            "MarkerButton", createMarkerCoords(x, y, agregatedWeight), MyGUI::Align::Default);
        markerWidget->setVisible(markerWidget->getHeight() >= 6.0);
        markerWidget->setUserString("Caption_TextOneLine", "#{sCell=" + name + "}");
        setGlobalMapMarkerTooltip(markerWidget, x, y);

        markerWidget->setUserString("ToolTipLayout", "TextToolTipOneLine");

        markerWidget->setNeedMouseFocus(true);
        markerWidget->setColour(
            MyGUI::Colour::parse(MyGUI::LanguageManager::getInstance().replaceTags("#{fontcolour=normal}")));
        markerWidget->setDepth(Global_MarkerLayer);
        markerWidget->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        if (Settings::map().mAllowZooming)
            markerWidget->eventMouseWheel += MyGUI::newDelegate(this, &MapWindow::onMapZoomed);
        markerWidget->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);

        return markerWidget;
    }

    void MapWindow::addVisitedLocation(const std::string& name, int x, int y)
    {
        CellId cell;
        cell.first = x;
        cell.second = y;
        if (mMarkers.insert(cell).second)
        {
            MapMarkerType mapMarkerWidget = { osg::Vec2f(x, y), createMarker(name, x, y, 0) };
            mGlobalMapMarkers.emplace(mapMarkerWidget, std::vector<MapMarkerType>());

            std::string name_ = name.substr(0, name.find(','));
            auto& entry = mGlobalMapMarkersByName[name_];
            if (!entry.widget)
            {
                entry = { osg::Vec2f(x, y), entry.widget }; // update the coords

                entry.widget = createMarker(name_, entry.position.x(), entry.position.y(), 1);
                mGlobalMapMarkers.emplace(entry, std::vector<MapMarkerType>{ entry });
            }
            else
            {
                auto it = mGlobalMapMarkers.find(entry);
                auto& marker = const_cast<MapMarkerType&>(it->first);
                auto& elements = it->second;
                elements.emplace_back(mapMarkerWidget);

                // we compute the barycenter of the entry elements => it will be the place on the world map for the
                // agregated widget
                marker.position = std::accumulate(elements.begin(), elements.end(), osg::Vec2f(0.f, 0.f),
                                      [](const auto& left, const auto& right) { return left + right.position; })
                    / float(elements.size());

                marker.widget->setCoord(createMarkerCoords(marker.position.x(), marker.position.y(), elements.size()));
                marker.widget->setVisible(marker.widget->getHeight() >= 6);
            }
        }
    }

    void MapWindow::cellExplored(int x, int y)
    {
        mGlobalMapRender->cleanupCameras();
        mGlobalMapRender->exploreCell(x, y, mLocalMapRender->getMapTexture(x, y));
    }

    void MapWindow::onFrame(float dt)
    {
        LocalMapBase::onFrame(dt);
        NoDrop::onFrame(dt);
    }

    void MapWindow::setGlobalMapMarkerTooltip(MyGUI::Widget* markerWidget, int x, int y)
    {
        ESM::RefId cellRefId = ESM::RefId::esm3ExteriorCell(x, y);
        CustomMarkerCollection::RangeType markers = mCustomMarkers.getMarkers(cellRefId);
        std::vector<std::string> destNotes;
        for (CustomMarkerCollection::ContainerType::const_iterator it = markers.first; it != markers.second; ++it)
            destNotes.push_back(it->second.mNote);

        if (!destNotes.empty())
        {
            MarkerUserData data(nullptr);
            std::swap(data.notes, destNotes);
            data.caption = markerWidget->getUserString("Caption_TextOneLine");
            markerWidget->setUserData(data);
            markerWidget->setUserString("ToolTipType", "MapMarker");
        }
        else
        {
            markerWidget->setUserString("ToolTipType", "Layout");
        }
    }

    float MapWindow::getMarkerSize(size_t agregatedWeight) const
    {
        float markerSize = 12.f * mGlobalMapZoom;
        if (mGlobalMapZoom < 1)
            return markerSize * std::sqrt(agregatedWeight); // we want to see agregated object
        return agregatedWeight ? 0 : markerSize; // we want to see only original markers (i.e. non agregated)
    }

    void MapWindow::resizeGlobalMap()
    {
        mGlobalMap->setCanvasSize(
            mGlobalMapRender->getWidth() * mGlobalMapZoom, mGlobalMapRender->getHeight() * mGlobalMapZoom);
        mGlobalMapImage->setSize(
            mGlobalMapRender->getWidth() * mGlobalMapZoom, mGlobalMapRender->getHeight() * mGlobalMapZoom);
    }

    void MapWindow::worldPosToGlobalMapImageSpace(float x, float y, float& imageX, float& imageY) const
    {
        mGlobalMapRender->worldPosToImageSpace(x, y, imageX, imageY);
        imageX *= mGlobalMapZoom;
        imageY *= mGlobalMapZoom;
    }

    void MapWindow::updateCustomMarkers()
    {
        LocalMapBase::updateCustomMarkers();

        for (auto& [widgetPair, ignore] : mGlobalMapMarkers)
            setGlobalMapMarkerTooltip(widgetPair.widget, widgetPair.position.x(), widgetPair.position.y());
    }

    void MapWindow::onDragStart(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        if (_id != MyGUI::MouseButton::Left)
            return;
        mLastDragPos = MyGUI::IntPoint(_left, _top);
    }

    void MapWindow::onMouseDrag(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
    {
        if (_id != MyGUI::MouseButton::Left)
            return;

        MyGUI::IntPoint diff = MyGUI::IntPoint(_left, _top) - mLastDragPos;

        if (!Settings::map().mGlobal)
        {
            mNeedDoorMarkersUpdate = true;
            mLocalMap->setViewOffset(mLocalMap->getViewOffset() + diff);
        }
        else
            mGlobalMap->setViewOffset(mGlobalMap->getViewOffset() + diff);

        mLastDragPos = MyGUI::IntPoint(_left, _top);
    }

    void MapWindow::onWorldButtonClicked(MyGUI::Widget* _sender)
    {
        const bool global = !Settings::map().mGlobal;

        Settings::map().mGlobal.set(global);

        mGlobalMap->setVisible(global);
        mLocalMap->setVisible(!global);

        mButton->setCaptionWithReplacing(global ? "#{sLocal}" : "#{sWorld}");
    }

    void MapWindow::onPinToggled()
    {
        Settings::windows().mMapPin.set(mPinned);

        MWBase::Environment::get().getWindowManager()->setMinimapVisibility(!mPinned);
    }

    void MapWindow::onTitleDoubleClicked()
    {
        if (MyGUI::InputManager::getInstance().isShiftPressed())
            MWBase::Environment::get().getWindowManager()->toggleMaximized(this);
        else if (!mPinned)
            MWBase::Environment::get().getWindowManager()->toggleVisible(GW_Map);
    }

    void MapWindow::onOpen()
    {
        ensureGlobalMapLoaded();

        globalMapUpdatePlayer();
    }

    void MapWindow::globalMapUpdatePlayer()
    {
        // For interiors, position is set by WindowManager via setGlobalMapPlayerPosition
        if (MWBase::Environment::get().getWorld()->isCellExterior())
        {
            osg::Vec3f pos = MWBase::Environment::get().getWorld()->getPlayerPtr().getRefData().getPosition().asVec3();
            setGlobalMapPlayerPosition(pos.x(), pos.y());
        }
    }

    void MapWindow::notifyPlayerUpdate()
    {
        globalMapUpdatePlayer();

        setGlobalMapPlayerDir(mLastDirectionX, mLastDirectionY);
    }

    void MapWindow::centerView()
    {
        LocalMapBase::centerView();
        // set the view offset so that player is in the center
        MyGUI::IntSize viewsize = mGlobalMap->getSize();
        MyGUI::IntPoint pos = mPlayerArrowGlobal->getPosition() + MyGUI::IntPoint{ 16, 16 };
        MyGUI::IntPoint viewoffs(
            static_cast<int>(viewsize.width * 0.5f - pos.left), static_cast<int>(viewsize.height * 0.5f - pos.top));
        mGlobalMap->setViewOffset(viewoffs);
    }

    void MapWindow::setGlobalMapPlayerPosition(float worldX, float worldY)
    {
        float x, y;
        worldPosToGlobalMapImageSpace(worldX, worldY, x, y);
        mPlayerArrowGlobal->setPosition(MyGUI::IntPoint(static_cast<int>(x - 16), static_cast<int>(y - 16)));
    }

    void MapWindow::setGlobalMapPlayerDir(const float x, const float y)
    {
        MyGUI::ISubWidget* main = mPlayerArrowGlobal->getSubWidgetMain();
        MyGUI::RotatingSkin* rotatingSubskin = main->castType<MyGUI::RotatingSkin>();
        rotatingSubskin->setCenter(MyGUI::IntPoint(16, 16));
        float angle = std::atan2(x, y);
        rotatingSubskin->setAngle(angle);
    }

    void MapWindow::ensureGlobalMapLoaded()
    {
        if (!mGlobalMapTexture.get())
        {
            mGlobalMapTexture = std::make_unique<osgMyGUI::OSGTexture>(mGlobalMapRender->getBaseTexture());
            mGlobalMapImage->setRenderItemTexture(mGlobalMapTexture.get());
            mGlobalMapImage->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, 1.f, 1.f));

            mGlobalMapOverlayTexture = std::make_unique<osgMyGUI::OSGTexture>(mGlobalMapRender->getOverlayTexture());
            mGlobalMapOverlay->setRenderItemTexture(mGlobalMapOverlayTexture.get());
            mGlobalMapOverlay->getSubWidgetMain()->_setUVSet(MyGUI::FloatRect(0.f, 0.f, 1.f, 1.f));

            // Redraw children in proper order
            mGlobalMap->getParent()->_updateChilds();
        }
    }

    void MapWindow::clear()
    {
        mMarkers.clear();

        mGlobalMapRender->clear();
        mActiveCell = nullptr;

        for (auto& widgetPair : mGlobalMapMarkers)
            MyGUI::Gui::getInstance().destroyWidget(widgetPair.first.widget);
        mGlobalMapMarkers.clear();
        mGlobalMapMarkersByName.clear();
    }

    void MapWindow::write(ESM::ESMWriter& writer, Loading::Listener& progress)
    {
        ESM::GlobalMap map;
        mGlobalMapRender->write(map);

        map.mMarkers = mMarkers;

        writer.startRecord(ESM::REC_GMAP);
        map.save(writer);
        writer.endRecord(ESM::REC_GMAP);
    }

    void MapWindow::readRecord(ESM::ESMReader& reader, uint32_t type)
    {
        if (type == ESM::REC_GMAP)
        {
            ESM::GlobalMap map;
            map.load(reader);

            mGlobalMapRender->read(map);

            for (const ESM::GlobalMap::CellId& cellId : map.mMarkers)
            {
                const ESM::Cell* cell
                    = MWBase::Environment::get().getESMStore()->get<ESM::Cell>().search(cellId.first, cellId.second);
                if (cell && !cell->mName.empty())
                    addVisitedLocation(cell->mName, cellId.first, cellId.second);
            }
        }
    }

    void MapWindow::setAlpha(float alpha)
    {
        NoDrop::setAlpha(alpha);
        // can't allow showing map with partial transparency, as the fog of war will also go transparent
        // and reveal parts of the map you shouldn't be able to see
        for (MapEntry& entry : mMaps)
            entry.mMapWidget->setVisible(alpha == 1);
    }

    void MapWindow::customMarkerCreated(MyGUI::Widget* marker)
    {
        marker->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        marker->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);
        marker->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &MapWindow::onCustomMarkerDoubleClicked);
        if (Settings::map().mAllowZooming)
            marker->eventMouseWheel += MyGUI::newDelegate(this, &MapWindow::onMapZoomed);
    }

    void MapWindow::doorMarkerCreated(MyGUI::Widget* marker)
    {
        marker->eventMouseDrag += MyGUI::newDelegate(this, &MapWindow::onMouseDrag);
        marker->eventMouseButtonPressed += MyGUI::newDelegate(this, &MapWindow::onDragStart);
        if (Settings::map().mAllowZooming)
            marker->eventMouseWheel += MyGUI::newDelegate(this, &MapWindow::onMapZoomed);
    }

    void MapWindow::asyncPrepareSaveMap()
    {
        mGlobalMapRender->asyncWritePng();
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

    void EditNoteDialog::setText(const std::string& text)
    {
        mTextEdit->setCaption(MyGUI::TextIterator::toTagsString(text));
    }

    std::string EditNoteDialog::getText()
    {
        return MyGUI::TextIterator::getOnlyText(mTextEdit->getCaption());
    }

    void EditNoteDialog::onOpen()
    {
        WindowModal::onOpen();
        center();
        MWBase::Environment::get().getWindowManager()->setKeyFocusWidget(mTextEdit);
    }

    void EditNoteDialog::onCancelButtonClicked(MyGUI::Widget* sender)
    {
        setVisible(false);
    }

    void EditNoteDialog::onOkButtonClicked(MyGUI::Widget* sender)
    {
        eventOkClicked();
    }

    void EditNoteDialog::onDeleteButtonClicked(MyGUI::Widget* sender)
    {
        eventDeleteClicked();
    }

    bool LocalMapBase::MarkerUserData::isPositionExplored() const
    {
        if (!mLocalMapRender)
            return true;
        return mLocalMapRender->isPositionExplored(nX, nY, cellX, cellY);
    }

}
