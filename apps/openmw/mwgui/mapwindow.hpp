#ifndef MWGUI_MAPWINDOW_H
#define MWGUI_MAPWINDOW_H

#include <cstdint>
#include <memory>

#include <osg/Vec2f>

#include <MyGUI_ITexture.h>

#include "windowpinnablebase.hpp"

#include <components/esm3/custommarkerstate.hpp>
#include <components/misc/constants.hpp>

namespace MWRender
{
    class GlobalMap;
    class LocalMap;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace MWWorld
{
    class Cell;
    class CellStore;
}

namespace Loading
{
    class Listener;
}

namespace SceneUtil
{
    class WorkQueue;
}

namespace MWGui
{

    class CustomMarkerCollection
    {
    public:
        void addMarker(const ESM::CustomMarker& marker, bool triggerEvent = true);
        void deleteMarker(const ESM::CustomMarker& marker);
        void updateMarker(const ESM::CustomMarker& marker, const std::string& newNote);

        void clear();

        size_t size() const;

        typedef std::multimap<ESM::RefId, ESM::CustomMarker> ContainerType;

        typedef std::pair<ContainerType::const_iterator, ContainerType::const_iterator> RangeType;

        ContainerType::const_iterator begin() const;
        ContainerType::const_iterator end() const;

        RangeType getMarkers(const ESM::RefId& cellId) const;

        typedef MyGUI::delegates::MultiDelegate<> EventHandle_Void;
        EventHandle_Void eventMarkersChanged;

    private:
        ContainerType mMarkers;
    };

    class LocalMapBase
    {
    public:
        LocalMapBase(CustomMarkerCollection& markers, MWRender::LocalMap* localMapRender, bool fogOfWarEnabled);
        virtual ~LocalMapBase();
        void init(MyGUI::ScrollView* widget, MyGUI::ImageBox* compass, int cellDistance = Constants::CellGridRadius);

        void setActiveCell(const MWWorld::Cell& cell);
        void requestMapRender(const MWWorld::CellStore* cell);
        void setPlayerDir(const float x, const float y);
        void setPlayerPos(int cellX, int cellY, const float nx, const float ny);

        void onFrame(float dt);

        bool toggleFogOfWar();

        struct MarkerUserData
        {
            MarkerUserData(MWRender::LocalMap* map)
                : mLocalMapRender(map)
                , cellX(0)
                , cellY(0)
                , nX(0.f)
                , nY(0.f)
            {
            }

            bool isPositionExplored() const;

            MWRender::LocalMap* mLocalMapRender;
            int cellX;
            int cellY;
            float nX;
            float nY;
            std::vector<std::string> notes;
            std::string caption;
        };

    protected:
        void updateLocalMap();

        MWRender::LocalMap* mLocalMapRender;
        const MWWorld::Cell* mActiveCell = nullptr;
        osg::Vec2f mCurPos; // the position of the player in the world (in cell coords)

        MyGUI::ScrollView* mLocalMap = nullptr;
        MyGUI::ImageBox* mCompass = nullptr;
        float mLocalMapZoom = 1.f;
        bool mHasALastActiveCell = false;
        bool mFogOfWarToggled = true;
        bool mFogOfWarEnabled;
        bool mNeedDoorMarkersUpdate = false;

        // Stores markers that were placed by a player. May be shared between multiple map views.
        CustomMarkerCollection& mCustomMarkers;

        struct MapEntry
        {
            MapEntry(MyGUI::ImageBox* mapWidget, MyGUI::ImageBox* fogWidget)
                : mMapWidget(mapWidget)
                , mFogWidget(fogWidget)
                , mCellX(0)
                , mCellY(0)
            {
            }

            MyGUI::ImageBox* mMapWidget;
            MyGUI::ImageBox* mFogWidget;
            std::unique_ptr<MyGUI::ITexture> mMapTexture;
            std::unique_ptr<MyGUI::ITexture> mFogTexture;
            int mCellX;
            int mCellY;
        };
        std::vector<MapEntry> mMaps;

        // Keep track of created marker widgets, just to easily remove them later.
        std::vector<MyGUI::Widget*> mExteriorDoorMarkerWidgets;
        std::map<std::pair<int, int>, std::vector<MyGUI::Widget*>> mExteriorDoorsByCell;
        std::vector<MyGUI::Widget*> mInteriorDoorMarkerWidgets;
        std::vector<MyGUI::Widget*> mMagicMarkerWidgets;
        std::vector<MyGUI::Widget*> mCustomMarkerWidgets;
        std::vector<MyGUI::Widget*> mDoorMarkersToRecycle;

        std::vector<MyGUI::Widget*>& currentDoorMarkersWidgets();

        virtual void updateCustomMarkers();

        void applyFogOfWar();

        MyGUI::IntPoint getPosition(int cellX, int cellY, float nx, float ny) const;
        MyGUI::IntPoint getMarkerPosition(float worldX, float worldY, MarkerUserData& markerPos) const;
        MyGUI::IntCoord getMarkerCoordinates(
            float worldX, float worldY, MarkerUserData& markerPos, size_t markerSize) const;
        MyGUI::Widget* createDoorMarker(const std::string& name, float x, float y) const;
        MyGUI::IntCoord getMarkerCoordinates(MyGUI::Widget* widget, size_t markerSize) const;

        virtual void notifyPlayerUpdate() {}
        virtual void centerView();
        virtual void notifyMapChanged() {}

        virtual void customMarkerCreated(MyGUI::Widget* marker) {}
        virtual void doorMarkerCreated(MyGUI::Widget* marker) {}

        void updateRequiredMaps();

        void updateMagicMarkers();
        void addDetectionMarkers(int type);

