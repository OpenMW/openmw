#ifndef MWGUI_MAPWINDOW_H
#define MWGUI_MAPWINDOW_H

#include <stdint.h>
#include <memory>

#include "windowpinnablebase.hpp"

#include <components/esm/cellid.hpp>

#include <components/esm/custommarkerstate.hpp>

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
        void addMarker(const ESM::CustomMarker& marker, bool triggerEvent=true);
        void deleteMarker (const ESM::CustomMarker& marker);
        void updateMarker(const ESM::CustomMarker& marker, const std::string& newNote);

        void clear();

        size_t size() const;

        typedef std::multimap<ESM::CellId, ESM::CustomMarker> ContainerType;

        typedef std::pair<ContainerType::const_iterator, ContainerType::const_iterator> RangeType;

        ContainerType::const_iterator begin() const;
        ContainerType::const_iterator end() const;

        RangeType getMarkers(const ESM::CellId& cellId) const;

        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;
        EventHandle_Void eventMarkersChanged;

    private:
        ContainerType mMarkers;
    };

    class LocalMapBase
    {
    public:
        LocalMapBase(CustomMarkerCollection& markers, MWRender::LocalMap* localMapRender, bool fogOfWarEnabled = true);
        virtual ~LocalMapBase();
        void init(MyGUI::ScrollView* widget, MyGUI::ImageBox* compass, int mapWidgetSize, int cellDistance);

        void setCellPrefix(const std::string& prefix);
        void setActiveCell(const int x, const int y, bool interior=false);
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
        MWRender::LocalMap* mLocalMapRender;

        int mCurX, mCurY;
        bool mInterior;
        MyGUI::ScrollView* mLocalMap;
        MyGUI::ImageBox* mCompass;
        std::string mPrefix;
        bool mChanged;
        bool mFogOfWarToggled;
        bool mFogOfWarEnabled;

        int mMapWidgetSize;

        int mNumCells; // for convenience, mCellDistance * 2 + 1
        int mCellDistance;

        // Stores markers that were placed by a player. May be shared between multiple map views.
        CustomMarkerCollection& mCustomMarkers;

        struct MapEntry
        {
            MapEntry(MyGUI::ImageBox* mapWidget, MyGUI::ImageBox* fogWidget)
                : mMapWidget(mapWidget), mFogWidget(fogWidget), mCellX(0), mCellY(0) {}

            MyGUI::ImageBox* mMapWidget;
            MyGUI::ImageBox* mFogWidget;
            std::shared_ptr<MyGUI::ITexture> mMapTexture;
            std::shared_ptr<MyGUI::ITexture> mFogTexture;
            int mCellX;
            int mCellY;
        };
        std::vector<MapEntry> mMaps;

        // Keep track of created marker widgets, just to easily remove them later.
        std::vector<MyGUI::Widget*> mDoorMarkerWidgets;
        std::vector<MyGUI::Widget*> mMagicMarkerWidgets;
        std::vector<MyGUI::Widget*> mCustomMarkerWidgets;

        virtual void updateCustomMarkers();

        void applyFogOfWar();

        MyGUI::IntPoint getMarkerPosition (float worldX, float worldY, MarkerUserData& markerPos);

        virtual void notifyPlayerUpdate() {}
        virtual void notifyMapChanged() {}

        virtual void customMarkerCreated(MyGUI::Widget* marker) {}
        virtual void doorMarkerCreated(MyGUI::Widget* marker) {}

        void updateRequiredMaps();

        void updateMagicMarkers();
        void addDetectionMarkers(int type);

        void redraw();

        float mMarkerUpdateTimer;

        float mLastDirectionX;
        float mLastDirectionY;

    private:
        void updateDoorMarkers();
        bool mNeedDoorMarkersUpdate;
    };

    class EditNoteDialog : public MWGui::WindowModal
    {
    public:
        EditNoteDialog();

        virtual void onOpen();

        void showDeleteButton(bool show);
        bool getDeleteButtonShown();
        void setText(const std::string& text);
        std::string getText();

        typedef MyGUI::delegates::CMultiDelegate0 EventHandle_Void;

        EventHandle_Void eventDeleteClicked;
        EventHandle_Void eventOkClicked;

    private:
        void onCancelButtonClicked(MyGUI::Widget* sender);
        void onOkButtonClicked(MyGUI::Widget* sender);
        void onDeleteButtonClicked(MyGUI::Widget* sender);

        MyGUI::TextBox* mTextEdit;
        MyGUI::Button* mOkButton;
        MyGUI::Button* mCancelButton;
        MyGUI::Button* mDeleteButton;
    };

    class MapWindow : public MWGui::WindowPinnableBase, public LocalMapBase, public NoDrop
    {
    public:
        MapWindow(CustomMarkerCollection& customMarkers, DragAndDrop* drag, MWRender::LocalMap* localMapRender, SceneUtil::WorkQueue* workQueue);
        virtual ~MapWindow();

        void setCellName(const std::string& cellName);

        virtual void setAlpha(float alpha);
        void setVisible(bool visible);

        void renderGlobalMap();

        /// adds the marker to the global map
        /// @param name The ESM::Cell::mName
        void addVisitedLocation(const std::string& name, int x, int y);

        // reveals this cell's map on the global map
        void cellExplored(int x, int y);

        void setGlobalMapPlayerPosition (float worldX, float worldY);
        void setGlobalMapPlayerDir(const float x, const float y);

        void ensureGlobalMapLoaded();

        virtual void onOpen();

        void onFrame(float dt);

        virtual void updateCustomMarkers();

        /// Clear all savegame-specific data
        void clear();

        void write (ESM::ESMWriter& writer, Loading::Listener& progress);
        void readRecord (ESM::ESMReader& reader, uint32_t type);

    private:
        void onDragStart(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onMouseDrag(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onWorldButtonClicked(MyGUI::Widget* _sender);
        void onMapDoubleClicked(MyGUI::Widget* sender);
        void onCustomMarkerDoubleClicked(MyGUI::Widget* sender);
        void onNoteEditOk();
        void onNoteEditDelete();
        void onNoteEditDeleteConfirm();
        void onNoteDoubleClicked(MyGUI::Widget* sender);
        void onChangeScrollWindowCoord(MyGUI::Widget* sender);
        void globalMapUpdatePlayer();
        void setGlobalMapMarkerTooltip(MyGUI::Widget* widget, int x, int y);

        MyGUI::ScrollView* mGlobalMap;
        std::unique_ptr<MyGUI::ITexture> mGlobalMapTexture;
        std::unique_ptr<MyGUI::ITexture> mGlobalMapOverlayTexture;
        MyGUI::ImageBox* mGlobalMapImage;
        MyGUI::ImageBox* mGlobalMapOverlay;
        MyGUI::ImageBox* mPlayerArrowLocal;
        MyGUI::ImageBox* mPlayerArrowGlobal;
        MyGUI::Button* mButton;
        MyGUI::IntPoint mLastDragPos;
        bool mGlobal;

        MyGUI::IntCoord mLastScrollWindowCoordinates;

        // Markers on global map
        typedef std::pair<int, int> CellId;
        std::set<CellId> mMarkers;

        MyGUI::Button* mEventBoxGlobal;
        MyGUI::Button* mEventBoxLocal;

        MWRender::GlobalMap* mGlobalMapRender;

        std::map<std::pair<int, int>, MyGUI::Widget*> mGlobalMapMarkers;

        EditNoteDialog mEditNoteDialog;
        ESM::CustomMarker mEditingMarker;

        virtual void onPinToggled();
        virtual void onTitleDoubleClicked();

        virtual void doorMarkerCreated(MyGUI::Widget* marker);
        virtual void customMarkerCreated(MyGUI::Widget *marker);

        virtual void notifyPlayerUpdate();

    };
}
#endif
