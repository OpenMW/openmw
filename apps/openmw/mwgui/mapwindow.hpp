#ifndef MWGUI_MAPWINDOW_H
#define MWGUI_MAPWINDOW_H

#include <stdint.h>

#include "windowpinnablebase.hpp"

namespace MWRender
{
    class GlobalMap;
}

namespace ESM
{
    class ESMReader;
    class ESMWriter;
}

namespace Loading
{
    class Listener;
}

namespace MWGui
{
    class LocalMapBase
    {
    public:
        LocalMapBase();
        virtual ~LocalMapBase();
        void init(MyGUI::ScrollView* widget, MyGUI::ImageBox* compass, OEngine::GUI::Layout* layout, bool mapDragAndDrop=false);

        void setCellPrefix(const std::string& prefix);
        void setActiveCell(const int x, const int y, bool interior=false);
        void setPlayerDir(const float x, const float y);
        void setPlayerPos(const float x, const float y);

        void onFrame(float dt);

        bool toggleFogOfWar();

        struct MarkerPosition
        {
            bool interior;
            int cellX;
            int cellY;
            float nX;
            float nY;
        };

    protected:
        int mCurX, mCurY;
        bool mInterior;
        MyGUI::ScrollView* mLocalMap;
        MyGUI::ImageBox* mCompass;
        std::string mPrefix;
        bool mChanged;
        bool mFogOfWar;

        std::vector<MyGUI::ImageBox*> mMapWidgets;
        std::vector<MyGUI::ImageBox*> mFogWidgets;

        // Keep track of created marker widgets, just to easily remove them later.
        std::vector<MyGUI::Widget*> mDoorMarkerWidgets; // Doors
        std::vector<MyGUI::Widget*> mMarkerWidgets; // Other markers

        void applyFogOfWar();

        void onMarkerFocused(MyGUI::Widget* w1, MyGUI::Widget* w2);
        void onMarkerUnfocused(MyGUI::Widget* w1, MyGUI::Widget* w2);

        MyGUI::IntPoint getMarkerPosition (float worldX, float worldY, MarkerPosition& markerPos);

        virtual void notifyPlayerUpdate() {}
        virtual void notifyMapChanged() {}

        // Update markers (Detect X effects, Mark/Recall effects)
        // Note, door markers are handled in setActiveCell
        void updateMarkers();
        void addDetectionMarkers(int type);

        OEngine::GUI::Layout* mLayout;

        float mMarkerUpdateTimer;

        bool mMapDragAndDrop;

        float mLastPositionX;
        float mLastPositionY;
        float mLastDirectionX;
        float mLastDirectionY;
    };

    class MapWindow : public MWGui::WindowPinnableBase, public LocalMapBase, public NoDrop
    {
    public:
        MapWindow(DragAndDrop* drag, const std::string& cacheDir);
        virtual ~MapWindow();

        void setCellName(const std::string& cellName);

        void renderGlobalMap(Loading::Listener* loadingListener);

        // adds the marker to the global map
        void addVisitedLocation(const std::string& name, int x, int y);

        // reveals this cell's map on the global map
        void cellExplored(int x, int y);

        void setGlobalMapPlayerPosition (float worldX, float worldY);

        virtual void open();

        void onFrame(float dt);

        /// Clear all savegame-specific data
        void clear();

        void write (ESM::ESMWriter& writer, Loading::Listener& progress);
        void readRecord (ESM::ESMReader& reader, int32_t type);

    private:
        void onDragStart(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onMouseDrag(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id);
        void onWorldButtonClicked(MyGUI::Widget* _sender);

        void globalMapUpdatePlayer();

        MyGUI::ScrollView* mGlobalMap;
        MyGUI::ImageBox* mGlobalMapImage;
        MyGUI::ImageBox* mGlobalMapOverlay;
        MyGUI::ImageBox* mPlayerArrowLocal;
        MyGUI::ImageBox* mPlayerArrowGlobal;
        MyGUI::Button* mButton;
        MyGUI::IntPoint mLastDragPos;
        bool mGlobal;

        // Markers on global map
        typedef std::pair<int, int> CellId;
        std::vector<CellId> mMarkers;

        // Cells that should be explored in the next frame (i.e. their map revealed on the global map)
        // We can't do this immediately, because the map update is not immediate either (see mNeedMapUpdate in scene.cpp)
        std::vector<CellId> mQueuedToExplore;

        MyGUI::Button* mEventBoxGlobal;
        MyGUI::Button* mEventBoxLocal;

        MWRender::GlobalMap* mGlobalMapRender;

    protected:
        virtual void onPinToggled();

        virtual void notifyPlayerUpdate();
        virtual void notifyMapChanged();

    };
}
#endif