        void redraw();
        float getWidgetSize() const;

        MWGui::LocalMapBase::MapEntry& addMapEntry();

        MyGUI::IntRect mGrid{ -1, -1, 1, 1 };
        int mExtCellDistance = 0;
        float mMarkerUpdateTimer = 0.f;

        float mLastDirectionX = 0.f;
        float mLastDirectionY = 0.f;

    private:
        void updateDoorMarkers();
    };

    class EditNoteDialog : public MWGui::WindowModal
    {
    public:
        EditNoteDialog();

        void onOpen() override;

        void showDeleteButton(bool show);
        bool getDeleteButtonShown();
        void setText(const std::string& text);
        std::string getText();

        typedef MyGUI::delegates::MultiDelegate<> EventHandle_Void;

        EventHandle_Void eventDeleteClicked;
        EventHandle_Void eventOkClicked;

        ControllerButtonStr* getControllerButtons() override;

    private:
        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onOkButtonClicked(MyGUI::Widget* sender);
        void onDeleteButtonClicked(MyGUI::Widget* sender);

        MyGUI::TextBox* mTextEdit;
        MyGUI::Button* mOkButton;
        MyGUI::Button* mCancelButton;
        MyGUI::Button* mDeleteButton;

        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        int mControllerFocus;
    };

    class MapWindow : public MWGui::WindowPinnableBase, public LocalMapBase, public NoDrop
    {
    public:
        MapWindow(CustomMarkerCollection& customMarkers, DragAndDrop* drag, MWRender::LocalMap* localMapRender,
            SceneUtil::WorkQueue* workQueue);
        virtual ~MapWindow();

        void setCellName(const std::string& cellName);

        void setAlpha(float alpha) override;
        void setVisible(bool visible) override;

        void renderGlobalMap();

        /// adds the marker to the global map
        /// @param name The ESM::Cell::mName
        void addVisitedLocation(const std::string& name, int x, int y);

        // reveals this cell's map on the global map
        void cellExplored(int x, int y);

        void setGlobalMapPlayerPosition(float worldX, float worldY);
        void setGlobalMapPlayerDir(const float x, const float y);

        void ensureGlobalMapLoaded();

        void onOpen() override;

        void onFrame(float dt) override;

        void updateCustomMarkers() override;

        /// Clear all savegame-specific data
        void clear() override;

        void write(ESM::ESMWriter& writer, Loading::Listener& progress);
        void readRecord(ESM::ESMReader& reader, uint32_t type);

        void asyncPrepareSaveMap();

        std::string_view getWindowIdForLua() const override { return "Map"; }

    protected:
        bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) override;
        void setActiveControllerWindow(bool active) override;

    private:
        void onDragStart(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onMouseDrag(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onWorldButtonClicked(MyGUI::Widget* _sender);
        void onMapDoubleClicked(MyGUI::Widget* sender);
        void onMapZoomed(MyGUI::Widget* sender, int rel);
        void zoomOnCursor(float speedDiff);
        void updateGlobalMap();
        void onCustomMarkerDoubleClicked(MyGUI::Widget* sender);
        void onNoteEditOk();
        void onNoteEditDelete();
        void onNoteEditDeleteConfirm();
        void onNoteDoubleClicked(MyGUI::Widget* sender);
        void onChangeScrollWindowCoord(MyGUI::Widget* sender);
        void globalMapUpdatePlayer();
        void setGlobalMapMarkerTooltip(MyGUI::Widget* widget, int x, int y);
        float getMarkerSize(size_t agregatedWeight) const;
        void resizeGlobalMap();
        void shiftMap(int dx, int dy);
        void worldPosToGlobalMapImageSpace(float x, float z, float& imageX, float& imageY) const;
        MyGUI::IntCoord createMarkerCoords(float x, float y, float agregatedWeight) const;
        MyGUI::Widget* createMarker(const std::string& name, float x, float y, float agregatedWeight);

        MyGUI::ScrollView* mGlobalMap;
        std::unique_ptr<MyGUI::ITexture> mGlobalMapTexture;
        std::unique_ptr<MyGUI::ITexture> mGlobalMapOverlayTexture;
        MyGUI::ImageBox* mGlobalMapImage;
        MyGUI::ImageBox* mGlobalMapOverlay;
        MyGUI::ImageBox* mPlayerArrowLocal;
        MyGUI::ImageBox* mPlayerArrowGlobal;
        MyGUI::Button* mButton;
        MyGUI::IntPoint mLastDragPos;

        MyGUI::IntCoord mLastScrollWindowCoordinates;

        // Markers on global map
        typedef std::pair<int, int> CellId;
        std::set<CellId> mMarkers;

        MyGUI::Button* mEventBoxGlobal;
        MyGUI::Button* mEventBoxLocal;

        float mGlobalMapZoom = 1.0f;
        std::unique_ptr<MWRender::GlobalMap> mGlobalMapRender;

        struct MapMarkerType
        {
            osg::Vec2f position;
            MyGUI::Widget* widget = nullptr;

            bool operator<(const MapMarkerType& right) const { return widget < right.widget; }
        };

        std::map<std::string, MapMarkerType> mGlobalMapMarkersByName;
        std::map<MapMarkerType, std::vector<MapMarkerType>> mGlobalMapMarkers;

        EditNoteDialog mEditNoteDialog;
        ESM::CustomMarker mEditingMarker;

        void onPinToggled() override;
        void onTitleDoubleClicked() override;

        void doorMarkerCreated(MyGUI::Widget* marker) override;
        void customMarkerCreated(MyGUI::Widget* marker) override;

        void notifyPlayerUpdate() override;

        void centerView() override;
    };
}
#endif
